import logging
import argparse
import shutil
import os
import json

logging.basicConfig(level=logging.DEBUG, format='[%(levelname)7s] %(asctime)s -- %(message)s')
log = logging.getLogger(__name__)

def main():
	parser = argparse.ArgumentParser(description='Generate a new Forge app')
	parser.add_argument('-o', '--output', default='generated', help='Location to put the generated app')
	parser.add_argument('-z', '--modules_folder', default='plugins', help='Location of modules to generate app from')
	parser.add_argument('-t', '--type', default='automated', help='Type of tests to include (either automated or interactive)')
	parser.add_argument('modules', nargs='+', help='A list of modules to include in the test app (or all for all modules)')

	args = parser.parse_args()

	if os.path.exists(args.output):
		log.info("Removing existing output folder")
		shutil.rmtree(args.output)

	log.info("Copying template test app to output folder")
	shutil.copytree(os.path.join('test-apps', 'module'), args.output)

	if not os.path.isdir(os.path.join(args.output, 'tests')):
		os.makedirs(os.path.join(args.output, 'tests'))

	if not os.path.isdir(os.path.join(args.output, 'fixtures')):
		os.makedirs(os.path.join(args.output, "fixtures"))

	with open(os.path.join(args.output, 'config.json')) as app_config_file:
		app_config = json.load(app_config_file)

	if "all" in args.modules:
		modules = os.listdir(args.modules_folder)
	else:
		modules = args.modules

	log.info("Processing modules")
	for module in modules:
		if module.startswith("."): continue
		if not os.path.exists(os.path.join(args.modules_folder, module)):
			log.error("Missing module: %s" % module)
			return
		if os.path.exists(os.path.join(args.modules_folder, module, "plugin", "tests", "%s.js" % args.type)):
			log.info("Copying tests for module: %s" % module)
			shutil.copy2(os.path.join(args.modules_folder, module, "plugin", "tests", "%s.js" % args.type), os.path.join(args.output, 'tests', '%s.js' % module))
		else:
			log.warn("Module has no %s tests: %s" % (args.type, module))

		if os.path.exists(os.path.join(args.modules_folder, module, "plugin", "tests", "fixtures")):
			shutil.copytree(os.path.join(args.modules_folder, module, "plugin", "tests", "fixtures"), os.path.join(args.output, "fixtures", module))

		if os.path.exists(os.path.join(args.modules_folder, module, "plugin", "tests", "config_" + args.type + ".json")):
			with open(os.path.join(args.modules_folder, module, "plugin", "tests", "config_" + args.type + ".json")) as module_config_file:
				app_config['modules'][module] = json.load(module_config_file)
		elif os.path.exists(os.path.join(args.modules_folder, module, "plugin", "config_example.json")):
			with open(os.path.join(args.modules_folder, module, "plugin", "config_example.json")) as module_config_file:
				app_config['modules'][module] = json.load(module_config_file)
		else:
			app_config['modules'][module] = True

	log.info("Writing app config")
	with open(os.path.join(args.output, 'config.json'), 'w') as app_config_file:
		json.dump(app_config, app_config_file, indent=4, sort_keys=True)

	log.info("Done.")


if __name__ == "__main__":
	main()
