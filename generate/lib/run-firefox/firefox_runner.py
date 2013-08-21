import os
import shutil
import codecs
import json

from cuddlefish.runner import run_app
from cuddlefish.rdf import RDFManifest

def run():
	original_harness_options = os.path.join('development', 'firefox', 'harness-options.json')
	backup_harness_options = os.path.join('development', 'firefox', 'harness-options-bak.json')
	shutil.move(original_harness_options, backup_harness_options)

	with codecs.open(backup_harness_options, encoding='utf8') as harness_file:
		harness_config = json.load(harness_file)

	run_app(
		harness_root_dir=os.path.join('development', 'firefox'), 
		harness_options=harness_config, 
		app_type="firefox", 
		verbose=True
	)
