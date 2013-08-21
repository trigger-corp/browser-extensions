import logging
import json
from os import path

import utils

LOG = logging.getLogger(__name__)

def config_changes_invalidate_templates(
		generate,
		old_config_filename,
		new_config_filename):
	with open(old_config_filename) as old_config_file:
		old_config = old_config_file.read()
	with open(new_config_filename) as new_config_file:
		new_config = new_config_file.read()
	
	try:
		current_filename = old_config_filename
		old_config_d = json.loads(old_config)
		current_filename = new_config_filename
		new_config_d = json.loads(new_config)
	except Exception as e:
		raise generate.lib.BASE_EXCEPTION(
				"{file} is not valid JSON: {msg}".format(
					file=current_filename,
					msg=e,
				)
		)
	
	if old_config_d == new_config_d:
		LOG.debug("configuration is identical to last run")
		return False

	with open(
			path.join(utils.path_to_lib(), "invalidating_config.json")
			) as invalidating_config_file:
		invalidating_config = json.load(invalidating_config_file)

	for key in invalidating_config:
		if old_config_d.get(key) != new_config_d.get(key):
			LOG.debug("'{key}' has changed in configuration".format(key=key))
			return True
	
	LOG.debug("configuration has not changed")
	return False
