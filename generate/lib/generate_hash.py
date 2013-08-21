import os
import hashlib
import json

os.chdir(os.path.dirname(os.path.abspath(__file__)))

hashes = {}

for root, dirs, files in os.walk('.'):
	for file in files:
		path = os.path.join(root, file)
		with open(path, 'rb') as cur_file:
			hash = hashlib.md5(cur_file.read()).hexdigest()
			hashes[path.replace('\\', '/')[2:]] = hash
			

with open('hash.json', 'w') as hash_file:
	json.dump(hashes, hash_file, indent=2)