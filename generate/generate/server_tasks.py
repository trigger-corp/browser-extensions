import glob
import json
import os
from os import path
import re
import sys
import shutil
import subprocess
from subprocess import PIPE, STDOUT
import zipfile
import uuid
import urllib
import time
import tempfile

from genshi.template import NewTextTemplate, TemplateLoader
import validictory
import pystache

import firefox_tasks
import safari_tasks
import ie_tasks
import utils
from build import ConfigurationError
from lib import task
import minify
from customer_tasks import add_element_to_xml, add_to_json_array

_platform_dir_map = {
	'ie': 'development/ie',
	'safari': 'development/forge.safariextension',
	'firefox': 'development/firefox',
	'chrome': 'development/chrome',
}

def _call_with_params(method, build_params, params):
	if isinstance(params, dict):
		return method(build_params, **params)
	elif isinstance(params, tuple):
		return method(build_params, *params)
	else:
		return method(build_params, params)

@task
def preprocess_config(build):
	'''
	preprocess config
	'''
	build.log.info('preprocessing configuration')

	# Add a UUID to config.json if not already got one.
	if 'uuid' not in build.config:
		build.config['uuid'] = str(uuid.uuid4())
		build.log.warning('generated new UUID: %s' % build.config['uuid'])
		
	if build.test:
		ant = subprocess.Popen(['hg', 'id', '-i', build.source_dir], stdout=PIPE, stderr=STDOUT)
		cshash = ant.communicate()[0].replace('+','').strip()
		build.log.warning('new UUID: %s' % cshash)
		build.config['uuid'] = cshash

	# Encode all configuration keys/values as UTF-8.
	build.config = dict((k.encode('utf8'), v) for k, v in build.config.iteritems())

	desc = build.config.get('description', '')
	if not desc.endswith('(built on Forge)') and not build.remove_attribution:
		build.config['description'] = desc + ' (built on Forge)'

	# Package name is like "foobar" from "Foo Bar!"
	build.config['package_name'] = re.sub("[^a-zA-Z0-9]", "", build.config["name"].lower()) + build.config["uuid"]
	if build.test:
		build.config['package_name'] = re.sub("[^a-zA-Z0-9]", "", build.config["name"].lower())

	build.config["core"]["firefox"]["package_name"] = firefox_tasks._generate_package_name(build)
	build.config["core"]["safari"]["package_name"] = safari_tasks._generate_package_name(build)
	build.config["core"]["ie"]["package_name"] = ie_tasks._generate_package_name(build)

	build.config['xml_safe_name'] = build.config["name"].replace('"', '\\"').replace("'", "\\'")

	if "logging" in build.config and "level" in build.config["logging"]:
		build.config["logging_level"] = build.config["logging"]["level"]
	else:
		build.config["logging_level"] = "INFO"

	build.config['json'] = json
	
	
@task
def need_https_access(build):
	'If any activation has a HTTPS prefix, set the "activate_on_https" config flag'
	def pattern_needs_https(pat):
		return (
			pat == "<all_urls>" or
			pat.startswith("*") or
			pat.startswith("https")
		)
	def activation_needs_https(act):
		return any(pattern_needs_https(pat) for pat in act.get("patterns", []))
	
	build.config["activate_on_https"] = any(
		pattern_needs_https(pat) for pat in build.config["plugins"]
			.get("request", {})
			.get("config", {})
			.get("permissions", [])
	) or any(
		activation_needs_https(act) for act in build.config["plugins"] 
			.get("activations", {}).get("config", {}).get("activations", [])
	)

@task
def minify_in_place(build, *files):
	'''Minify a JS or CSS file, without renaming it.
	'''
	real_files = [utils.render_string(build.config, f) for f in files]
	minify.minify_in_place(build.source_dir, *real_files)

@task
def addon_source(build, *directories):
	for d in directories:
		from_to = (path.join(build.source_dir, d), d)
		build.log.info('copying source directory %s to %s' % from_to)
		shutil.copytree(*from_to, symlinks=True)

@task
def user_source(build, directory):
	from_to = (build.usercode, directory)
	build.log.debug('copying source directory "%s" to "%s"' % from_to)
	shutil.copytree(*from_to)

@task
def concatenate_files(build, **kw):
	if 'in' not in kw or 'out' not in kw:
		raise ConfigurationError('concatentate_files requires "in" and "out" keyword arguments')

	with open(kw['out'], 'a') as out_file:
		for frm in kw['in']:
			if not path.isfile(frm):
				raise Exception("not a file: " + frm)
			build.log.debug('concatenating %s to %s' % (frm, kw['out']))
			with open(frm) as in_file:
				out_file.write(in_file.read())
			build.log.info('appended %s to %s' % (frm, kw['out'],))
	
@task
def add_to_all_js(build, file):
	all_js_paths = {
		'ie': ('ie/forge/all.js', 'ie/forge/all-priv.js',),
		'safari': ('forge.safariextension/forge/all.js', 'forge.safariextension/forge/all-priv.js',),
		'firefox': ('firefox/template-app/data/forge/all.js',),
		'chrome': ('chrome/forge/all.js', 'chrome/forge/all-priv.js',),
	}
	for platform in build.enabled_platforms:
		if not platform in all_js_paths:
			continue
		for out in all_js_paths[platform]:
			kw = {
				'in': (file,),
				'out': out
			}
			concatenate_files(build, **kw)

@task
def extract_files(build, **kw):
	if 'from' not in kw or 'to' not in kw:
		raise ConfigurationError('extract_files requires "from" and "to" keyword arguments')

	build.log.debug('Extracting %s to %s' % (utils.render_string(build.config,kw['from']), utils.render_string(build.config, kw['to'])))
	zipf = zipfile.ZipFile(utils.render_string(build.config, kw['from']))
	zipf.extractall(utils.render_string(build.config, kw['to']))
	zipf.close()

@task
def fallback_to_default_toolbar_icon(build):
	if "button" not in build.config["plugins"] or "config" not in  build.config["plugins"]["button"] or "default_icon" in build.config["plugins"]["button"]["config"]:
		# don't need to worry about toolbar icons
		return
	# no default icon given in browser_action section
	build.log.debug("moving default toolbar icon into place")
	shutil.copytree("common-v2/graphics", "common-v2/forge/graphics")
	
	build.log.debug("settings browser_action.default_icon")
	build.config["plugins"]["button"]["config"]["default_icon"] = "forge/graphics/icon16.png"

@task
def template_files(build, *files):
	'''apply genshi templating to files in place'''
	build.log.info('applying templates to %d files' % (len(files)))

	for glob_str in files:
		found_files = glob.glob(glob_str)
		if len(found_files) == 0:
			build.log.warning('No files were found to match pattern "%s"' % glob_str)
		for _file in found_files:
			build.log.debug('templating %s' % _file)
			loader = TemplateLoader(path.dirname(_file))
			tmpl = loader.load(path.basename(_file), cls=NewTextTemplate)
			stream = tmpl.generate(**build.config)
			tmp_file = _file+'-tmp'
			with open(tmp_file, 'w') as out_file:
				out_file.write(stream.render('text'))
			from_to = (tmp_file, _file)
			shutil.move(*from_to)

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
def move_output(build, *output_dirs):
	# XXX this is gross: need to move Firefox output to specially named
	# directory so that it can be installed directly with no prompting
	# problems: we rely on running this method before
	# remember_build_output_location, and firefox's real output location
	# is implicitly stored in _platform_dir_map
	# apologies to whoever this bites.
	if build.test and \
			"development" in output_dirs and \
			"firefox" in build.enabled_platforms:
		from_ = path.join("development", "firefox")
		to = path.join(from_, build.config["uuid"]+"@jetpack")
		build.log.debug("as test build, moving {0} to {1}".format(from_, to))
		shutil.move(from_, to)
		_platform_dir_map["firefox"] = to
	for output_dir in output_dirs:
		to = path.join(build.output_dir, output_dir)
		build.log.debug("moving {0} to {1}".format(output_dir, to))
		shutil.move(output_dir, to)

@task
def move_debug_output(build, platform):
	to = path.join(build.output_dir, 'debug')
	build.log.debug("moving {0} to {1}".format(platform, to))
	shutil.move(platform, to)

@task
def remember_build_output_location(build):
	for platform in build.enabled_platforms:
		# build.unpackaged is used to locate output after build is complete,
		# when running embedded
		build.unpackaged[platform] = _platform_dir_map[platform]

@task
def ant_build(build, new_working_dir, scheme='partial'):
	original_dir = os.getcwd()

	try:
		build.log.debug('changing dir to do Ant: %s, was in %s' % (new_working_dir, original_dir))
		os.chdir(new_working_dir)
		if sys.platform.startswith('win'):
			ant = subprocess.Popen(['ant.bat', scheme], stdout=PIPE, stderr=STDOUT)
		else:
			ant = subprocess.Popen(['ant', scheme], stdout=PIPE, stderr=STDOUT)
		out = ant.communicate()[0]
		if ant.returncode != 0:
			build.log.error('ant build error: %s' % out)
			raise Exception('ant build error')
		else:
			build.log.debug('ant build output: %s' % out)
	finally:
		os.chdir(original_dir)

@task
def cfx_build(build, source_dir):
	original_dir = os.getcwd()
	try:
		os.chdir(source_dir)

		cmd = ["python", path.join(build.source_dir, 'firefox', 'addon-sdk', 'bin', 'cfx'), "xpi"]
		try:
			update_url = build.config["plugins"]["update_url"]["config"]["firefox"]
			cmd += ["--update-url", update_url]
		except (KeyError, TypeError):
			# no FF update URL defined
			pass
		
		build.log.debug('running cfx command: "%s"' % (cmd))
		cfx = subprocess.Popen(cmd, stdout=PIPE, stderr=STDOUT)
		out = cfx.communicate()[0]
		if cfx.returncode != 0:
			build.log.error('cfx error: %s' % out)
			raise Exception('cfx error')
		else:
			build.log.debug('cfx output: %s' % out)
	finally:
		os.chdir(original_dir)

@task
def copy_jquery(build, **kw):
	if 'to' not in kw:
		raise ConfigurationError('copy_jquery needs "to" keyword arguments')

	_from = 'common-v2/libs/jquery/jquery-' + build.config.get('plugins')['jquery']['config']['version'] + '.min.js'
	_to = utils.render_string(build.config, kw['to'])

	dir = ''
	for next_dir in _to.split('/'):
		dir += next_dir + '/'
		if not os.path.isdir(dir):
			os.mkdir(dir)

	shutil.copy(_from, _to)

def _download_and_extract_plugin(build, name, hash):
	if os.path.exists(os.path.join('plugins', name)):
		build.log.debug("Plugin already downloaded: %s" % name)
		return
	build.log.debug("Downloading plugin id: %s" % hash)

	count = 0
	while count < 5:
		try:
			urllib.urlretrieve(build.config['plugin_url_prefix']+hash, 'plugins/'+hash)
			count = 99
		except Exception as e:
			# Retry a few times after a short sleep
			time.sleep(3)
			count = count + 1
			if count == 5:
				raise e
	
	zipf = zipfile.ZipFile('plugins/'+hash)
	zipf.extractall(os.path.join('plugins', name))
	zipf.close()
	os.remove('plugins/'+hash)

@task
def download_and_extract_plugins(build):
	'''Download and extract any plugins to be included in this build, a plugin named alert will be extracted to plugins/alert'''
	build.log.info("Downloading plugins")

	if 'plugins' in build.config:
		if not 'plugin_url_prefix' in build.config:
			raise Exception('"plugin_url_prefix" must be configured in system_config to use plugins')

		try:
			os.makedirs('plugins')
		except OSError:
			pass

		for plugin in build.config['plugins']:
			build.log.info("Downloading plugin: %s" % plugin)
			file = build.config['plugins'][plugin]['hash']
			_download_and_extract_plugin(build, plugin, file)
			build.log.info("Downloaded plugin: %s" % plugin)

@task
def include_dependencies(build, **kw):
	for plugin, properties in kw.items():
		_download_and_extract_plugin(build, plugin, properties['hash'])

@task
def prepare_plugin_override(build):
	from_to = (os.path.abspath(os.path.join(build.source_dir, build.override_plugins)), 'plugins')
	build.log.info('copying source directory %s to %s' % from_to)
	shutil.copytree(*from_to, symlinks=True)

	plugins = os.listdir('plugins')
	for plugin in plugins:
		if plugin.startswith('.'):
			continue

		if plugin not in build.config['plugins']:
			shutil.rmtree(os.path.join('plugins', plugin))
			continue

		for path in os.listdir(os.path.join('plugins', plugin)):
			if path == 'plugin' or path.startswith('.'):
				continue
			shutil.rmtree(os.path.join('plugins', plugin, path))

		for path in os.listdir(os.path.join('plugins', plugin, 'plugin')):
			if path.startswith('.'):
				continue
			os.rename(os.path.join('plugins', plugin, 'plugin', path), os.path.join('plugins', plugin, path))
