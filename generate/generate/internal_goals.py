import logging

LOG = logging.getLogger(__name__)

def config_changes_invalidate_templates(generate, old_config_filename, new_config_filename):
	return generate.internal_tasks.config_changes_invalidate_templates(
			generate,
			old_config_filename,
			new_config_filename)
