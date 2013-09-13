import os
from os import path
import shutil
import logging
import sys
import zipfile

from lib import task, cd, filter_filenames, filter_dirnames
from utils import run_shell, ensure_lib_available


LOG = logging.getLogger(__name__)


def _clean_firefox(build_type_dir):
	original_harness_options = os.path.join(build_type_dir, 'firefox', 'harness-options.json')
	backup_harness_options = os.path.join(build_type_dir, 'firefox', 'harness-options-bak.json')
	LOG.debug('Cleaning up after firefox run')
	if os.path.isfile(backup_harness_options):
		shutil.move(backup_harness_options, original_harness_options)


@task
def clean_firefox(build, build_type_dir):
	_clean_firefox(build_type_dir)


def _run_python_code(build, extra_path, entry_point):
	python_runner = ensure_lib_available(build, 'python_runner.py')

	if sys.platform.startswith("win"):
		runner = ensure_lib_available(build, 'python_runner_win.exe')
		run_shell(runner, extra_path, entry_point, command_log_level=logging.INFO, check_for_interrupt=True)
	elif sys.platform.startswith("darwin"):
		runner = ensure_lib_available(build, 'python_runner_darwin')
		run_shell(runner, extra_path, entry_point, command_log_level=logging.INFO, check_for_interrupt=True, create_process_group=True)
	else:
		python = sys.executable
		run_shell(python, python_runner, extra_path, entry_point, command_log_level=logging.INFO, check_for_interrupt=True)


@task
def run_firefox(build, build_type_dir):
	firefox_lib = ensure_lib_available(build, 'run-firefox.zip')

	try:
		_run_python_code(build, firefox_lib, 'firefox_runner.run')
	finally:
		_clean_firefox(build_type_dir)


def _generate_package_name(build):
	if "core" not in build.config:
		build.config["core"] = {}
	if "firefox" not in build.config["core"]:
		build.config["core"]["firefox"] = {}
	if "package_name" not in build.config["core"]["firefox"]:
		build.config["core"]["firefox"]["package_name"] = build.config["uuid"]
	return build.config["core"]["firefox"]["package_name"]

@task
def package_firefox(build):
	development_dir = path.join('development', 'firefox')
	release_dir = path.join('release', 'firefox')
	if not path.isdir(release_dir):
		os.makedirs(release_dir)

	xpi_filename = '{name}.xpi'.format(name=build.config['xml_safe_name'])
	IGNORED_FILES = ['.hgignore', '.DS_Store', 'install.rdf',
					 'application.ini', xpi_filename]

	with cd(development_dir):
		zipf = zipfile.ZipFile(xpi_filename, 'w', zipfile.ZIP_DEFLATED)
		for dirpath, dirnames, filenames in os.walk('.'):
			filenames = list(filter_filenames(filenames, IGNORED_FILES))
			dirnames[:] = filter_dirnames(dirnames)
			for filename in filenames:
				abspath = os.path.join(dirpath, filename)
				LOG.info('Adding: %s' % abspath)
				zipf.write(abspath)
		zipf.close()
	shutil.move(path.join(development_dir, xpi_filename), path.join(release_dir, xpi_filename))
