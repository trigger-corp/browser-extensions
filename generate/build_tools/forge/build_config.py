import json
import logging
import traceback
from os import path

import defaults
import forge
from forge.lib import open_file

LOG = logging.getLogger(__name__)

def load(filename=None):
	'''Read in and JSON the app-independent configuration file (normally forge_build.json).
	'''
	if filename is None:
		filename = defaults.CONFIG_FILE
		if path.exists(filename + ".local"):
			filename = filename + ".local"
			LOG.info("Using config file: %s" % filename)

	if path.exists(filename):
		with open(filename) as conf_file:
			config = json.load(conf_file)
	else:
		config = {}

	LOG.debug('Forge build tools version: %s' % forge.get_version())
	for key, val in config.iteritems():
		LOG.debug('%s: %s' % (key, json.dumps(val)))

	return config

def load_app(app_path="."):
	'''Read in and JSON parse the per-app configuration file (src/config.json)
	and identify JSON file (src/identity.json)
	'''
	app_config_file = defaults.APP_CONFIG_FILE

	with open_file(path.join(app_path, app_config_file)) as app_config:
		try:
			config = json.load(app_config)
		except ValueError as e:
			raise forge.ForgeError("Your configuration file ({0}) is malformed:\n{1}".format(app_config_file, e))
	
	identity_file = defaults.IDENTITY_FILE
	
	if not path.isfile(path.join(app_path, identity_file)):
		if 'uuid' in config:
			# old-style config, where uuid was in config.json, rather than identity.json
			identity_contents = {"uuid": config["uuid"]}
			LOG.warning("we need to update your configuration to include an 'identity.json' file")
			with open_file(path.join(app_path, identity_file), 'w') as identity:
				json.dump(identity_contents, identity)
			LOG.info("configuration updated: 'identity.json' created")
		else:
			raise IOError("'identity.json' file is missing")
	
	with open_file(path.join(app_path, identity_file)) as identity:
		try:
			identity_config = json.load(identity)
		except ValueError as e:
			raise forge.ForgeError("Your identity file ({0}) is malformed:\n{1}".format(identity_file, e))
	
	config.update(identity_config)
	return config

def load_local(app_path="."):
	"""Read in and parse local configuration containing things like location of 
	provisioning profiles, certificates, deployment details
	"""
	local_config_path = path.join(app_path, defaults.LOCAL_CONFIG_FILE)
	local_config_dict = {}
	if path.isfile(local_config_path):
		try:
			with open_file(local_config_path) as local_config_file:
				local_configs = local_config_file.read()
			if local_configs:
				local_config_dict = json.loads(local_configs)
		except IOError as e:
			LOG.debug("Couldn't load local_config.json")
			LOG.debug("%s" % traceback.format_exc())

	return local_config_dict

def save_local(settings, app_path="."):
	"""Dump a dict as JSON into local_config.json, overwriting anything currently in there"""
	local_config_path = defaults.LOCAL_CONFIG_FILE

	with open_file(path.join(app_path, local_config_path), 'w') as local_config_file:
		try:
			json.dump(settings, local_config_file, indent=4)
		except IOError as e:
			raise IOError("Couldn't write to local_config.json")
