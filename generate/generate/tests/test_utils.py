from nose.tools import eq_, ok_
from os import path
from StringIO import StringIO
import errno
import mock

from generate import utils
from generate.tests.lib import assert_call_got_kwargs

class TestRunShell(object):
	def test_returns_output(self):
		eq_(utils.run_shell('echo', 'hello world'), 'hello world\n')

	def test_returns_output_with_check(self):
		eq_(utils.run_shell('echo', 'hello world', check_for_interrupt=True), 'hello world\n')

	def test_no_shell_output(self):
		eq_(utils.run_shell('echo', '-n'), '')

	def test_no_shell_output_with_check(self):
		eq_(utils.run_shell('echo', '-n', check_for_interrupt=True), '')

	def test_raises_on_error(self):
		try:
			fail_script = path.abspath(path.join(__file__, '..', 'fixtures', 'fail.sh'))
			utils.run_shell('sh', fail_script)
		except utils.ShellError as e:
			expected = 'I failed because I am fail.\n'
			message = (
				'Exception should have had attached output: %s, but had %s' % (expected, e.output)
			)
			eq_(e.output, expected, message)
		else:
			ok_(False, 'Should have raised ShellError')

	def test_raises_on_error_check(self):
		try:
			fail_script = path.abspath(path.join(__file__, '..', 'fixtures', 'fail.sh'))
			utils.run_shell('sh', fail_script, check_for_interrupt=True)
		except utils.ShellError as e:
			expected = 'I failed because I am fail.\n'
			message = (
				'Exception should have had attached output: %s, but had %s' % (expected, e.output)
			)
			eq_(e.output, expected, message)
		else:
			ok_(False, 'Should have raised ShellError')

	def test_raises_no_such_file(self):
		try:
			utils.run_shell('bamboowang')
		except OSError as e:
			eq_(e.errno, errno.ENOENT)
		else:
			ok_(False, 'Should have raised OSError')

	def test_raises_no_such_file_with_check_for_interrupt(self):
		try:
			utils.run_shell('bamboowang', check_for_interrupt=True)
		except OSError as e:
			eq_(e.errno, errno.ENOENT)
		else:
			ok_(False, 'Should have raised OSError')

	def _fake_popen(self, *args, **kwargs):
		m = mock.Mock()
		m.stdout = StringIO('Dummy stdout')
		m.stderr = StringIO('Dummy stderr')
		m.poll.return_value = 0
		m.wait.return_value = 0
		return m

	@mock.patch('generate.utils.lib.PopenWithoutNewConsole')
	def test_preexec_fn_is_none_by_default(self, Popen):
		Popen.side_effect = self._fake_popen
		utils.run_shell('foo')

		Popen.assert_called_once()
		assert_call_got_kwargs(Popen, 0, preexec_fn=None)

	@mock.patch('generate.utils.lib.PopenWithoutNewConsole')
	@mock.patch('generate.utils._required_preexec')
	def test_should_pass_in_required_preexec(self, required_preexec, Popen):
		Popen.side_effect = self._fake_popen
		utils.run_shell('foo', create_process_group=True)

		Popen.assert_called_once()
		assert_call_got_kwargs(Popen, 0, preexec_fn=required_preexec.return_value)

class TestRequiredPreexec(object):
	def test_should_return_setsid_if_new_process_group_requested_and_setsid_available(self):
		mock_os = mock.Mock(spec=['setsid'])
		eq_(utils._required_preexec(True, mock_os), mock_os.setsid)

	def test_should_return_none_if_no_new_process_group(self):
		mock_os = mock.Mock(spec=['setsid'])
		eq_(utils._required_preexec(False, mock_os), None)

	def test_should_return_none_if_setsid_unavailable(self):
		mock_os = mock.Mock(spec=['setsid'])
		eq_(utils._required_preexec(False, mock_os), None)

