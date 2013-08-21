import tempfile
import shutil
import os
from nose.tools import eq_, raises

import generate
from generate import internal_tasks

class TestConfigChangesInvalidateTemplates(object):
	def setUp(self):
		self._temp_dir = tempfile.mkdtemp('test_internal_tasks')
		self._orig_dir = os.getcwd()
		os.chdir(self._temp_dir)
	
	def _copy_file_from_fixture(self, path_in_fixtures, destination):
		"""
		Copy a file from in the test "fixtures" directory to the current directory.
		"""
		shutil.copy(os.path.abspath(os.path.join(__file__, '..', 'fixtures', path_in_fixtures)),
				destination)

	def tearDown(self):
		os.chdir(self._orig_dir)
		shutil.rmtree(self._temp_dir)

	def test_identical(self):
		self._copy_file_from_fixture("config.json-initial", "old_config.json")
		self._copy_file_from_fixture("config.json-initial", "new_config.json")
		eq_(False, internal_tasks.config_changes_invalidate_templates(
			generate, "old_config.json", "new_config.json"))
	
	def test_platform_version_changed(self):
		self._copy_file_from_fixture("config.json-initial", "old_config.json")
		self._copy_file_from_fixture("config.json-new_platform_version", "new_config.json")
		eq_(True, internal_tasks.config_changes_invalidate_templates(
			generate, "old_config.json", "new_config.json"))
	
	def test_partners_changed(self):
		self._copy_file_from_fixture("config.json-initial", "old_config.json")
		self._copy_file_from_fixture("config.json-nested_partner_change", "new_config.json")
		eq_(True, internal_tasks.config_changes_invalidate_templates(
			generate, "old_config.json", "new_config.json"))
	
	@raises(generate.lib.BASE_EXCEPTION)
	def test_modules_changed(self):
		self._copy_file_from_fixture("config.json-initial", "old_config.json")
		self._copy_file_from_fixture("config.json-invalid_json", "new_config.json")
		internal_tasks.config_changes_invalidate_templates(generate, "old_config.json", "new_config.json")
