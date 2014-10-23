import codecs
import fnmatch
import glob
import os
from os import path
import re
import shutil
import sys
import uuid
import json
from copy import copy

from build import ConfigurationError
import firefox_tasks
import lib
from lib import task, walk_with_depth, read_file_as_str, cd
import safari_tasks
import ie_tasks
import utils
import hashlib
import pystache

from xml.etree import ElementTree

@task
def migrate_to_plugins(build):
	config = build.config
	if config['config_version'] == "4":
		return config

	build.log.debug("Migrating config.json from version %s to version 4" % config['config_version'])

	new_config = {
		# Required config
		"uuid": config["uuid"],
		"config_version": "4",
		"name": config["name"],
		"author": config["author"],
		"platform_version": config["platform_version"],
		"version": config["version"],
		"plugins": config.get("plugins", {}),
		"core": config.get("requirements", {}),
		"config_hash": config.get("config_hash", "CONFIG_HASH_HERE"),
		# TODO: Should this be here?
		"logging": config["modules"].get("logging", {}),

		# Extra stuff we've added to config
		"plugin_url_prefix": config.get("plugin_url_prefix", None)
	}

	if "general" not in new_config["core"]:
		new_config["core"]["general"] = {}
	if "reload" not in new_config["core"]["general"]:
		new_config["core"]["general"]["reload"] = False
	if "trusted_urls" not in new_config["core"]["general"] and "trusted_urls" in config:
		new_config["core"]["general"]["trusted_urls"] = config["trusted_urls"]

	if "description" in config:
		new_config["description"] = config["description"]
	if "homepage" in config:
		new_config["homepage"] = config["homepage"]

	if "partners" in config and "parse" in config["partners"]:
		new_config["plugins"]["parse"] = {
			"hash": "notahash",
			"config": config["partners"]["parse"]
		}

	for module in config["modules"]:
		# previously setting a module config to false would just enable it with no configuration
		# preserve backwards compatibility
		if config["modules"][module] is False:
			config["modules"][module] = True

		if module == "parameters":
			# Duplicate paramter settings to old location for compatibility
			new_config["modules"] = {
				module: config["modules"][module]
			}

		if module == "requirements":
			# These modules are now core and configured under top level core
			for platform in config["modules"][module]:
				if platform not in new_config["core"]:
					new_config["core"][platform] = {}
				for setting in config["modules"][module][platform]:
					new_config["core"][platform][setting] = config["modules"][module][platform][setting]
			continue

		if module == "package_names":
			for platform in config["modules"]["package_names"]:
				if platform not in new_config["core"]:
					new_config["core"][platform] = {}
				new_config["core"][platform]["package_name"] = config["modules"]["package_names"][platform]
			continue

		if module == "reload":
			new_config["core"]["general"]["reload"] = True
			continue

		if module in ["is", "event", "internal", "logging", "message", "tools"]:
			# These are now core, ignore them
			continue

		new_config["plugins"][module] = {
			"hash": "notahash"
		}
		if config["modules"][module] == True:
			continue

		if module == "activations":
			new_config["plugins"]["activations"]["config"] = {
				"activations": config["modules"]["activations"]
			}
			continue

		new_config["plugins"][module]["config"] = config["modules"][module]

	build.config = new_config

	if os.path.isfile(os.path.abspath(os.path.join(os.path.split(__file__)[0], '..', '..', 'configuration', 'schema_for_v2.json'))):
		import validictory
		with open(os.path.abspath(os.path.join(os.path.split(__file__)[0], '..', '..', 'configuration', 'schema_for_v2.json'))) as schema_file:
			validictory.validate(build.config, json.load(schema_file))

@task
def add_element_to_xml(build, file, element, to=None, unless=None):
	'''add new element to an XML file

	:param file: filename or file object
	:param element: dict containing tag and optionally attributes, text and children
	:param to: sub element tag name or path we will append to
	:param unless: don't add anything if this tag name or path already exists
	'''
	def create_element(tag, attributes={}, text=None, children=[]):
		for attribute in attributes:
			if isinstance(attributes[attribute], str) or isinstance(attributes[attribute], unicode):
				attributes[attribute] = pystache.render(attributes[attribute], build.config)
		element = ElementTree.Element(tag, attributes)
		if text is not None:
			if isinstance(text, str) or isinstance(text, unicode):
				text = pystache.render(text, build.config)
			element.text = text
		for child in children:
			element.append(create_element(**child))

		return element

	xml = ElementTree.ElementTree()
	xml.parse(file)
	if to is None:
		el = xml.getroot()
	else:
		el = xml.find(to, dict((v,k) for k,v in ElementTree._namespace_map.items()))
	if not unless or xml.find(unless, dict((v,k) for k,v in ElementTree._namespace_map.items())) is None:
		new_el = create_element(**element)
		el.append(new_el)
		xml.write(file)

@task
def set_element_value_xml(build, file, value, element=None):
	'''set text contents of an XML element

	:param build: the current build.Build
	:param file: filename or file object
	:param value: the new element contents (will be templated)
	:param element: tag name or path to change (defaults to root node)
	'''
	xml = ElementTree.ElementTree()
	xml.parse(file)
	if element is None:
		el = xml.getroot()
	else:
		el = xml.find(element, dict((v,k) for k,v in ElementTree._namespace_map.items()))
	el.text = utils.render_string(build.config, value).decode('utf8', errors='replace')
	xml.write(file)

@task
def set_attribute_value_xml(build, file, value, attribute, element=None):
	'''set contents of an XML element's attribute

	:param build: the current build.Build
	:param file: filename or file object
	:param value: the new attribute value (will be templated)
	:param attribute: attribute name
	:param element: tag name or path to change (defaults to root node)
	'''
	xml = ElementTree.ElementTree()
	xml.parse(file)
	if element is None:
		el = xml.getroot()
	else:
		el = xml.find(element, dict((v,k) for k,v in ElementTree._namespace_map.items()))
	
	# set is not aware of namespaces, so we have to replace "namespace" with "{schema}"
	namespaces = dict((v,k) for k,v in ElementTree._namespace_map.items())
	if ":" in attribute:
		parts = attribute.split(":")
		attribute = "{%s}%s" % (namespaces[parts[0]], parts[1])
	
	el.set(attribute, utils.render_string(build.config, value))
	
	xml.write(file)

@task
def rename_files(build, **kw):
	if 'from' not in kw or 'to' not in kw:
		raise ConfigurationError('rename_files requires "from" and "to" keyword arguments')

	return _rename_or_copy_files(build, kw['from'], kw['to'], rename=True)

@task
def copy_files(build, **kw):
	if 'from' not in kw or 'to' not in kw:
		raise ConfigurationError('copy_files requires "from" and "to" keyword arguments')
		
	return _rename_or_copy_files(build, kw['from'], kw['to'], rename=False, ignore_patterns=kw.get('ignore_patterns'))

class Pattern(object):
	def __init__(self, type, value):
		self.type = type
		self.value = value

def git_ignore(root, patterns):
	classified_patterns = []
	with cd(root):
		for pattern in patterns:
			if pattern:
				if '/' in pattern[:-1]:
					ignored_paths = (Pattern('path', match) for match in glob.glob(pattern))
					classified_patterns.extend(ignored_paths)
				else:
					classified_patterns.append(Pattern('file', pattern))

	def git_ignorer(src, names):
		relative_src = src[len(root):].lstrip(r"""\/""")
		ignored = []
		for name in names:
			for pattern in classified_patterns:
				if pattern.type == 'path':
					if path.join(relative_src, name) == os.path.normpath(pattern.value):
						ignored.append(name)
				elif pattern.type == 'file':
					ignore_name = pattern.value
					if pattern.value[-1] in ('/', '\\'):
						if path.isdir(path.join(src, name)):
							ignore_name = ignore_name[:-1]

					if fnmatch.fnmatch(name, ignore_name):
						ignored.append(name)

		return set(ignored)

	return git_ignorer

@task
def _rename_or_copy_files(build, frm, to, rename=True, ignore_patterns=None):
	if ignore_patterns is None:
		ignore_patterns = []

	from_, to = utils.render_string(build.config, frm), utils.render_string(build.config, to)
	if path.isdir(from_):
		ignore_func = git_ignore(from_, ignore_patterns)
	else:
		ignore_func = None

	if rename:
		build.log.debug('renaming {from_} to {to}'.format(**locals()))
		shutil.move(from_, to)
	else:
		if '*' in to:
			# looks like a glob - last directory in path might not exist.
			tos = glob.glob(path.dirname(to))
			tos = [path.join(t,path.basename(to)) for t in tos]
		else:
			# don't glob in case the to path doesn't exist yet
			tos = [to]
		
		for found_to in tos:
			build.log.debug('copying {from_} to {found_to}'.format(**locals()))
			if path.isdir(from_):
				shutil.copytree(from_, found_to, ignore=ignore_func)
			else:
				shutil.copy(from_, found_to)

@task
def find_and_replace(build, *files, **kwargs):
	'''replace one string with another in a set of files
	
	:param kwargs: must contain ``find`` and ``replace`` keys, 
	representing the string to look for, and string to replace
	with, respectively.
	
	:param kwargs: can also contain the ``template`` boolean
	argument, which determines if we will run the ``replace``
	argument through genshi templating first (defaults to True).
	
	:param files: array of glob patterns to select files
	:param kwargs: must contain ``find`` and ``replace`` keys
	'''
	if "in" in kwargs:
		files = kwargs['in']
	if "find" not in kwargs:
		raise ConfigurationError("Find not passed in to find_and_replace")
	if "replace" not in kwargs:
		raise ConfigurationError("Replace not passed in to find_and_replace")
	template = kwargs.get('template', True)
	find = kwargs["find"]
	replace = kwargs['replace']
	if template:
		replace = utils.render_string(build.config, replace)

	replace_summary = replace[:60]+'...' if len(replace) > 60 else replace
	build.log.debug("replacing %s with %s" % (find, repr(replace_summary)))

	for glob_str in files:
		found_files = glob.glob(utils.render_string(build.config, glob_str))
		if len(found_files) == 0:
			build.log.warning('No files were found to match pattern "%s"' % glob_str)
		for _file in found_files:
			_replace_in_file(build, _file, find, replace)

@task
def write_config(build, filename, content):
	# We hang various things we shouldn't off config, this is pretty horrible
	clean_config = copy(build.config)
	if 'json' in clean_config:
		clean_config.pop('json')
	if 'plugin_url_prefix' in clean_config:
		clean_config.pop('plugin_url_prefix')
	content = utils.render_string({'config': json.dumps(clean_config, indent=4, sort_keys=True)}, content)

	with open(filename, 'w') as fileobj:
		fileobj.write(content)

@task
def insert_head_tag(build, root_dir, tag, file_suffixes=("html",), template=False, **kw):
	'''For all files ending with one of the suffixes, under the root_dir, insert ``tag`` as
	early as possible after the <head> tag but after any <meta> tags.
	'''
	if template:
		tag = utils.render_string(build.config, tag)

	build.log.debug("inserting {tag} into <head> of {files}".format(
		tag=tag, files="{0}/**/*.{1}".format(root_dir, file_suffixes)
	))

	find = r'<head>((\s*<meta[^>]+>)*)'
	replace = r'<head>\1\n' + tag

	find_and_replace_in_dir(build, root_dir, find, replace, file_suffixes)

@task
def find_and_replace_in_dir(build, root_dir, find, replace, file_suffixes=("html",), template=False, **kw):
	'For all files ending with one of the suffixes, under the root_dir, replace ``find`` with ``replace``'
	if template:
		replace = utils.render_string(build.config, replace)

	build.log.debug("replacing {find} with {replace} in {files}".format(
		find=find, replace=replace, files="{0}/**/*.{1}".format(root_dir, file_suffixes)
	))
	
	found_roots = glob.glob(root_dir)
	if len(found_roots) == 0:
		build.log.warning('No files were found to match pattern "%s"' % root_dir)
	for found_root in found_roots:
		for root, _, files, depth in walk_with_depth(found_root):
			for file_ in files:
				if file_.rpartition('.')[2] in file_suffixes:
					find_with_fixed_path = find.replace("%{back_to_parent}%", "../" * (depth+1))
					replace_with_fixed_path = replace.replace("%{back_to_parent}%", "../" * (depth+1))
					regex_replace_in_file(build, path.join(root, file_), find_with_fixed_path, replace_with_fixed_path)

def _replace_in_file(build, filename, find, replace):
	build.log.debug(u"replacing {find} with {replace} in {filename}".format(**locals()))
	
	tmp_file = uuid.uuid4().hex
	in_file_contents = read_file_as_str(filename)
	in_file_contents = in_file_contents.replace(find, replace)
	with codecs.open(tmp_file, 'w', encoding='utf8') as out_file:
		out_file.write(in_file_contents)
	os.remove(filename)
	shutil.move(tmp_file, filename)
	
@task
def remove_lines_in_file(build, filename, containing):
	build.log.debug("removing lines containing '{containing}' in {filename}".format(**locals()))
	
	tmp_file = uuid.uuid4().hex
	in_file_contents = read_file_as_str(filename)
	in_file_contents = re.sub(r".*"+re.escape(containing)+r".*\r?\n?", "", in_file_contents)
	with codecs.open(tmp_file, 'w', encoding='utf8') as out_file:
		out_file.write(in_file_contents)
	os.remove(filename)
	shutil.move(tmp_file, filename)

@task
def regex_replace_in_file(build, filename, find, replace, template=False):
	build.log.debug("regex replace in {filename}".format(**locals()))
	
	if template:
		replace = utils.render_string(build.config, replace)
	
	tmp_file = uuid.uuid4().hex
	in_file_contents = read_file_as_str(filename)
	in_file_contents = re.sub(find, replace, in_file_contents)
	with codecs.open(tmp_file, 'w', encoding='utf8') as out_file:
		out_file.write(in_file_contents)
	os.remove(filename)
	shutil.move(tmp_file, filename)

@task
def set_in_biplist(build, filename, key, value):
	# biplist import must be done here, as in the server context, biplist doesn't exist
	import biplist
	
	if isinstance(value, str):
		value = utils.render_string(build.config, value)
	
	build.log.debug(u"setting {key} to {value} in {files}".format(
		key=key, value=value, files=filename
	))
	
	found_files = glob.glob(filename)
	if len(found_files) == 0:
		build.log.warning('No files were found to match pattern "%s"' % filename)
	for found_file in found_files:
		plist = biplist.readPlist(found_file)
		plist = utils.transform(plist, key, lambda _: value, allow_set=True)
		biplist.writePlist(plist, found_file)

@task
def set_in_json(build, filename, key, value):
	if isinstance(value, str):
		value = utils.render_string(build.config, value)
	
	build.log.debug("setting {key} to {value} in {files}".format(
		key=key, value=value, files=filename
	))
	
	found_files = glob.glob(filename)
	if len(found_files) == 0:
		build.log.warning('No files were found to match pattern "%s"' % filename)
	for found_file in found_files:
		file_json = {}
		with open(found_file, "r") as opened_file:
			file_json = json.load(opened_file)
			# TODO: . separated keys?
			file_json[key] = value
		with open(found_file, "w") as opened_file:
			json.dump(file_json, opened_file, indent=2, sort_keys=True)

@task
def set_in_config(build, key, value):
	if isinstance(value, str):
		value = utils.render_string(build.config, value)

	build.log.debug("Setting {key} to {value} in app_config.json".format(key=key, value=value))

	key = key.split(".")
	last = key.pop()
	at = build.config
	for part in key:
		if not part in at or not isinstance(at[part], dict):
			at[part] = {}
		at = at[part]

	at[last] = value

@task
def add_to_json_array(build, filename, key, value):
	if isinstance(value, str):
		value = utils.render_string(build.config, value)
	
	build.log.debug("adding '{value}' to '{key}' in {files}".format(
		key=key, value=value, files=filename
	))
	
	found_files = glob.glob(filename)
	if len(found_files) == 0:
		build.log.warning('No files were found to match pattern "%s"' % filename)
	for found_file in found_files:
		file_json = {}
		with open(found_file, "r") as opened_file:
			file_json = json.load(opened_file)
			# TODO: . separated keys?
			file_json[key].append(value)
		with open(found_file, "w") as opened_file:
			json.dump(file_json, opened_file, indent=2, sort_keys=True)

@task
def resolve_urls(build, *url_locations):
	'''Include "src" prefix for relative URLs, e.g. ``file.html`` -> ``src/file.html``
	
	``url_locations`` uses::
	
	* dot-notation to descend into a dictionary
	* ``[]`` at the end of a field name to denote an array
	* ``*`` means all attributes on a dictionary
	'''
	def resolve_url_with_uuid(url):
		return utils._resolve_url(build.config, url, 'src')
	for location in url_locations:
		build.config = utils.transform(build.config, location, resolve_url_with_uuid)

@task
def wrap_activations(build, location):
	'''Wrap user activation code to prevent running in frames if required
	
	'''
	if "activations" in build.config['plugins'] and \
	   "config" in build.config['plugins']['activations'] and \
	   "activations" in build.config['plugins']['activations']['config']:
		for activation in build.config['plugins']['activations']['config']['activations']:
			if not 'all_frames' in activation or activation['all_frames'] is False:
				for script in activation['scripts']:
					tmp_file = uuid.uuid4().hex
					filename = location+script[3:]
					build.log.debug("wrapping activation {filename}".format(**locals()))
					in_file_contents = read_file_as_str(filename)
					in_file_contents = '// firefox complains when the first line is an if statement\n' + 'if (forge._disableFrames === undefined || window.location == window.parent.location) {\n' + in_file_contents + '\n}';
					with codecs.open(tmp_file, 'w', encoding='utf8') as out_file:
						out_file.write(in_file_contents)
					os.remove(filename)
					shutil.move(tmp_file, filename)
		
@task
def populate_icons(build, platform, icon_list):
	'''
	adds a platform's icons to a build config.
	platform is a string platform, eg. "chrome"
	icon_list is a list of string dimensions, eg. [36, 48, 72]
	'''
	if "icons" in build.config["plugins"] and "config" in build.config["plugins"]["icons"]:
		if not platform in build.config["plugins"]["icons"]["config"]:
			build.config["plugins"]["icons"]["config"][platform] = {}
		for icon in icon_list:
			str_icon = str(icon)
			if not str_icon in build.config["plugins"]["icons"]["config"][platform]:
				try:
					build.config["plugins"]["icons"]["config"][platform][str_icon] = \
						build.config["plugins"]["icons"]["config"][str_icon]
				except KeyError:
					build.log.warning('missing icon "%s" for platform "%s"' % (str_icon, platform))
	else:
		pass #no icons is valid, though it should have been caught priorly.

@task
def populate_xml_safe_name(build):
	build.config['xml_safe_name'] = build.config["name"].replace('"', '\\"').replace("'", "\\'")

@task
def populate_json_safe_name(build):
	build.config['json_safe_name'] = build.config["name"].replace('"', '\\"')

@task
def run_hook(build, **kw):
	for file in sorted(os.listdir(os.path.join('hooks', kw['hook']))):
		if os.path.isfile(os.path.join('hooks', kw['hook'], file)):
			cwd = os.getcwd()
			os.chdir(kw['dir'])
			
			target = iter(build.enabled_platforms).next()
			
			# Get the extension
			ext = os.path.splitext(file)[-1][1:]

			proc = None
			if ext == "py":
				build.log.info('Running (Python) hook: '+file)
				proc = lib.PopenWithoutNewConsole(["python", os.path.join(cwd, 'hooks', kw['hook'], file), target])
			elif ext == "js":
				build.log.info('Running (node) hook: '+file)
				proc = lib.PopenWithoutNewConsole(["node", os.path.join(cwd, 'hooks', kw['hook'], file), target])
			elif ext == "bat" and sys.platform.startswith('win'):
				build.log.info('Running (Windows Batch file) hook: '+file)
				proc = lib.PopenWithoutNewConsole([os.path.join(cwd, 'hooks', kw['hook'], file), target])
			elif ext == "sh" and not sys.platform.startswith('win'):
				build.log.info('Running (shell) hook: '+file)
				proc = lib.PopenWithoutNewConsole([os.path.join(cwd, 'hooks', kw['hook'], file), target])
			
			if proc != None:
				proc.wait()

			os.chdir(cwd)
			
			if proc != None and proc.returncode != 0:
				raise ConfigurationError('Hook script exited with a non-zero return code.')

@task
def remove_files(build, *removes):
	build.log.info('deleting %d files' % len(removes))
	for rem in removes:
		real_rem = utils.render_string(build.config, rem)
		build.log.debug('deleting %s' % real_rem)
		if path.isfile(real_rem):
			os.remove(real_rem)
		else:
			shutil.rmtree(real_rem, ignore_errors=True)

@task
def populate_package_names(build):
	build.config['package_name'] = re.sub("[^a-zA-Z0-9]", "", build.config["name"].lower()) + build.config["uuid"]
	if "core" not in build.config:
		build.config["core"] = {}
	if "firefox" not in build.config["core"]:
		build.config["core"]["firefox"] = {}
	build.config["core"]["firefox"]["package_name"] = firefox_tasks._generate_package_name(build)
	if "safari" not in build.config["core"]:
		build.config["core"]["safari"] = {}
	build.config["core"]["safari"]["package_name"] = safari_tasks._generate_package_name(build)
	if "ie" not in build.config["core"]:
		build.config["core"]["ie"] = {}
	build.config["core"]["ie"]["package_name"] = ie_tasks._generate_package_name(build)

@task	
def populate_trigger_domain(build):
	try:
		from forge import build_config
		config = build_config.load()
		build.config['trigger_domain'] = config['main']['server'][:-5]
	except ImportError:
		build.config['trigger_domain'] = "TRIGGER_DOMAIN_HERE"

	if not "config_hash" in build.config:
		build.config['config_hash'] = "CONFIG_HASH_HERE"

@task
def make_dir(build, dir):
	os.makedirs(dir)

@task
def generate_sha1_manifest(build, input_folder, output_file):
	with open(output_file, 'w') as out:
		manifest = dict()
		for root, dirs, files in os.walk(input_folder):
			for filename in files:
				filename = os.path.join(root, filename)
				with open(filename, 'rb') as file:
					hash = hashlib.sha1(file.read()).hexdigest()
					manifest[hash]  = filename[len(input_folder)+1:].replace('\\','/')
		json.dump(manifest, out)

@task
def check_index_html(build, src='src'):
	index_path = os.path.join(src, 'index.html')
	if not os.path.isfile(index_path):
		raise Exception("Missing index.html in source directory, index.html is required by Forge.")

	with open(index_path) as index_file:
		index_html = index_file.read()

		if index_html.find("<head>") == -1:
			raise Exception("index.html does not contain '<head>', this is required to add the Forge javascript library.")

@task
def run_plugin_build_steps(build, steps_path, src_path, project_path):
	# XXX: This should be using build_steps_local in plugin_dynamic but for now that code is duplicated here
	def copy_file_from_src(build_params, filename, dest):
		filename = pystache.render(filename, build_params['app_config'])
		dest = pystache.render(dest, build_params['app_config'])
		if os.path.isfile(os.path.join(src_path, filename)):
			shutil.copy2(os.path.join(src_path, filename), os.path.join(project_path, dest))

	if os.path.isdir(steps_path): 
		plugins = os.listdir(steps_path)
		for plugin in plugins:
			build.log.debug("Running local build steps for: %s" % plugin)
			with open(os.path.join(steps_path, plugin), 'r') as plugin_steps_file:
				plugin_steps = json.load(plugin_steps_file)
				for step in plugin_steps:
					if "do" in step:
						for task in step['do']:
							task_func = locals().get(task, None)
							if task_func is not None:
								# XXX: only supports dict of args, could be better?
								build_params = {}
								build_params['app_config'] = build.config
								build_params['project_path'] = project_path
								build_params['src_path'] = src_path
								task_func(build_params, **step["do"][task])
