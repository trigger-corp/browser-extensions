import json
from nose.tools import eq_, assert_raises
import mock
from os import path

from generate import build

class TestHandleFlagArg(object):

	def test_should_set_flag_to_False_if_value_was_no(self):
		log = mock.Mock()

		config = build.ToolConfig(log, None, None, ['dummy target'])
		result = config._handle_flag_arg('dummy flag', ['no'], {})

		eq_(result, False, 'dummy flag should have been set to False')

	def test_should_set_flag_to_True_if_no_value(self):
		log = mock.Mock()

		config = build.ToolConfig(log, None, None, ['dummy target'])
		result = config._handle_flag_arg('dummy flag', [], {})

		eq_(result, True, 'dummy flag should have been set to True')

class TestDefaultsAndOverridesAreSaved(object):
	def get_default_local_config(self):
		local_config_filename = path.join(path.dirname(__file__), path.pardir, path.pardir, path.pardir, 'configuration', 'local_config.json')
		with open(local_config_filename) as local_config_file:
			return json.load(local_config_file)

	def test_normal_defaults_saved(self):
		log = mock.Mock()
		config = build.ToolConfig(log, {"a": {"b": 1, "c": 2}}, None, ['dummy target'])

		eq_(config.get('a.b'), 1)
		eq_(config['a.c'], 2)

	def test_deprecated_overrides_saved(self):
		log = mock.Mock()
		config = build.ToolConfig(log, {}, ["--sdk", "1", "-c", "2"], ['dummy target'])

		eq_(config.get('sdk'), None)
		assert_raises(KeyError, lambda: config["c"])

	def test_profile_switching(self):
		log = mock.Mock()
		local_config = self.get_default_local_config()
		local_config["ie"]["profiles"]["other_profile"] = {"developer_certificate": "dummy certificate"}

		config = build.ToolConfig(log, local_config, ['--profile', 'other_profile'], ['ie'])

		eq_(config["ie.profile.developer_certificate"], "dummy certificate")
	
	def test_profile_switching_with_typo(self):
		log = mock.Mock()
		local_config = self.get_default_local_config()

		assert_raises(build.ArgumentError, lambda: build.ToolConfig(
			log, local_config, ['--profile', 'other_profile'], ['ie']))
	
	def test_no_DEFAULT_profile(self):
		log = mock.Mock()
		local_config = self.get_default_local_config()
		del local_config["ie"]["profiles"]["DEFAULT"]

		config = build.ToolConfig(log, local_config, [], ['ie'])
		eq_(config.get("ie.profile.developer_certificate"), None)


