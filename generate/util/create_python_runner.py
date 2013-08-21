"""Run this to generate python_runner_win32.exe or python_runner_darwin binaries.

Requires Pyinstaller to be installed.
"""

import subprocess
import sys
import shutil
import os
from os import path

if sys.platform.startswith("win"):
	pyinstaller = 'C:\\opt\\pyinstaller-1.5.1'
elif sys.platform.startswith("darwin"):
	pyinstaller = '/opt/pyinstaller-1.5.1'

parent_dir = path.dirname(path.abspath(__file__))
python = sys.executable
output = path.join(path.dirname(__file__), 'dist', 'python_runner_%s' % sys.platform)

if sys.platform.startswith("win"):
	output += '.exe'

spec_file = path.join(parent_dir, 'python_runner.spec')
configure = path.join(pyinstaller, 'Configure.py')
build = path.join(pyinstaller, 'Build.py')
lib_folder = path.join(path.dirname(parent_dir), 'lib')


def _run(*command):
	proc = subprocess.Popen(
		list(command),
		env=dict(os.environ, VERSIONER_PYTHON_PREFER_32_BIT='yes')
	)
	assert proc.wait() == 0

_run(python, configure)
_run(python, build, spec_file)

print "Moving to %s" % lib_folder
shutil.move(output, lib_folder)
