import re
import traceback

from nose.tools import ok_, eq_
from hamcrest import *

def assert_raises_regexp(exc, regex, to_call, *args, **kw):
	'''Check that to_call(*args, **kw) raises an exception of type
	:param:`exc`, whose string value matches the given :param:`regex`
	
	:param exc: an ``type`` of :class:`Exception` to catch
	:param regex: a regular expression string which we will match against
		the stringified exception
	:type regex: string
	:param to_call: the callable which should error out
	'''
	try:
		to_call(*args, **kw)
	except exc, e:
		ok_(
			re.compile(regex).search(str(e)),
			'Raised exception did not match "%s": "%s"' % (regex, str(e))
		)
	except Exception, e:
		ok_(False, 'Raised exception is not a %s: %s\n%s' % (exc, type(e), traceback.format_exc(e)))
	else:
		ok_(False, '%s did not raise an exception' % to_call)


def assert_call_got_kwargs(mock, call_index, **expected_kws):
	got_kws = mock.call_args_list[call_index][1]
	for k,v in expected_kws.items():
		assert_that(got_kws, has_item(k))
		eq_(got_kws[k], v)
