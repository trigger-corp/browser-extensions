import logging
import os
from os import path
import sys

from forge import build_config, defaults, ForgeError, lib

LOG = logging.getLogger(__name__)

def _enabled_platforms(build_type_dir):
	'''Return a list of target names we know how to build.

	This method used to return a list of the platforms enabled, based on
	directories in development.
	'''
	return ['safari', 'firefox', 'chrome', 'ie']

def import_generate_dynamic(do_reload=False):
	"""Load the dynamically-fetched generate libs.

	*N.B*. this has the side effect of checking to see if the app
	needs to be migrated in some way to be useable with this platform version.

	:param do_reload: call reload() on generate_dynamic modules if they're already imported.
	"""
	try:
		import generate_dynamic
		if do_reload:
			# need to do build and lib first so we can use @task
			reload(sys.modules['generate_dynamic.build'])
			reload(sys.modules['generate_dynamic.lib'])
			# ... and not reload them twice
			for name, module in sys.modules.items():
				if module and \
						name.startswith('generate_dynamic') and \
						name != 'generate_dynamic.build' and \
						name != 'generate_dynamic.lib':
					reload(module)
	except ImportError:
		sys.path.insert(0, path.abspath('.template'))
		sys.path.insert(0, path.abspath(path.join('.template', 'lib')))
		try:
			import generate_dynamic
		except ImportError as e:
			raise ForgeError("Couldn't import generation code: {0}".format(e))
	
	# make sure app is compatible with generate_dynamic logic
	if hasattr(generate_dynamic.customer_goals, 'migrate_app_to_current'):
		generate_dynamic.customer_goals.migrate_app_to_current(path=os.getcwd())

	return generate_dynamic

def _get_ignore_patterns_for_src(src_dir):
	"""Returns the set of match_patterns
	:param src_dir: Relative path to src directory containing user's code
	"""

	try:
		with lib.open_file(os.path.join(src_dir, '.forgeignore')) as ignore_file:
			ignores = map(lambda s: s.strip(), ignore_file.readlines())
	except Exception:
		ignores = []

	return list(set(ignores))

def create_build(build_type_dir, config=None, targets=None, extra_args=None, generate_dynamic=None):
	'''Helper to instantiate a Build object from the dynamic generate code
	
	Assumes the working directory is alongside src and development
	
	:param build_type_dir: currently always "development"
	:param targets: the targets this build will concern itself with;
		a value of `None` signifies all targets
	:type targets: iterable
	:param extra_args: command line arguments that haven't been consumed yet
	:type extra_args: sequence
	:param generate_dynamic: generate_dynamic module to get Build class from
	'''
	if generate_dynamic is None:
		# prevent cyclic recursion with import_generate_dynamic
		generate_dynamic = import_generate_dynamic()

	if config is None:
		app_config = build_config.load_app()
	else:
		app_config = config

	local_config = build_config.load_local()
	extra_args = [] if extra_args is None else extra_args
	ignore_patterns = _get_ignore_patterns_for_src(defaults.SRC_DIR)
	enabled_platforms = _enabled_platforms(build_type_dir)
	if targets is not None:
		enabled_platforms = set(enabled_platforms) & set(targets)
	
	build_to_run = generate_dynamic.build.Build(
		app_config,
		defaults.SRC_DIR, # confusingly this is meant to point to platform source code... this value is not used by the customer tasks.
		build_type_dir, # where the output of build should go
		enabled_platforms=enabled_platforms,
		ignore_patterns=ignore_patterns,
		local_config=local_config,
		extra_args=extra_args,
		forge_root=defaults.FORGE_ROOT
	)

	return build_to_run
