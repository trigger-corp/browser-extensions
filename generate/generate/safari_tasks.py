import logging
import os
from os import path

from lib import task

LOG = logging.getLogger(__name__)

def _generate_package_name(build):
	if "core" not in build.config:
		build.config["core"] = {}
	if "safari" not in build.config["core"]:
		build.config["core"]["safari"] = {}
	if "package_name" not in build.config["core"]["safari"]:
		build.config["core"]["safari"]["package_name"] = "forge.safari.{package_name}".format(package_name=build.config["package_name"])
	return build.config["core"]["safari"]["package_name"]

@task
def package_safari(build):
	msg = """Currently it is not possible to package a Safari extension via this interface.

More information on packaging Safari extensions can be found here:
	http://legacy-docs.trigger.io/en/v1.4/best_practice/release_browser.html#safari
	"""
	development_dir = path.join("development", "safari")
	release_dir = path.join("release", "safari")
	if not path.isdir(release_dir):
		os.makedirs(release_dir)

	LOG.info(msg)

@task
def run_safari(build):
	msg = """Currently it is not possible to launch a Safari extension via this interface."""
	
	LOG.info(msg)
