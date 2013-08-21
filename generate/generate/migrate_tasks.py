import logging

from lib import task, BASE_EXCEPTION

log = logging.getLogger(__name__)

class MigrationError(BASE_EXCEPTION):
	pass

@task
def migrate_config(build):
	# TODO: Orientations migration
	log.info("No migration currently available, you are already on the latest stable version of Forge.")
