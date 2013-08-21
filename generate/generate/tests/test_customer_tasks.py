import tempfile
import shutil
import mock
import os
from nose.tools import ok_

from generate import build
from generate.build import ConfigurationError
from generate import customer_tasks as tasks

import lib

class TestCopyFiles(object):
	def setUp(self):
		self._build = build.Build({
			"activations": [
				{"patterns": []}
			]
		}, "", "")

		self._temp_dir = tempfile.mkdtemp('test_customer_tasks')
		self._src_dir = os.path.join(self._temp_dir, 'src')
		self._build_dir = os.path.join(self._temp_dir, 'development')

		dummy_src_dir = os.path.abspath(os.path.join(__file__, '..', 'fixtures', 'src'))
		shutil.copytree(dummy_src_dir, self._src_dir)

		dummy_file = os.path.abspath(os.path.join(__file__, '..', 'fixtures', 'dummy_file.txt'))
		shutil.copy(dummy_file, self._temp_dir)
		self._dummy_file = os.path.join(self._temp_dir, 'dummy_file.txt')

	def tearDown(self):
		shutil.rmtree(self._temp_dir)

	def test_copy_files_should_raise_on_missing_args(self):
		# TODO: pass in a proper build instead of a mock
		lib.assert_raises_regexp(ConfigurationError, 'requires "from" and "to"', tasks.copy_files, mock.Mock())

	def test_copy_files_should_raise_on_attempting_to_copy_to_already_existing_dir(self):
		os.makedirs(self._build_dir)
		lib.assert_raises_regexp(OSError, 'File exists', tasks.copy_files, self._build, **{'from':self._src_dir, 'to':self._build_dir})

	def test_will_copy_a_single_file_to_an_already_existing_dir(self):
		os.makedirs(self._build_dir)

		tasks.copy_files(self._build, **{
			'from':self._dummy_file, 
			'to':self._build_dir
		})

		file = 'dummy_file.txt'

		ok_(os.path.isfile(os.path.join(self._build_dir, file)), "Should have copied %s into %s" % (file, self._build_dir))

	def test_will_create_a_file_if_to_argument_doesnt_exist(self):
		tasks.copy_files(self._build, **{
			'from':self._dummy_file, 
			'to':self._build_dir
		})

		ok_(os.path.isfile(self._build_dir), "Should have made a new file at %s" % self._build_dir)

	def test_copy_files_should_ignore_the_right_files(self):
		tasks.copy_files(self._build, **{
			'from':self._src_dir, 
			'to':self._build_dir,
			'ignore_patterns': ['ignoreme.txt', '.svn', '.git'],
		})

		os.listdir(self._build_dir)

		for dir in ('res', ):
			ok_(os.path.isdir(os.path.join(self._build_dir, dir)), "directory should have been copied across: %s" % dir)

		for dir in ('.svn', '.git'):
			ok_(not os.path.isdir(os.path.join(self._build_dir, dir)), "directory shouldn't have been copied across: %s" % dir)

		for name in ('index.html', 'background.js'):
			ok_(os.path.isfile(os.path.join(self._build_dir, name)), "file should have been copied across: %s" % name)

		for name in ('ignoreme.txt', ):
			ok_(not os.path.isfile(os.path.join(self._build_dir, name)), "file shouldn't have been copied across: %s" % name)

	def test_ignores(self):
		input = self._src_dir
		output = self._build_dir

		tasks.copy_files(self._build, **{
			'from': input,
			'to': output,
			'ignore_patterns': [
				# ignore any file called file1
				'file1',

				# ignore file2 at the top level
				'./file2',

				# ignore folder1/file3 specifically, relative to top level
				# i.e. equivalent to ./folder1/file3
				'folder1/file3',

				# ignore any files with the ignore1 extension
				'*.ignore1',

				# ignore any files in folder1 with the ignore2 extension
				'folder1/*.ignore2',
				'ignore-*',

				# ignore a folder called fileorfolder1 anywhere in the tree (but not files called fileorfolder1)
				'fileorfolder1/',

				'folder5/*/file',
			]
		})

		def was_copied(name):
			ok_(os.path.exists(os.path.join(input, name)), "problem with fixture, file should have existed in src: %s" % name)
			ok_(os.path.exists(os.path.join(output, name)), "file should have been copied across: %s" % name)

		def wasnt_copied(name):
			ok_(os.path.exists(os.path.join(input, name)), "problem with fixture, file should have existed in src: %s" % name)
			ok_(not os.path.exists(os.path.join(output, name)), "file shouldn't have been copied across: %s" % name)

		wasnt_copied('file1')
		wasnt_copied('folder1/file1')

		wasnt_copied('file2')
		was_copied('folder1/file2')

		wasnt_copied('folder1/file3')
		was_copied('file3')

		wasnt_copied('file.ignore1')
		wasnt_copied('folder1/file.ignore1')

		wasnt_copied('folder1/file.ignore2')
		was_copied('folder1/folder2/file.ignore2')

		wasnt_copied('fileorfolder1')
		wasnt_copied('fileorfolder1/file2')
		wasnt_copied('fileorfolder1/file3')

		file_or_folder = 'folder3/fileorfolder1'
		ok_(os.path.isfile(os.path.join(input, file_or_folder)), "problem with fixture, expected file: %s" % file_or_folder)
		was_copied(file_or_folder)

		wasnt_copied('folder4/fileorfolder1')

		wasnt_copied('ignore-1')
		wasnt_copied('ignore-2')

		wasnt_copied('folder5/folder6/file')
		was_copied('folder5/folder6/file2')
		wasnt_copied('folder5/folder7/file')

