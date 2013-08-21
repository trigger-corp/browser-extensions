from contextlib import contextmanager
from functools import wraps
import os
import shutil
from os import error, listdir
from os.path import join, isdir, islink
import tempfile
import subprocess
import sys
import logging
import traceback
import signal
import json

import chardet
import requests

LOG = logging.getLogger(__name__)

# set up BASE_EXCEPTION early - it's relied upon by other imports
# use ForgeError if we're on the client, so it can catch us
try:
	from forge import ForgeError
	BASE_EXCEPTION = ForgeError
except ImportError:
	BASE_EXCEPTION = Exception

import build

class CouldNotLocate(BASE_EXCEPTION):
	pass

def task(function):
	build.Build.tasks[function.func_name] = function
	
	@wraps(function)
	def wrapper(*args, **kw):
		return function(*args, **kw)
	return wrapper
	
def predicate(function):
	build.Build.predicates[function.func_name] = function
	
	@wraps(function)
	def wrapper(*args, **kw):
		return function(*args, **kw)
	return wrapper
	
# modified os.walk() function from Python 2.4 standard library
def walk_with_depth(top, topdown=True, onerror=None, deeplevel=0): # fix 0
	"""Modified directory tree generator.

	For each directory in the directory tree rooted at top (including top
	itself, but excluding '.' and '..'), yields a 4-tuple

		dirpath, dirnames, filenames, deeplevel

	dirpath is a string, the path to the directory.  dirnames is a list of
	the names of the subdirectories in dirpath (excluding '.' and '..').
	filenames is a list of the names of the non-directory files in dirpath.
	Note that the names in the lists are just names, with no path components.
	To get a full path (which begins with top) to a file or directory in
	dirpath, do os.path.join(dirpath, name). 

	----------------------------------------------------------------------
	+ deeplevel is 0-based deep level from top directory
	----------------------------------------------------------------------
	...

	"""

	try:
		names = listdir(top)
	except error, err:
		if onerror is not None:
			onerror(err)
		return

	dirs, nondirs = [], []
	for name in names:
		if isdir(join(top, name)):
			dirs.append(name)
		else:
			nondirs.append(name)

	if topdown:
		yield top, dirs, nondirs, deeplevel # fix 1
	for name in dirs:
		path = join(top, name)
		if not islink(path):
			for x in walk_with_depth(path, topdown, onerror, deeplevel+1): # fix 2
				yield x
	if not topdown:
		yield top, dirs, nondirs, deeplevel # fix 3


@contextmanager
def cd(target_dir):
	'Change directory to :param:`target_dir` as a context manager - i.e. rip off Fabric'
	old_dir = os.getcwd()
	try:
		os.chdir(target_dir)
		yield target_dir
	finally:
		os.chdir(old_dir)

@contextmanager
def temp_file():
	'Return a path to save a temporary file to and delete afterwards'
	file = tempfile.mkstemp()
	try:
		os.close(file[0])
		os.remove(file[1])
		yield file[1]
	finally:
		if os.path.isfile(file[1]):
			os.remove(file[1])
			
@contextmanager
def temp_dir():
	'Return a path to a temporary directory and delete afterwards'
	dir = tempfile.mkdtemp()
	try:
		yield dir
	finally:
		shutil.rmtree(dir)

def read_file_as_str(filename):
	with open(filename, 'rb') as in_file:
		file_contents = in_file.read()

	try:
		unicode_res = file_contents.decode('utf8', 'strict')
	except UnicodeDecodeError:
		char_result = chardet.detect(file_contents)
		encoding = char_result.get('encoding', 'utf8')
		encoding = 'utf8' if encoding is None else encoding
		unicode_res = file_contents.decode(encoding)
	return unicode_res

def _shell_quote(word):
	"""Use single quotes to make the word usable as a single commandline
	argument on bash, so that debug output can be easily runnable.
	"""
	return "'" + word.replace("'", "'\"'\"'") + "'"

def _format_popen_args(args, kwargs, platform):
	if not args:
		return "Tried to run a subprocess but didn't specify anything to run!"
	
	if kwargs.get('shell'):
		return "Running shell: %s" % args[0]
	
	if sys.platform.startswith("win"):
		return "Running: %s" % subprocess.list2cmdline(args[0])
	
	return "Running: %s" % " ".join(_shell_quote(a) for a in args[0])

# TODO: this is duplicated in build tools, we should figure out a way to share
# the code between generate_dynamic and build tools sensibly
class PopenWithoutNewConsole(subprocess.Popen):
	"""Wrapper around Popen that adds the appropriate options to prevent launching
	a new console window everytime we want to launch a subprocess.
	"""
	_old_popen = subprocess.Popen

	def __init__(self, *args, **kwargs):
		if sys.platform.startswith("win") and 'startupinfo' not in kwargs:
			startupinfo = subprocess.STARTUPINFO()
			startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
			startupinfo.wShowWindow = subprocess.SW_HIDE
			kwargs['startupinfo'] = startupinfo

		LOG.debug(_format_popen_args(args, kwargs, platform=sys.platform))
		self._old_popen.__init__(self, *args, **kwargs)


def expand_relative_path(build, *possibly_relative_path):
	"""Expands a path relative to the original working directory a build started in.

	Example usage:

	>> build.orig_wd
	'/home/monk/my-app'

	>> lib.expand_relative_path(build, '.template/lib', 'apksigner.jar')
	'/home/monk/my-app/.template/lib/apksigner.jar'

	>> lib.expand_relative_path(build, '/absolute/path/to/stuff')
	'/absolute/path/to/stuff'

	>> lib.expand_relative_path(build, '../release.keystore')
	'/home/release.keystore'
	"""
	return os.path.normpath(os.path.join(build.orig_wd, *possibly_relative_path))


def ask_multichoice(question_text, choices, radio=True):
	"""Ask a multichoice question and block until we get a response"""
	while True:
		field_name = 'answer'
		call = current_call()

		event_id = call.emit('question', schema={
			'title': question_text,
			'properties': {
				field_name: {
					'type': 'string',
					'enum': choices,
					'title': 'Choice',
					'_radio': radio,
				}
			}
		})

		response = call.wait_for_response(event_id)
		
		if response.get('data') is None:
			raise BASE_EXCEPTION("User aborted")
		response_data = response['data']

		if field_name not in response_data:
			LOG.warning("Invalid response, expected field: %s" % field_name)
			continue
		answer = response_data[field_name]

		try:
			choice = choices.index(answer) + 1
		except ValueError:
			LOG.debug("User gave invalid response")
			continue
		return choice


class ProgressBar(object):
	"""Helper context manager to emit progress events. e.g.

	with ProgressBar('Downloading something awesome'):
		time.sleep('2')
		bar.progress(0.25) # 25% complete
		time.sleep('2')
		bar.progress(0.5) # 50% complete

	# 100% complete if finishes without exception

	*N.B* any logging occuring during the progress bar will mess up
	how it looks in the commandline, might be able to resolve this later
	by erasing the progress bar, printing the log output then printing the progress bar.
	"""
	def __init__(self, message):
		self._message = message
		self._call = current_call()

	def __enter__(self):
		self._call.emit('progressStart', message=self._message)
		return self

	def progress(self, fraction):
		self._call.emit('progress', fraction=fraction, message=self._message)

	def __exit__(self, exc_type, exc_val, exc_tb):
		if exc_type is not None:
			self.progress(1)
		self._call.emit('progressEnd', message=self._message)

def import_async():
	'If in client-side environment, import async from forge. Mock, otherwise'
	try:
		from forge import async
	except ImportError:
		import mock
		async = mock.Mock()
	return async

def current_call():
	return import_async().current_call()

def download_with_progress_bar(progress_bar_title, url, destination_path):
	"""Download something from a given URL, emitting progress events if possible
	"""
	download_response = requests.get(url)
	content_length = download_response.headers.get('content-length')

	with ProgressBar(progress_bar_title) as bar:
		with open(destination_path, 'wb') as destination_file:
			bytes_written = 0
			for chunk in download_response.iter_content(chunk_size=102400):
				if content_length:
					content_length = int(content_length)
					destination_file.write(chunk)
					bytes_written += len(chunk)
					fraction_complete = float(bytes_written) / content_length
					bar.progress(fraction_complete)


def set_dotted_attributes(build, unexpanded_local_config_values):
	"""Save a local_config value given in dotted form to the local_config.json for the current build.

	Example use::
		set_dotted_attribute(build, 'ie.profile.nsis', '/opt/localbin/makensis')
	"""
	from forge import build_config
	local_config_values = {expand_profile(build, k): v for k, v in unexpanded_local_config_values.items()}
	LOG.info('Updating local config with %s' % json.dumps(local_config_values, indent=4))

	with cd(build.orig_wd):
		local_config = build_config.load_local()

		for attribute_name, value in local_config_values.items():
			current_level = local_config
			crumbs = attribute_name.split('.')
			for k in crumbs[:-1]:
				if k not in current_level:
					current_level[k] = {}
				current_level = current_level[k]
			current_level[crumbs[-1]] = value

		build_config.save_local(local_config)


def progressive_kill(pid, kill_process_group=False):
	if sys.platform.startswith("win"):
		commands_to_try = ['TASKKILL /PID {pid}', 'TASKKILL /F /PID {pid}']
		with open(os.devnull) as devnull:
			for command in commands_to_try:
				kill_proc = subprocess.Popen(command.format(pid=pid), shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
				out = kill_proc.communicate()[0]
				if kill_proc.poll() == 0:
					LOG.debug("{command} succeeded".format(command=command))
					return
				else:
					LOG.debug("{command} failed".format(command=command))
					LOG.debug(out)

	else:
		signals_to_try = ['SIGINT', 'SIGTERM', 'SIGKILL']
		
		for signal_name in signals_to_try:
			signal_num = getattr(signal, signal_name)
			try:
				if kill_process_group:
					os.killpg(pid, signal_num)
				else:
					os.kill(pid, signal_num)
				LOG.debug("{signal_name} succeeded".format(signal_name=signal_name))
				return
			except Exception as e:
				LOG.debug("{signal_name} failed".format(signal_name=signal_name))
				LOG.debug(traceback.format_exc(e))


def interactive(build):
	return build.tool_config.get('general.interactive', True)


def expand_dotted_dict(dotted_dict):
	top_level = {}

	for key, val in dotted_dict.items():
		crumbs = key.split('.')

		current_level = top_level
		for crumb in crumbs[:-1]:
			if crumb not in current_level:
				current_level[crumb] = {}

			current_level = current_level[crumb]
		current_level[crumbs[-1]] = val

	return top_level

def _format_override(dotted_name):
	"""e.g.

	foo.profiles.DEFAULT.bar -> foo.profile.bar
	foo.bar -> foo.bar
	foo.baz.bar -> foo.baz.bar
	"""
	parts = dotted_name.split('.')
	if len(parts) > 1 and parts[1] == 'profiles':
		new_parts = [parts[0], 'profile'] + parts[3:]
		return ".".join(new_parts)
	else:
		return dotted_name

def local_config_problem(build, message, examples, more_info=None):
	if interactive(build):
		config = json.dumps(expand_dotted_dict(examples), indent=4)

		override_args = ['--%s "%s"' % (_format_override(k), v) for k, v in examples.items()]
		cmdline = "forge %s" % " ".join(override_args)

		exc_message = """{message}

To resolve this problem, you can modify the file "local_config.json" to something like:
{config}

Or pass commandline arguments to forge:
{cmdline}
""".format(message=message, config=config, cmdline=cmdline)

	else:
		exc_message = """{message}

You can modify the Local config for your app to resolve this problem.
""".format(message=message)
	
	if more_info:
		exc_message += """
More information is available at:
[{more_info}]
""".format(more_info=more_info)

	raise BASE_EXCEPTION(exc_message)

def get_or_ask_for_local_config(build, required_info, question_title):
	known_info = {}
	unknown_info = {}

	# figure out which info we have in local_config.json and which we need to ask for
	for dotted_name in required_info.keys():
		if dotted_name in build.tool_config:
			known_info[dotted_name] = build.tool_config.get(dotted_name)
		else:
			unknown_info[dotted_name] = required_info[dotted_name]

	# if there's anything we need to ask for, ask for it
	if unknown_info:
		event_id = current_call().emit('question', schema={
			'title': question_title,
			'properties': unknown_info
		})

		response = current_call().wait_for_response(event_id)
		if not response.get('data'):
			raise BASE_EXCEPTION("User aborted")

		answers = response['data']
		known_info.update(answers)
		set_dotted_attributes(build, answers)

	return known_info

def expand_profile(build, dotted_name):
	parts = dotted_name.split('.')
	with_profile_replaced = ['profiles.%s' % build.tool_config.profile() if p == 'profile' else p for p in parts]
	return ".".join(with_profile_replaced)

IGNORED_FILE_PREFIXES = ["."]
IGNORED_FILE_SUFFIXES = ["~", ".swp"]
IGNORED_DIRS = [".git", ".svn", ".hg", "defaults"]

def filter_filenames(filenames, ignored_files=[".hgignore"]):
	for filename in filenames:
		if filename in ignored_files:
			continue
		if any([filename.startswith(suffix)
				for suffix in IGNORED_FILE_PREFIXES]):
			continue
		if any([filename.endswith(suffix)
				for suffix in IGNORED_FILE_SUFFIXES]):
			continue
		yield filename

def filter_dirnames(dirnames):
	return [dirname for dirname in dirnames if dirname not in IGNORED_DIRS]
