# XXX should consolidate this with lib
import logging
from os import path
import subprocess
import StringIO
import hashlib
import json
import sys
import os
import stat
import time
import threading
import Queue
import urlparse

import lib

from genshi.template import NewTextTemplate

LOG = logging.getLogger(__name__)

class ShellError(lib.BASE_EXCEPTION):
	def __init__(self, message, output):
		self.message = message
		self.output = output

	def __str__(self):
		return "%s: %s" % (self.message, self.output)

# # # # # # # # # # # # # # # # # # # 
#
# Data transform
# TODO XPath or similar?
#
# # # # # # # # # # # # # # # # # # # 
def transform(data, node_steps, fn, allow_set=False):
	'''Mutate an arbitrary nested dictionary/array combination with the given function.
	
	``node_steps`` is dot-separated instructions on how to arrive at the data node
	which needs changing::
	
		array_name.[]
		dictionary.key_name
		dictionary.*			   // all keys in a dictionary

	:param data: a nested dictionary / array combination
	:type data: ``dict``
	:param node_steps: dot-separated data path, e.g. my_dict.my_array.[].*.target_key
	:param fn: mutating function - will be passed the data found at the end
		``node_steps``, and should return the desired new value
	:param allow_set: if True the mutating function will be called with None for none
		existing keys - i.e. you can set new keys
	'''
	obj = data.copy()
	list(_handle_all(obj, node_steps.split('.'), fn, allow_set))
	return obj

def _yield_plain(obj, name):
	'If obj is a dictionary, yield an attribute'
	if hasattr(obj, '__contains__') and name in obj:
		yield obj[name]
def _yield_array(obj):
	'Yield all elements of an array'
	assert hasattr(obj, '__iter__'), 'Expecting an array, got %s' % obj
	for thing in obj:
		yield thing
def _yield_asterisk(obj):
	'Yield all values in a dictionary'
	if hasattr(obj, 'iteritems'):
		for _, value in obj.iteritems():
			yield value
def _yield_any(obj, name):
	'Yield a value, or array or dictionary values'
	if name == '*':
		return _yield_asterisk(obj)
	elif name == '[]':
		return _yield_array(obj)
	else:
		return _yield_plain(obj, name)

def recurse_dict(dictionary, fn):
	'''
	if the property isn't a string, recurse till it is
	'''
	for key, value in dictionary.iteritems():
		if hasattr(value, 'iteritems'):
			recurse_dict(value, fn)
		else:
			dictionary[key] = fn(value)

def _handle_all(obj, steps, fn, allow_set):
	if len(steps) > 1:
		for value in _yield_any(obj, steps[0]):
			for x in _handle_all(value, steps[1:], fn, allow_set):
				yield x
	else:
		step = steps[0]
		if step == '*':
			assert hasattr(obj, 'iteritems'), 'Expecting a dictionary, got %s' % obj
			recurse_dict(obj, fn)
		elif step == '[]':
			assert hasattr(obj, '__iter__'), 'Expecting an array, got %s' % obj
			for i, x in enumerate(obj):
				obj[i] = fn(x)
		else:
			if hasattr(obj, '__contains__') and step in obj:
				obj[step] = fn(obj[step])
			elif allow_set:
				obj[step] = fn(None)
	
# # # # # # # # # # # # # # # # # # # 
#
# End data transform
#
# # # # # # # # # # # # # # # # # # # 

def render_string(config, in_s):
	'''Render a Genshi template as a string
	
	:param config: data dictionary
	:param in_s: genshi template
	'''
	tmpl = NewTextTemplate(in_s)

	# older versions of python don't allow unicode keyword arguments
	# so we have to encode the keys (for best compatibility in the client side tools)
	config = _encode_unicode_keys(config)
	return tmpl.generate(**config).render('text').decode('utf8')

def _encode_unicode_keys(dictionary):
	'''Returns a new dictionary constructed from the given one, but with the keys encoded as strings.
	:param dictionary: dictionary to encode the keys for

	(For use with old versions of python that can't use unicode keys for keyword arguments)'''

	new_items = [(str(k), v) for k, v in dictionary.items()]
	return dict(new_items)

def _resolve_url(config, url, prefix):
	'''Prefix non-absolute URLs with the path to the user's code'''
	if hasattr(url, "startswith"):
		# string
		if url.startswith('http://') or \
				url.startswith('https://') or \
				url.startswith(prefix):
			return url
		else:
			return prefix + url if url.startswith('/') else prefix + '/' + url
	else:
		# summat else
		return url

class RunnerState(object):
	pass

class ProcessGroup(object):
	"""Helper for managing a collection of processes and ensuring they're all shut down."""

	def __init__(self):
		self._running_processes = {}
		self._notifications = Queue.Queue()
		self._procs = 0

	def spawn(self, *args, **kw):
		"""Create (and start) a new process."""
		self._procs += 1
		group_id = self._procs
		proc = ProcessWithLogging(group_id, self._notifications, *args, **kw)
		proc.start()
		self._running_processes[group_id] = proc

	def wait_for_success(self):
		"""Wait for every process to succeed.
		
		If one process fails, shuts all the other processes down.
		If an interrupt is received from the tools, shuts all processes down.
		"""
		call = lib.current_call()

		try:
			while self._running_processes != {}:
				try:
					finished_pid = self._notifications.get(timeout=1)
					finished_proc = self._running_processes[finished_pid]
					finished_proc.assert_success()
					del self._running_processes[finished_pid]

				except Queue.Empty:
					call.assert_not_interrupted()

		finally:
			self._shutdown_running_processes()

	def _shutdown_running_processes(self):
		for pid, proc in self._running_processes.items():
			proc.kill()

class ProcessWithLogging(object):
	"""Wrapper around a subprocess.Popen
	
	Logs output from it the subprocess.
	Notifies through a queue when it has finished.
	"""
	def __init__(self, group_id, notify, args, **kw):
		self._notify = notify
		self._group_id = group_id
		self._args = args
		self._check_for_interrupt = kw.get('check_for_interrupt', False)
		self._command_log_level = kw.get("command_log_level", logging.DEBUG)
		self._filter = kw.get("filter", False)
		self._env = kw.get("env")
		self._fail_if = kw.get("fail_if")

		self._state = RunnerState()
		self._state.done = False
		self._state.output = StringIO.StringIO()
		self._state.proc = None
		self._state.error = None

		self._process_launched = Queue.Queue()

	def start(self):
		self._runner_thread = threading.Thread(target=self._runner)
		self._runner_thread.daemon = True
		self._runner_thread.start()
		self._wait_until_process_started()

	def _wait_until_process_started(self):
		try:
			process_launched = self._process_launched.get(timeout=20)
		except Queue.Empty:
			raise ShellError('Failed to start up subprocess "%s", took longer than 20 seconds.', output='')

		if process_launched != self.PROCESS_START_SUCCESS:
			raise self._state.error

	def pid(self):
		return self._state.proc.pid

	def assert_success(self):
		assert self._state.done

		if self._state.error:
			raise self._state.error
	
	def kill(self):
		import lib
		lib.progressive_kill(self._state.proc.pid)

	PROCESS_START_SUCCESS = 9001
	PROCESS_START_FAILURE = 9002
	PROCESS_FINISH_SUCCESS = 9003
	PROCESS_EXCEPTION = 9004

	def _runner(self):
		try:
			self._state.proc = lib.PopenWithoutNewConsole(self._args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=self._env)
			self._process_launched.put(self.PROCESS_START_SUCCESS)

			for line in iter(self._state.proc.stdout.readline, ''):
				if not self._filter or self._filter(line):
					self._state.output.write(line)
					LOG.log(self._command_log_level, line.rstrip('\r\n'))

				if self._fail_if and self._fail_if(line):
					raise ShellError('Detected failure based on output of subprocess "%s"' % self._args[0], self._state.output.getvalue())


		except Exception as e:
			if self._state.proc is None:
				self._process_launched.put(self.PROCESS_START_FAILURE)
			else:
				self._process_launched.put(self.PROCESS_EXCEPTION)
			self._state.error = e

		finally:
			self._finished()

	def _finished(self):
		self._state.done = True
		self._notify.put(self._group_id)

# TODO: extract common logic in ProcessWithLogging and run_shell
def _required_preexec(create_process_group, os):
	if create_process_group and getattr(os, 'setsid', None) is not None:
		return os.setsid

def run_shell(*args, **kw):
	check_for_interrupt = kw.get('check_for_interrupt', False)
	create_process_group = kw.get('create_process_group', False)
	fail_silently = kw.get('fail_silently', False)
	command_log_level = kw.get("command_log_level", logging.DEBUG)
	filter = kw.get("filter", False)

	state = RunnerState()
	state.done = False
	state.output = StringIO.StringIO()
	state.proc = None
	state.error = None

	def runner():
		try:
			preexec_fn = _required_preexec(create_process_group, os)
			state.proc = lib.PopenWithoutNewConsole(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=kw.get('env'), preexec_fn=preexec_fn)

			for line in iter(state.proc.stdout.readline, ''):
				if filter:
					line = filter(line)
				if line != False:
					state.output.write(line)
					LOG.log(command_log_level, line.rstrip('\r\n'))

			state.done = True
		except Exception as e:
			state.done = True
			state.error = e


	if check_for_interrupt:
		try:
			call = lib.current_call()
			runner_thread = threading.Thread(target=runner)
			runner_thread.daemon = True
			runner_thread.start()

			while not state.done:
				time.sleep(1)
				call.assert_not_interrupted()
		finally:
			# if interrupted, kill child process
			if state.proc and not state.done:
				lib.progressive_kill(state.proc.pid, kill_process_group=create_process_group)

	else:
		runner()

	if state.error:
		raise state.error

	if state.proc.wait() != 0:
		if fail_silently:
			LOG.debug('Failed to run %s, but was told to carry on anyway' % subprocess.list2cmdline(args))
		else:
			raise ShellError(
				message="Failed when running {command}".format(command=args[0]),
				output=state.output.getvalue()
			)

	return state.output.getvalue()

def path_to_lib():
	return path.abspath(path.join(
		__file__,
		path.pardir,
		path.pardir,
		'lib',
	))

def ensure_lib_available(build, file):
	# In case of forge-generate check for file
	server_path = path.abspath(path.join(path.split(path.abspath(__file__))[0], '..', 'lib', file))
	if path.isfile(server_path):
		return server_path

	lib_dir = path.join(path.dirname(build.source_dir), '.lib')
	hash_path = path.join(path.dirname(build.source_dir), '.template', 'lib', 'hash.json')
	if not path.exists(lib_dir):
		os.makedirs(lib_dir)
		
	# Hide directory on windows
	if sys.platform == 'win32':
		try:
			lib.PopenWithoutNewConsole(['attrib', '+h', lib_dir]).wait()
		except Exception:
			# don't care if we fail to hide the templates dir
			pass
	
	hashes = None
	if path.exists(hash_path):
		with open(hash_path, 'r') as hash_file:
			hashes = json.load(hash_file)
	
	file_path = path.join(lib_dir, file)

	if path.exists(file_path) and file in hashes:
		# Check hash
		with open(file_path, 'rb') as cur_file:
			hash = hashlib.md5(cur_file.read()).hexdigest()
			if hash == hashes[file]:
				# File exists and is correct
				build.log.debug("File: %s, already downloaded and correct." % file)
				return file_path

	# File doesn't exist, or has the wrong hash or has no known hash - download
	build.log.info("Downloading lib file: %s, this will only happen when a new file is available." % file)
	
	from forge.remote import Remote
	from forge import build_config
	config = build_config.load()
	remote = Remote(config)

	server_details = urlparse.urlparse(remote.server)
	url = "{protocol}://{netloc}/lib-static/{platform_version}/{file}".format(
		protocol=server_details.scheme,
		netloc=server_details.netloc,
		platform_version=build.config['platform_version'],
		file=file
	)
	remote._get_file(url, file_path)
	
	# Make file executable.
	os.chmod(file_path, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IWGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
	
	return file_path

def which(program):
	"http://stackoverflow.com/questions/377017/test-if-executable-exists-in-python"
	def is_exe(fpath):
		return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

	if sys.platform.startswith("win"):
		programs = [".".join((program, extension)) for extension in ("cmd", "exe", "bat")]
		programs.insert(0, program)
	else:
		programs = [program]

	for program_name in programs:
		fpath, fname = os.path.split(program_name)
		if fpath:
			if is_exe(program_name):
				LOG.debug("using {name} for {program}".format(
					name=program_name, program=program))
				return program_name
		else:
			for path in os.environ["PATH"].split(os.pathsep):
				exe_file = os.path.join(path, program_name)
				if is_exe(exe_file):
					LOG.debug("using {name} for {program}".format(
						name=exe_file, program=program))
					return exe_file

	return None
