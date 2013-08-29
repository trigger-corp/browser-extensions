"""
See the bottom of this file for the command line arguments and starter.
The file "How to install and use.txt" might be of use.
"""
from __future__ import with_statement

import logging
import os
from os import path
import platform
import shutil
import sys
import tempfile

import argparse
try:
	import json
except ImportError:
	import simplejson as json
import validictory

from build import Build
import customer_phases, legacy_phases, server_phases, customer_tasks, server_tasks, predicates, legacy_predicates

log = None

class WMGenerator(object):
	def __init__(self, *args, **kw):
		'''THIS IS A FACADE.
		
		See :class:`generate.build.Build` for details of *args and **kw
		'''
		self.build = Build(*args, **kw)
	
	@property
	def unpackaged(self):
		'For legacy reasons: older code expects there to be a WMGenerator.unpackaged attribute'
		return self.build.unpackaged
	@property
	def packaged(self):
		'For legacy reasons: older code expects there to be a WMGenerator.packaged attribute'
		return self.build.packaged

	def run(self, script='unused argument'):
		self.build.add_steps(server_phases.prepare_config())
		self.build.add_steps(customer_phases.resolve_urls())
		self.build.add_steps(server_phases.copy_platform_source())
		if self.build.override_plugins:
			self.build.add_steps(server_phases.override_plugins())
		self.build.add_steps(server_phases.sensible_default_for_toolbar())
		self.build.add_steps(server_phases.copy_common_files())
		self.build.add_steps(server_phases.pre_create_all_js())
		if (set(['ie', 'chrome', 'safari', 'firefox']) & set(self.build.enabled_platforms)):
			self.build.add_steps(legacy_phases.create_all_js())
		self.build.add_steps(server_phases.post_create_all_js())
		self.build.add_steps(server_phases.remove_assets_forge())
		# -- TODO - mutating the build while adding phases is evil and should be reconsidered ----
		# XXX: Temporary server-side migration until we publicly deploy modules as plugins
		customer_tasks.migrate_to_plugins(self.build)
		self.build.add_steps(server_phases.platform_specific_templating(self.build))
		if (set(['ie', 'chrome', 'safari', 'firefox']) & set(self.build.enabled_platforms)):
			self.build.add_steps(legacy_phases.platform_specific_templating(self.build))
		# -- TODO --------------------------------------------------------------------------------
		self.build.add_steps(server_phases.add_plugins())
		self.build.add_steps(server_phases.minification())
		if getattr(self.build, "debug", False):
			self.build.add_steps(server_phases.copy_customer_source())
			self.build.add_steps(customer_phases.validate_user_source())
			self.build.add_steps(customer_phases.copy_user_source_to_template(debug=True))
			self.build.add_steps(customer_phases.include_platform_in_html(debug=True))
			self.build.add_steps(customer_phases.include_config(debug=True))
			self.build.add_steps(server_phases.handle_debug_output())
		else:
			self.build.add_steps(server_phases.platform_specific_build())
			self.build.add_steps(server_phases.handle_template_output())
			if not self.build.template_only:
				# TODO should this branch be handled by a predicate?
				self.build.add_steps(server_phases.copy_customer_source())
				self.build.add_steps(customer_phases.validate_user_source())
				self.build.add_steps(customer_phases.copy_user_source_to_template())
				self.build.add_steps(customer_phases.include_platform_in_html())
				self.build.add_steps(customer_phases.include_name())
				self.build.add_steps(customer_phases.include_uuid())
				self.build.add_steps(customer_phases.include_author())
				self.build.add_steps(customer_phases.include_description())
				self.build.add_steps(customer_phases.include_version())
				self.build.add_steps(customer_phases.include_requirements())
				self.build.add_steps(customer_phases.include_config())
				if (set(['ie', 'chrome', 'safari', 'firefox']) & set(self.build.enabled_platforms)):
					self.build.add_steps(legacy_phases.customer_phase())
				self.build.add_steps(customer_phases.run_plugin_build_steps(self.build))
				self.build.add_steps(customer_phases.make_installers())
			if getattr(self.build, "package", False):
				# TODO should this branch be handled by a predicate?
				self.build.log.info("we will be doing packaging too")
				self.build.add_steps(
					server_phases.copy_lib_files_to_template(self.build.source_dir)
				)
				self.build.add_steps(customer_phases.package(self.build.output_dir))

			self.build.add_steps(server_phases.handle_output())

		orig_dir = os.getcwd()
		temp_d = tempfile.mkdtemp()
		try:
			os.chdir(temp_d)
			self.build.run()
		finally:
			os.chdir(orig_dir)
			shutil.rmtree(temp_d, ignore_errors=True)
		
	def __repr__(self):
		return '<Build ({0})>'.format(', '.join(self.build.enabled_platforms))

def validate_schema(schema_file, config):
	with open(schema_file) as schema_f:
		schema = json.load(schema_f)
	
	try:
		validictory.validate(config, schema)
	except validictory.validator.UnexpectedPropertyError as e:
		log.warning(e)

def main():
	global log
	in_default = os.getcwd()
	usercode_default = 'user'
	out_default = None
	key_default = None
	configfile_default = 'config.json'
	schema_default = path.join('configuration', 'schema.json')

	# Every system should be able to build the browsers
	platform_default = ['firefox', 'chrome', 'safari', 'ie']

	parser = argparse.ArgumentParser(description='Generate a new Forge app')
	parser.add_argument('sub_command', nargs='+', choices=("build", "package", "debug"))
	parser.add_argument('-s', '--source', help='directory containing add-on code (default %s)' % in_default,
		default=in_default)
	parser.add_argument('-o', '--output', help='directory to write new add-on to (default %s)' % out_default,
		default=out_default)
	parser.add_argument('-u', '--usercode', help='directory holding the user\'s code (default %s)' % usercode_default, default=usercode_default)
	parser.add_argument('-t', '--template',
		help='only create templated output, not using or including the user code',
		action='store_const', const=True, default=False)
	parser.add_argument('-k', '--key', help='path to .pem key file for Chrome packaging (default %s)' % key_default,
		default=key_default)
	parser.add_argument('-c', '--config', required=False,
		help='configuration file for this extension (default %s)' % path.join(usercode_default, configfile_default))
	parser.add_argument('--config_schema', required=False,
		help='Validictory schema specification to check the configuration file against (default %s)' % schema_default)
	parser.add_argument('--system_config', help='configuration file for this system. defaults to system_config.json. see generate/example_system_config.json', required=False, default='system_config.json')
	parser.add_argument('--remove_attribution', help='flag to remove "(built on Forge)" from description', required=False, default=False)
	parser.add_argument('-v', '--verbose', help='produce more logging output',
		action='store_const', const=True, default=False)
	parser.add_argument('-r', '--remove', help='replace existing output directory if it already exists',
		action='store_const', const=True, default=False)
	parser.add_argument('-p', '--production', help='minify library files for customer distribution',
		action='store_const', const=True, default=False)
	parser.add_argument('--test', help='Set the UUID to the changeset hash for testing',
		action='store_const', const=True, default=False)
	parser.add_argument('--platforms', help='list of platforms to build for (default "%s")' % ' '.join(platform_default),
		default=platform_default, nargs='+')
	parser.add_argument('-z', '--override_plugins', help='Override build plugins with given folder (probably "plugins")', default=False)

	args = parser.parse_args()
	args.source = path.abspath(args.source)

	level = logging.DEBUG if args.verbose else logging.INFO
	logging.basicConfig(level=level, format='[%(levelname)7s] %(asctime)s -- %(message)s')
	log = logging.getLogger(__name__)

	# make sure relative usercode directories still work
	usercode = path.abspath(args.usercode)
	
	# Look for config.json in usercode or as a command arg.
	if args.config and path.isfile(args.config):
		user_config_file = path.abspath(args.config)
	elif path.isfile(path.join(usercode, configfile_default)):
		user_config_file = path.abspath(path.join(usercode, configfile_default))
	else:
		parser.error("User config file must exist")

	# Load config.json
	with open(user_config_file) as opened_user_config_file:
		loaded_config = json.load(opened_user_config_file)
		log.debug('read configuration file from %s' % user_config_file)
	if len(loaded_config) == 0:
		parser.error("No config read from %s" % user_config_file)

	# Read the (optional) system config and merge it with the user config.
	if path.isfile(args.system_config):
		with open(args.system_config) as system_conf_file:
			system_config = json.load(system_conf_file)
			log.debug('read system config file from %s' % args.system_config)
		if len(system_config) == 0:
			log.error("no config options read from the system config file")
		loaded_config.update(system_config)

	# validate config
	if args.config_schema:
		config_schema = args.config_schema
	else:
		config_schema = path.join(args.source, schema_default)
	validate_schema(config_schema, loaded_config)

	# Ensure required system configs (if any) are present.
	log.info('processing "%s"' % loaded_config['name'])

	# If output is not set, default to a directory named after UUID.
	output = args.output
	if output is None:
		output = path.abspath(loaded_config['uuid'])

	if args.remove and path.exists(output):
		shutil.rmtree(output, ignore_errors=False)

	local_config = {}
	local_config_path = path.abspath(path.join(usercode, "..", "local_config.json"))
	if path.isfile(local_config_path):
		with open(local_config_path) as local_config_file:
			local_config = json.load(local_config_file)

	generator = WMGenerator(loaded_config, args.source, output,
		args.production, args.remove_attribution, usercode,
		enabled_platforms=args.platforms, template_only=args.template,
		test=args.test, local_config=local_config, override_plugins=args.override_plugins)
	if 'debug' in args.sub_command:
		generator.build.debug = True
	elif 'build' not in args.sub_command:
		parser.error("We don't currently support sub_commands other than debug "
			"without building first, e.g. {script} build package"
			.format(script=sys.argv[0]))
	elif 'package' in args.sub_command:
		generator.build.package = True
	
	return generator.run()

if __name__ == '__main__':
	main()
