"""Wrapper for WMGenerate that implements build-tools 'forge' command semantics"""

import argparse
import json
import logging
import os
from os import path
import shutil
import sys
import tempfile
import uuid
import zipfile
from StringIO import StringIO

project_dir = os.getcwd()
platform_dir = path.abspath(path.join(sys.path[0], '../..'))
config_dir = path.join(platform_dir, 'configuration')
sys.path.append(path.abspath(path.join(platform_dir, 'generate/build_tools')))

import forge
from forge import build_config, defaults, ForgeError
from forge.lib import cd, open_file, replace, InMemoryZip, template_file
import forge.cli as cli
from generate import main as forge_generate

LOG = logging.getLogger(__name__)
ERROR_LOG_FILE = os.path.join(os.getcwd(), 'forge-error.log')
ENTRY_POINT_NAME = 'forge'

def command_create():
	if os.path.exists(defaults.SRC_DIR):
		raise ForgeError('Source folder "%s" already exists, if you really want to create a new app you will need to remove it!' % defaults.SRC_DIR)

	questions = {
		'description': 'Enter details for app',
		'properties': {
			'name': {
				'type': 'string',
				'title': 'App Name',
				'description': 'This name is what your application will be called on devices. You can change it later through config.json.'
			}
		}
	}
	answers = cli.ask_question({ 'schema': questions })
	if 'name' in answers and answers['name']:
		forge.settings['name'] = answers['name']

	username = forge.settings['username']
	app_uuid = str(uuid.uuid4().hex)
	app_name = forge.settings['name']

	# generate app
	with open_file(path.join(config_dir, 'manifest.json')) as manifest_file:
		manifest = json.loads(manifest_file.read())
		zipf = InMemoryZip()
		for item in manifest:
			initial_file = template_file(config_dir, item["path"], item["template"],
				( ('${username}', username),
				  ('${uuid}', app_uuid),
				  ('${name}', app_name),
				  ('${platform_version}', forge.settings['LAST_STABLE']), ))
			zipf.writestr(item["path"], initial_file)
		with zipfile.ZipFile(StringIO(zipf.read())) as myzip:
			myzip.extractall()
	
	LOG.info('App structure created. To proceed:')
	LOG.info('1) Put your code in the "%s" folder' % defaults.SRC_DIR)
	LOG.info('2) Run %s build to make a build' % ENTRY_POINT_NAME)

def command_build(target=None):
	if target is None:
		LOG.error("Target required for 'forge-extension build', e.g. 'forge-extension build chrome'")
		return
	(config, config_tmp) = app_config()
	args = [
		'forge-generate', 'build',
		'--platforms', target,
		'-c', config_tmp,
		'-u', path.abspath('src'),
		'--remove_attribution', '1',
		'-r' # replace existing output directory if it already exists
	]
	if not forge.settings['development']:
		args.append('-p') # minify library files for customer distribution

	# safari output name differs from target
	if target == "safari":
		target = "forge.safariextension"	

	with cd(platform_dir):
		if path.exists(config['uuid']): # clean up any broken builds
			shutil.rmtree(config['uuid'])
		sys.argv = args
		forge_generate.main()
		if not path.exists(os.path.join(project_dir, 'development')):
			os.mkdir(os.path.join(project_dir, 'development'))
		if path.exists(os.path.join(project_dir, 'development', target)):
			shutil.rmtree(os.path.join(project_dir, 'development', target))
		shutil.move(path.join(config['uuid'], 'development', target),
					path.join(project_dir, 'development', target))
		shutil.rmtree(config['uuid'])
		os.remove(config_tmp)


def command_package(target=None):
	print "package"
	if target is None:
		LOG.error("Target required for 'forge-extension package', e.g. 'forge-extension build chrome'")
		return
	(config, config_tmp) = app_config()
	args = [
		'forge-generate', 'build', 'package',
		'--platforms', target,
		'-c', config_tmp,
		'-u', path.abspath('src'),
		'--remove_attribution', '1',
		'-r' # replace existing output directory if it already exists
	]
	if not forge.settings['development']:
		args.append('-p') # minify library files for customer distribution	

	with cd(platform_dir):
		if path.exists(config['uuid']): # clean up any broken builds
			shutil.rmtree(config['uuid'])
		sys.argv = args
		forge_generate.main()
		if not path.exists(os.path.join(project_dir, 'release')):
			os.mkdir(os.path.join(project_dir, 'release'))
		if path.exists(os.path.join(project_dir, 'release', target)):
			shutil.rmtree(os.path.join(project_dir, 'release', target))
		shutil.move(path.join(config['uuid'], 'release', target),
					path.join(project_dir, 'release', target))
		shutil.rmtree(config['uuid'])
		os.remove(config_tmp)

def command_run(target=None):
	import forge.build as forge_build
	import generate as generate_dynamic

	build_type_dir = 'development'

	build_to_run = forge_build.create_build(
		build_type_dir,
		targets=[target],
		generate_dynamic=generate_dynamic,
	)

	generate_dynamic.customer_goals.run_app(
		generate_module=generate_dynamic,
		build_to_run=build_to_run
	)

COMMANDS = {
	'create'  : command_create,
	'build'   : command_build,
	'package' : command_package,
	'run'     : command_run
}

class ArgumentError(Exception):
	pass

def app_config():
	# forge-generate expects uuid to be stored in config.json
	config = build_config.load_app()
	(fd, config_tmp) = tempfile.mkstemp(suffix=".json")
	os.close(fd)
	with open(config_tmp, 'w') as f:
		json.dump(config, f)
	return (config, config_tmp)

def setup_logging(settings):
	'Adjust logging parameters according to command line switches'
	verbose = settings.get('verbose')
	quiet = settings.get('quiet')
	if verbose and quiet:
		raise ArgumentError('Cannot run in quiet and verbose mode at the same time')
	if verbose:
		stdout_log_level = logging.DEBUG
	elif quiet:
		stdout_log_level = logging.WARNING
	else:
		stdout_log_level = logging.INFO

	logging.basicConfig(level=stdout_log_level, format='[%(levelname)7s] %(message)s')
	file_handler = logging.FileHandler(ERROR_LOG_FILE, delay=True)
	file_handler.setFormatter(logging.Formatter('%(asctime)s [%(levelname)7s] %(message)s'))
	file_handler.setLevel(logging.DEBUG)
	logging.root.addHandler(file_handler)
	LOG.info('Forge tools running at version %s' % forge.get_version())

def add_primary_options(parser):
	'''Top-level command-line arguments for settings which affect the running of
	any command for any platform
	'''
	parser.add_argument('command', choices=COMMANDS.keys())
	parser.add_argument('-v', '--verbose', action='store_true')
	parser.add_argument('-q', '--quiet', action='store_true')
	parser.add_argument('--username', help='your email address')
	parser.add_argument('-d', '--development', action='store_true', help="don't minify library files")

def handle_primary_options(args):
	'Parameterise our option based on common command-line arguments'
	parser = argparse.ArgumentParser(prog='forge-extension', add_help=False)
	add_primary_options(parser)

	handled_args, other_args = parser.parse_known_args(args)

	forge.settings['command'] = handled_args.command
	forge.settings['verbose'] = bool(handled_args.verbose)
	forge.settings['quiet'] = bool(handled_args.quiet)
	forge.settings['username'] = handled_args.username
	forge.settings['development'] = handled_args.development

	try:
		setup_logging(forge.settings)
	except ArgumentError as e:
		parser.error(e)

	return other_args

def main():
	forge.settings['commandline'] = True
	other_args = handle_primary_options(sys.argv[1:])	
	command = forge.settings['command']
	COMMANDS[command](*other_args)

if __name__ == '__main__':
	main()

