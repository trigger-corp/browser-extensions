import os
from os import path
import sys
import shutil, glob
import logging
import uuid
import hashlib
from subprocess import PIPE, STDOUT

import lib
from lib import CouldNotLocate, task

LOG = logging.getLogger(__name__)

class IEError(Exception):
	pass

@task
def package_ie(build, **kw):
	'Sign executables, Run NSIS'
	
	# On OS X use the nsis executable and files we ship
	if sys.platform.startswith('darwin'):
		nsis_osx = os.path.realpath(os.path.join(os.path.dirname(__file__), '../lib/nsis_osx'))
		nsis_cmd = 'NSISDIR={}/share/nsis PATH={}/bin/:$PATH makensis'.format(nsis_osx, nsis_osx)
	else:
		nsis_cmd = 'makensis'
	LOG.debug("Using nsis command: {nsis_cmd}".format(nsis_cmd=nsis_cmd))	

	nsis_check = lib.PopenWithoutNewConsole("{nsis_cmd} -VERSION".format(nsis_cmd=nsis_cmd), 
											shell=True, stdout=PIPE, stderr=STDOUT)
	stdout, stderr = nsis_check.communicate()
	
	if nsis_check.returncode != 0:
		raise CouldNotLocate("Make sure the 'makensis' executable is in your path")
	
	# JCB: need to check nsis version in stdout here?

	# Sign executables
	certificate = build.tool_config.get('ie.profile.developer_certificate')
	certificate_path = build.tool_config.get('ie.profile.developer_certificate_path')
	certificate_password = build.tool_config.get('ie.profile.developer_certificate_password')
	if certificate:
		# Figure out which signtool to use
		signtool = _check_signtool(build)
		if signtool == None:
			raise CouldNotLocate("Make sure the 'signtool' or 'osslsigncode' executable is in your path")
		LOG.info('Signing IE executables with: {signtool}'.format(signtool=signtool))	

		_sign_app(build=build, 
				  signtool=signtool,
				  certificate=certificate, 
				  certificate_path=certificate_path,
				  certificate_password=certificate_password)
	
	development_dir = path.join("development", "ie")
	release_dir = path.join("release", "ie")
	if not path.isdir(release_dir):
		os.makedirs(release_dir)

	for arch in ('x86', 'x64'):
		nsi_filename = "setup-{arch}.nsi".format(arch=arch)
		
		package = lib.PopenWithoutNewConsole('{nsis_cmd} {nsi}'.format(
			nsis_cmd=nsis_cmd,
			nsi=path.join(development_dir, "dist", nsi_filename)),
			stdout=PIPE, stderr=STDOUT, shell=True
		)
	
		out, err = package.communicate()
	
		if package.returncode != 0:
			raise IEError("problem running {arch} IE build: {stdout}".format(arch=arch, stdout=out))
		
		# move output to release directory of IE directory and sign it
		for exe in glob.glob(development_dir+"/dist/*.exe"):
			destination = path.join(release_dir, "{name}-{version}-{arch}.exe".format(
							name=build.config.get('name', 'Forge App'),
							version=build.config.get('version', '0.1'),
							arch=arch
							))
			shutil.move(exe, destination)
			if certificate:
				_sign_executable(build=build, 
								 signtool=signtool,
								 target=destination, 
								 certificate=certificate, 
								 certificate_path=certificate_path, 
								 certificate_password=certificate_password)
		

def _generate_package_name(build):
	if "core" not in build.config:
		build.config["core"] = {}
	if "ie" not in build.config["core"]:
		build.config["core"]["ie"] = {}
	build.config["core"]["ie"]["package_name"] =  _uuid_to_ms_clsid(build)
	return build.config["core"]["ie"]["package_name"]


def _uuid_to_ms_clsid(build):
	md5   = hashlib.md5(build.config['uuid'])
	guid  = uuid.UUID(md5.hexdigest())
	clsid = uuid.UUID(guid.bytes_le.encode('hex'))
	return "{" + str(clsid).upper() + "}"


def _check_signtool(build):
	options = ["signtool /?", "osslsigncode -v"]

	# Note: The follow code can be uncommented once osslsigncode_osx has been rebuilt to work
	#       on stock OS X. At the moment it is dynamically linked against 
	#       /opt/local/lib/libcrypto.1.0.0.dylib which is not part of the OS and probably a 
	#       homebrew library. The system libcrypto is at /usr/lib/libcrypto.dylib
	#
	# lib_dir = os.path.realpath(os.path.join(os.path.dirname(__file__), '../lib'))
	# if sys.platform.startswith('darwin'):
	# 	options.append('{lib_dir}/osslsigncode_osx -v'.format(lib_dir=lib_dir))
	# elif sys.platform.startswith('linux'):
	# 	options.append('{lib_dir}/osslsigncode_linux -v'.format(lib_dir=lib_dir))

	for option in options:
		LOG.info("Checking: %s", option[:-3])	
		check = lib.PopenWithoutNewConsole(option, shell=True, stdout=PIPE, stderr=STDOUT)
		stdout, stderr = check.communicate()
		if check.returncode == 0:
			return option[:-3]
	LOG.info("Could not find anything: %s" % stdout)
	return None


def _sign_app(build, signtool=None, certificate=None, certificate_path=None, certificate_password=""):
	'Sign all executable code'

	path_win32 = path.join("development", "ie", "build", "Win32", "Release")	
	path_x64   = path.join("development", "ie", "build", "x64",	  "Release")	

	_sign_executable(build, signtool, path.join(path_win32, "bho32.dll"),
					 certificate, certificate_path, certificate_password)
	_sign_executable(build, signtool, path.join(path_win32, "forge32.dll"),
					 certificate, certificate_path, certificate_password)
	_sign_executable(build, signtool, path.join(path_win32, "forge32.exe"),
					 certificate, certificate_path, certificate_password)
	_sign_executable(build, signtool, path.join(path_win32, "frame32.dll"),
					 certificate, certificate_path, certificate_password)
	_sign_executable(build, signtool, path.join(path_x64, "bho64.dll"),
					 certificate, certificate_path, certificate_password)
	_sign_executable(build, signtool, path.join(path_x64, "forge64.dll"),
					 certificate, certificate_path, certificate_password)
	_sign_executable(build, signtool, path.join(path_x64, "forge64.exe"),
					 certificate, certificate_path, certificate_password)
	_sign_executable(build, signtool, path.join(path_x64, "frame64.dll"),
					 certificate, certificate_path, certificate_password)


def _sign_executable(build, signtool, target, certificate = None, certificate_path = None, certificate_password = ""):
	'Sign a single executable file'

	LOG.info('Signing {target}'.format(target=target))

	if signtool == 'signtool':
		command = lib.PopenWithoutNewConsole('signtool sign /f {cert} /p {password} /v /t {time} "{target}"'.format(
			cert=path.join(certificate_path, certificate),
			password=certificate_password,
			time='http://timestamp.comodoca.com/authenticode',
			target=target),
			stdout=PIPE, stderr=STDOUT, shell=True
		)

	elif signtool == 'osslsigncode': 
		command = lib.PopenWithoutNewConsole('osslsigncode -pkcs12 {cert} -pass {password} -t {time} -in "{target}" -out "{target}.signed"'.format(
			cert=path.join(certificate_path, certificate),
			password=certificate_password,
			time='http://timestamp.comodoca.com/authenticode',
			target=target),
			stdout=PIPE, stderr=STDOUT, shell=True
		)

	else:
		raise IEError("problem signing IE build, unknown code sign tool: {signtool}".format(signtool=signtool))

	out, err = command.communicate()
	if command.returncode != 0:
		raise IEError("problem signing IE build: {stdout}".format(stdout=out))

	if signtool == 'osslsigncode':
		shutil.move(target + ".signed", target)	

@task
def run_ie(build):
	msg = """Currently it is not possible to launch an Internet Explorer extension via this interface."""
	
	LOG.info(msg)



