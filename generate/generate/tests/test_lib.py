from os import path

from nose.tools import ok_, eq_

from generate import lib

class TestReadFileAsStr(object):
	def _fixtures_file(self, filename):
		return path.abspath(path.join(__file__, '..', 'fixtures', 'encoded_files', filename))

	def test_utf8(self):
		result = lib.read_file_as_str(self._fixtures_file('utf8.txt'))
		ok_(isinstance(result, unicode))

	def test_utf32(self):
		result = lib.read_file_as_str(self._fixtures_file('utf32.txt'))
		ok_(isinstance(result, unicode))

	def test_gb2312(self):
		result = lib.read_file_as_str(self._fixtures_file('gb2312.txt'))
		ok_(isinstance(result, unicode))

	def test_cp866(self):
		result = lib.read_file_as_str(self._fixtures_file('cp866.txt'))
		ok_(isinstance(result, unicode))

	def test_shift_jis(self):
		result = lib.read_file_as_str(self._fixtures_file('shift_jis.txt'))
		ok_(isinstance(result, unicode))

class TestFormatPopenArgs(object):
	def test_should_perform_no_quoting_if_shell_true(self):
		eq_(lib._format_popen_args(args=("a b c",), kwargs={'shell':True}, platform=None), 'Running shell: a b c')

	def test_should_use_single_quotes_on_non_windows(self):
		eq_(lib._format_popen_args(args=(["a", "b", "c"],), kwargs={}, platform="linux"), "Running: 'a' 'b' 'c'")
	
	def test_should_escape_single_quotes_in_args(self):
		eq_(lib._format_popen_args(args=(["a", "b'", "c"],), kwargs={}, platform="linux"), "Running: 'a' 'b'\"'\"'' 'c'")
