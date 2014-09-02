from os import path
import os
import sys
import logging
from subprocess import PIPE, STDOUT, Popen
import shutil
import zipfile
import struct

import lib
from lib import task, cd, filter_filenames, filter_dirnames, CouldNotLocate

LOG = logging.getLogger(__name__)

class ChromeError(Exception):
	pass

def _chrome_folder():
	return path.abspath(path.join(os.getcwd(), 'development', 'chrome'))


def _windows_chrome_cmd():
	if not sys.platform.startswith('win32'):
		return None
	suffix = 'Google\\Chrome\\Application\\chrome.exe';
	for envvar in ['LOCALAPPDATA', 'PROGRAMFILES', 'PROGRAMFILES(X86)']:
		prefix = os.environ.get(envvar)
		if prefix:
			cmd = path.join(prefix, suffix)
			if path.isfile(cmd):
				return cmd
	return None


@task
def run_chrome(build):
	chrome_commands = {
		'linux': 'google-chrome',
		'darwin': '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
		'win32': _windows_chrome_cmd()
	}

	chrome_cmd = None
	for platform, cmd in chrome_commands.items():
		if sys.platform.startswith(platform) and cmd:
			chrome_cmd = cmd
			break

	if not chrome_cmd:
		msg = """Currently it is not possible to launch a Chrome extension for your platform
with this interface. The required steps are:

	1) Go to chrome:extensions in the Chrome browser
	2) Make sure "developer mode" is on (top right corner)')
	3) Use "Load unpacked extension" and select the folder: {chrome_folder}
""".format(chrome_folder=_chrome_folder())
		LOG.info(msg)
		return

	with lib.temp_dir() as user_data_dir:
		LOG.info("Chome user data directory: {}".format(user_data_dir))
		development_dir = path.realpath(path.join("development", "chrome"))
		LOG.info("Extension will be loaded from: {}".format(development_dir))
		chrome = Popen([chrome_cmd,
						'--args',
						'--load-extension={}'.format(development_dir),
						'--user-data-dir={}'.format(user_data_dir),
						'--no-default-browser-check',
						'--no-first-run'])
		try:
			chrome.wait()
		except KeyboardInterrupt:
			# Kill Chrome and wait until it's gone before erasing the temporary user data dir.
			chrome.terminate()
			chrome.wait()


@task
def package_chrome(build):
	development_dir = path.join("development", "chrome")
	release_dir = path.join("release", "chrome")
	if not path.isdir(release_dir):
		os.makedirs(release_dir)

	# Make sure we have openssl otherwise show manual instructions
	openssl_check = lib.PopenWithoutNewConsole('openssl version', shell=True, stdout=PIPE, stderr=STDOUT)
	if openssl_check.wait() != 0:
		msg = """Packaging a Chrome extensions requires openssl.
Alternatively you can manually package it. The required steps are:

	1) Go to chrome:extensions in the Chrome browser
	2) Make sure "developer mode" is on (top right corner)')
	3) Use "Pack extension" and use use this for the root: 
	   
        <app dir>/{chrome_folder}

More information on packaging Chrome extensions can be found here:
	http://legacy-docs.trigger.io/en/v1.4/best_practice/release_browser.html#chrome
	http://code.google.com/chrome/extensions/packaging.html
""".format(chrome_folder="development/chrome")
		LOG.info(msg)
		raise CouldNotLocate("Make sure 'openssl' is in your path")

	# We need a private key to package chrome extensions
	crx_key = build.tool_config.get('chrome.profile.crx_key')
	crx_key_path = build.tool_config.get('chrome.profile.crx_key_path')

	if not crx_key:
		key_msg = """Packaging a Chrome extension requires a private key.
You can generate this key with openssl:

    openssl genrsa -out crxkey.pem 2048

Keep this key safe and secure. It is needed to sign all future versions
of the extension. Add the following to <app dir>/local_config.json to use the key:

    "chrome": {
        "profile": {
            "crx_key": "crxkey.pem",
            "crx_key_path": "<path to crxkey.pem>"
        }
    }
"""
		LOG.info(key_msg)
		return

	# HACK: build.usercode seems to be the only way to get a reference to the app directory.
	crx_key_file = path.realpath(path.join(build.usercode, '..', crx_key_path, crx_key))
	crx_filename = '{name}.crx'.format(name=build.config['xml_safe_name'])
	IGNORED_FILES = ['.hgignore', '.DS_Store',
					 'application.ini', crx_filename]

	# Create standard zip file
	with cd(development_dir):
		zipf = zipfile.ZipFile(crx_filename, 'w', zipfile.ZIP_DEFLATED)
		for dirpath, dirnames, filenames in os.walk('.'):
			filenames = list(filter_filenames(filenames, IGNORED_FILES))
			dirnames[:] = filter_dirnames(dirnames)
			for filename in filenames:
				abspath = os.path.join(dirpath, filename)
				LOG.info('Adding: %s' % abspath)
				zipf.write(abspath)
		zipf.close()

	# Generate signature
	signature, pubkey = _generate_signature(path.join(development_dir, crx_filename), crx_key_file)

	# Combine magic, public key and signature into header and prepend to zip file
	magic = 'Cr24'
	version = struct.pack('<I', 2)
	pubkey_len = struct.pack('<I', len(pubkey))
	signature_len = struct.pack('<I', len(signature))

	with open(path.join(development_dir, crx_filename), 'rb') as crx:
		zip_data = crx.read()

	with open(path.join(development_dir, crx_filename), 'wb') as crx:
		data = [magic, version, pubkey_len, signature_len, pubkey, signature, zip_data]
		for d in data:
			crx.write(d)

	shutil.move(path.join(development_dir, crx_filename), path.join(release_dir, crx_filename))


def _generate_signature(zip_file, key_file):
	# Sign the zip file with the private key
	openssl_sign = lib.PopenWithoutNewConsole(['openssl', 'sha1', '-binary', '-sign', key_file, zip_file], stdout=PIPE)
	if openssl_sign.wait() != 0:
		raise ChromeError("Problem signing Chrome package. openssl returned {}".format(openssl_sign.returncode))
	signature = openssl_sign.stdout.read()

	# Convert the public part of the PEM key to DER format for inclusion in the CRX header
	openssl_der = lib.PopenWithoutNewConsole(['openssl', 'rsa', '-pubout', '-inform', 'PEM', '-outform', 'DER', '-in',
											 key_file], stdout=PIPE)
	if openssl_der.wait() != 0:
		raise ChromeError("Problem converting PEM key to DER. openssl returned {}".format(openssl_der.returncode))
	pubkey = openssl_der.stdout.read()

	return signature, pubkey

