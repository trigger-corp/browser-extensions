from os import path
import os
import logging

from lib import task

LOG = logging.getLogger(__name__)

def _chrome_folder():
	return path.abspath(path.join(os.getcwd(), 'development', 'chrome'))

@task
def run_chrome(build):
	msg = """Currently it is not possible to launch a Chrome extension via this interface.
The required steps are:

	1) Go to chrome:extensions in the Chrome browser
	2) Make sure "developer mode" is on (top right corner)')
	3) Use "Load unpacked extension" and select the folder: {chrome_folder}""".format(chrome_folder=_chrome_folder())

	LOG.info(msg)

@task
def package_chrome(build):
	msg = """Currently it is not possible to package a Chrome extension via this interface.
The required steps are:

	1) Go to chrome:extensions in the Chrome browser
	2) Make sure "developer mode" is on (top right corner)')
	3) Use "Pack extension" and use use this for the root: 
	   
        <app dir>/{chrome_folder}

More information on packaging Chrome extensions can be found here:
	http://legacy-docs.trigger.io/en/v1.4/best_practice/release_browser.html#chrome
	http://code.google.com/chrome/extensions/packaging.html
	""".format(chrome_folder="development/chrome")

	development_dir = path.join("development", "chrome")
	release_dir = path.join("release", "chrome")
	if not path.isdir(release_dir):
		os.makedirs(release_dir)

	LOG.info(msg)
