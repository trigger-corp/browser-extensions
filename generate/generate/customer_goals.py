'''Goals are a collection of phases which are automatically added to a build, then executed.

The idea here is for the calling code to not need to know about the right phases to include
when getting a higher-level "goal" done; e.g. running or generating an app.
'''
import platform
import sys
import os
import tempfile

from lib import BASE_EXCEPTION


def log_build(build, action):
	'''
	Bundle together some stats and send it to the server for tracking
	This is called by every other function in this module, just before running
	the build.
	'''
	from forge import build_config
	import forge

	log = {}
	log['action']	     = action
	log['platform']	     = platform.platform()
	log['version']	     = sys.version
	log['uuid']	     = build.config['uuid']
	log['tools_version'] = forge.VERSION
	config = build_config.load()

def generate_app_from_template(generate_module, build_to_run):
	'''Inject code into a previously built template.
	
	:param generate_module: the :mod:`generate.generate` module
	:param build_to_run: a :class:`build.Build` instance
	'''
	add_check_settings_steps(generate_module, build_to_run)
	build_to_run.add_steps(generate_module.customer_phases.migrate_to_plugins())
	build_to_run.add_steps(generate_module.customer_phases.resolve_urls())
	if os.path.isdir('hooks/prebuild'):
		# Create a temp dir
		tempdir = tempfile.mkdtemp()
		os.rmdir(tempdir)
		build_to_run.add_steps(generate_module.customer_phases.copy_user_source_to_tempdir(ignore_patterns=build_to_run.ignore_patterns, tempdir=tempdir))
		build_to_run.add_steps(generate_module.customer_phases.run_hook(hook='prebuild', dir=tempdir))
		build_to_run.add_steps(generate_module.customer_phases.validate_user_source(src=tempdir))
		build_to_run.add_steps(generate_module.customer_phases.copy_user_source_to_template(ignore_patterns=build_to_run.ignore_patterns, src=tempdir))
		# Delete temp dir
		build_to_run.add_steps(generate_module.customer_phases.delete_tempdir(tempdir=tempdir))
	else:
		build_to_run.add_steps(generate_module.customer_phases.validate_user_source())
		build_to_run.add_steps(generate_module.customer_phases.copy_user_source_to_template(ignore_patterns=build_to_run.ignore_patterns))
	build_to_run.add_steps(generate_module.customer_phases.include_platform_in_html())
	build_to_run.add_steps(generate_module.customer_phases.include_name())
	build_to_run.add_steps(generate_module.customer_phases.include_uuid())
	build_to_run.add_steps(generate_module.customer_phases.include_author())
	build_to_run.add_steps(generate_module.customer_phases.include_description())
	build_to_run.add_steps(generate_module.customer_phases.include_version())
	build_to_run.add_steps(generate_module.customer_phases.include_reload())
	build_to_run.add_steps(generate_module.customer_phases.include_requirements())
	build_to_run.add_steps(generate_module.customer_phases.run_plugin_build_steps(build_to_run))
	if (set(['ie', 'chrome', 'safari', 'firefox']) & set(build_to_run.enabled_platforms)):
		build_to_run.add_steps(generate_module.legacy_phases.customer_phase())
	build_to_run.add_steps(generate_module.customer_phases.include_config())
	build_to_run.add_steps(generate_module.customer_phases.make_installers())
	if os.path.isdir('hooks/postbuild'):
		build_to_run.add_steps(generate_module.customer_phases.run_hook(hook='postbuild', dir='development'))
	
	log_build(build_to_run, "generate")


	# -- TODO - mutating the build while adding phases is evil and should be reconsidered ----
	# XXX: Temporary server-side migration until we publicly deploy modules as plugins
	# for server-side analog, see main.py
	generate_module.customer_tasks.migrate_to_plugins(build_to_run)
	# -- TODO --------------------------------------------------------------------------------
	build_to_run.run()

def run_app(generate_module, build_to_run):
	'''Run a generated app on a device or emulator.
	
	:param generate_module: the :mod:`generate.generate` module
	:param build_to_run: a :class:`build.Build` instance
	'''

	if len(build_to_run.enabled_platforms) != 1:
		raise BASE_EXCEPTION("Expected to run exactly one platform at a time")

	build_to_run.add_steps(generate_module.customer_phases.migrate_to_plugins())

	target = list(build_to_run.enabled_platforms)[0]
	if target == 'firefox':
		build_to_run.add_steps(
			generate_module.customer_phases.run_firefox_phase(build_to_run.output_dir)
		)
	elif target == 'chrome':
		build_to_run.add_steps(
			generate_module.customer_phases.run_chrome_phase()
		)
	elif target == 'safari':
		build_to_run.add_steps(
			generate_module.customer_phases.run_safari_phase()
		)
	elif target == 'ie':
		build_to_run.add_steps(
			generate_module.customer_phases.run_ie_phase()
		)
	
	log_build(build_to_run, "run")
	build_to_run.run()

def package_app(generate_module, build_to_run):

	if len(build_to_run.enabled_platforms) != 1:
		raise BASE_EXCEPTION("Expected to package exactly one platform at a time")

	build_to_run.add_steps(generate_module.customer_phases.migrate_to_plugins())

	build_to_run.add_steps(
		generate_module.customer_phases.package(build_to_run.output_dir)
	)
	log_build(build_to_run, "package")
	build_to_run.run()

def add_check_settings_steps(generate_module, build_to_run):
	"""
	Required steps to sniff test the JavaScript and local configuration
	"""
	build_to_run.add_steps(generate_module.customer_phases.check_javascript())
	build_to_run.add_steps(generate_module.customer_phases.check_local_config_schema())

def check_settings(generate_module, build_to_run):
	"""
	Check the validity of locally configured settings.
	"""
	add_check_settings_steps(generate_module, build_to_run)

	build_to_run.run()

def add_migrate_app_steps(generate_module, build_to_run):
	"""
	Required steps to sniff test the JavaScript and local configuration
	"""
	build_to_run.add_steps(generate_module.customer_phases.migrate_config())

def migrate_app(generate_module, build_to_run):
	"""
	Migrate the app to the next major platform (if possible)
	"""
	add_migrate_app_steps(generate_module, build_to_run)

	build_to_run.run()

def cleanup_after_interrupted_run(generate_module, build_to_run):
	"""
	Cleanup after a run operation that was interrupted forcefully.

	This is exposed so the Trigger Toolkit can cleanup anything lingering from a run operation,
	e.g. node, adb, and any locks they have on files in the development folder
	"""
	build_to_run.add_steps(generate_module.customer_phases.clean_phase())
	build_to_run.run()
