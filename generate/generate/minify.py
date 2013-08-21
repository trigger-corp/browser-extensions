'''Various minification utilities and tools, from minifying a single file up to minifying whole source trees.
'''
import logging
from os import path
from subprocess import Popen, PIPE
import requests
import urllib
import os

log = logging.getLogger(__name__)

def minify_in_place(source_dir, *filenames):
	'''Minify a file, overwriting the original.
	
	:param filename: the file to minify
	'''
	log.info('minifying %d files' % len(filenames))
	
	try:
		# Try to use http service
		abs_filenames = [urllib.quote(path.join(os.getcwd(),f)) for f in filenames]
		result = requests.get('http://127.0.0.1:6080/?file='+("&file=").join(abs_filenames)).content
		if result[0:7] == "Success":
			log.debug('minification via http service successful')
		else:
			log.debug('minification via http service failed')
			raise Exception('Minification via http service failed')
	except:
		jar = path.join(source_dir, 'generate', 'lib', 'minify-all.jar')
		minify_args = ['java', '-jar', jar, '--charset', 'utf-8'] + list(filenames)
		log.debug('Running minification: "%s"' % ((' ').join(minify_args)))
		proc = Popen(minify_args, stdout=PIPE, stderr=PIPE)
		proc_err = proc.communicate()[1]
		if proc.returncode != 0:
			raise Exception('Minification failed: %s' % (proc_err))
		log.debug('minification successful: %s' % (proc_err))
