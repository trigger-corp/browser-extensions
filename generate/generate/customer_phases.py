"Tasks that might be run on the customers's machine"

from time import gmtime
from calendar import timegm

# where the customer code exists inside the apps
locations_normal = {
	'chrome': 'development/chrome/src',
	'firefox': 'development/firefox/resources/f/data/src',
	'safari': 'development/forge.safariextension/src',
	'ie': 'development/ie/src',
}

locations_debug = {
	'chrome': 'development/chrome/src',
	'firefox': 'development/firefox/resources/f/data/src',
	'safari': 'development/forge.safariextension/src',
	'ie': 'development/ie/src',
}


def validate_user_source(src='src'):
	'''Check for any issues with the user source, i.e. no where to include all.js'''
	return [
		{'do': {'check_index_html': (src,)}}
	]

def copy_user_source_to_tempdir(ignore_patterns=None, tempdir=None):
	return [
		{'do': {'copy_files': {'from': 'src', 'to': tempdir, 'ignore_patterns': ignore_patterns}}},
	]

def delete_tempdir(tempdir=None):
	return [
		{'do': {'remove_files': tempdir}},
	]

def run_hook(hook=None, dir=None):
	return [
		{'do': {'run_hook': {'hook': hook, 'dir': dir}}},
	]

def copy_user_source_to_template(ignore_patterns=None, src='src', debug=False):
	if not debug:
		locations = locations_normal
	else:
		locations = locations_debug

	return [
		{'when': {'platform_is': 'chrome'}, 'do': {'copy_files': { 'from': src, 'to': locations["chrome"], 'ignore_patterns': ignore_patterns }}},
		{'when': {'platform_is': 'firefox'}, 'do': {'copy_files': { 'from': src, 'to': locations["firefox"], 'ignore_patterns': ignore_patterns }}},
		{'when': {'platform_is': 'safari'}, 'do': {'copy_files': { 'from': src, 'to': locations["safari"], 'ignore_patterns': ignore_patterns }}},
		{'when': {'platform_is': 'ie'}, 'do': {'copy_files': { 'from': src, 'to': locations["ie"], 'ignore_patterns': ignore_patterns }}},
	]
	
def include_platform_in_html(debug=False):
	if not debug:
		locations = locations_normal
	else:
		locations = locations_debug

	return [
		{'when': {'platform_is': 'firefox'}, 'do': {'find_and_replace_in_dir': {
			"root_dir": locations["firefox"],
			"find": "<head>",
			"replace": "<head><script src='%{back_to_parent}%forge/app_config.js'></script><script src='%{back_to_parent}%forge/all.js'></script>"
		}}},
		{'when': {'platform_is': 'chrome'}, 'do': {'find_and_replace_in_dir': {
			"root_dir": locations["chrome"],
			"find": "<head>",
			"replace": "<head><script src='/forge/app_config.js'></script><script src='/forge/all.js'></script>"
		}}},
		{'when': {'platform_is': 'safari'}, 'do': {'find_and_replace_in_dir': {
			"root_dir": locations["safari"],
			"find": "<head>",
			"replace": "<head><script src='%{back_to_parent}%forge/app_config.js'></script><script src='%{back_to_parent}%forge/all.js'></script>"
		}}},
		{'when': {'platform_is': 'ie'}, 'do': {'find_and_replace_in_dir': {
			"root_dir": locations["ie"],
			"find": "<head>",
			"replace": "<head><script src='%{back_to_parent}%forge/app_config.js'></script><script src='%{back_to_parent}%forge/all.js'></script>"
		}}},
	]

def include_name():
	return [
		{'do': {'populate_xml_safe_name': ()}},
		{'do': {'populate_json_safe_name': ()}},
		{'when': {'platform_is': 'chrome'}, 'do': {'find_and_replace': {
			"in": ('development/chrome/manifest.json',),
			"find": "APP_NAME_HERE", "replace": "${json_safe_name}"
		}}},
		{'when': {'platform_is': 'firefox'}, 'do': {'find_and_replace': {
			"in": ('development/firefox/install.rdf',),
			"find": "APP_NAME_HERE", "replace": "${xml_safe_name}"
		}}},
		{'when': {'platform_is': 'safari'}, 'do': {'find_and_replace': {
			"in": ('development/forge.safariextension/Info.plist',),
			"find": "APP_NAME_HERE", "replace": "${xml_safe_name}"
		}}},
		{'when': {'platform_is': 'ie'}, 'do': {'find_and_replace': {
			"in": ('development/ie/manifest.json',
				'development/ie/dist/setup-x86.nsi',
				'development/ie/dist/setup-x64.nsi',
			), "find": "APP_NAME_HERE", "replace": "${json_safe_name}"
		}}},
	]

def include_requirements():
	return [
	]

def include_uuid():
	return [
		{'do': {'populate_package_names': ()}},
		{'when': {'platform_is': 'firefox'}, 'do': {'find_and_replace': {
			"in": ('development/firefox/install.rdf','development/firefox/harness-options.json',),
			"find": "PACKAGE_NAME_HERE", "replace": "${core.firefox.package_name}"
		}}},
		{'when': {'platform_is': 'safari'}, 'do': {'find_and_replace': {
			"in": ('development/forge.safariextension/Info.plist',),
			"find": "PACKAGE_NAME_HERE", "replace": "${core.safari.package_name}"
		}}},
		{'when': {'platform_is': 'ie'}, 'do': {'find_and_replace': {
			"in": ('development/ie/manifest.json', 'development/ie/forge/all.js', 'development/ie/forge/all-priv.js',
				   'development/ie/dist/setup-x86.nsi','development/ie/dist/setup-x64.nsi',),
			"find": "UUID_HERE", "replace": "${uuid}"
		}}},
		{'when': {'platform_is': 'ie'}, 'do': {'find_and_replace': {
			"in": ('development/ie/dist/setup-x86.nsi','development/ie/dist/setup-x64.nsi',),
			"find": "MS_CLSID_HERE", "replace": "${core.ie.package_name}"
		}}},
	]

def include_author():
	return [
		{'when': {'platform_is': 'firefox'}, 'do': {'find_and_replace': {
			"in": ('development/firefox/install.rdf','development/firefox/harness-options.json',),
			"find": "AUTHOR_HERE", "replace": "${author}"
		}}},
		{'when': {'platform_is': 'safari'}, 'do': {'find_and_replace': {
			"in": ('development/forge.safariextension/Info.plist',),
			"find": "AUTHOR_HERE", "replace": "${author}"
		}}},
		{'when': {'platform_is': 'ie'}, 'do': {'find_and_replace': {
			"in": ('development/ie/manifest.json','development/ie/dist/setup-x86.nsi','development/ie/dist/setup-x64.nsi',),
			"find": "AUTHOR_HERE", "replace": "${author}"
		}}},
	]

def include_description():
	return [
		{'when': {'platform_is': 'chrome'}, 'do': {'find_and_replace': {
			"in": ('development/chrome/manifest.json',),
			"find": "DESCRIPTION_HERE", "replace": "${description}"
		}}},
		{'when': {'platform_is': 'firefox'}, 'do': {'find_and_replace': {
			"in": ('development/firefox/install.rdf','development/firefox/harness-options.json',),
			"find": "DESCRIPTION_HERE", "replace": "${description}"
		}}},
		{'when': {'platform_is': 'safari'}, 'do': {'find_and_replace': {
			"in": ('development/forge.safariextension/Info.plist',),
			"find": "DESCRIPTION_HERE", "replace": "${description}"
		}}},
		{'when': {'platform_is': 'ie'}, 'do': {'find_and_replace': {
			"in": ('development/ie/manifest.json','development/ie/dist/setup-x86.nsi','development/ie/dist/setup-x64.nsi',),
			"find": "DESCRIPTION_HERE", "replace": "${description}"
		}}},
	]

def include_version():
	return [
		{'when': {'platform_is': 'chrome'}, 'do': {'find_and_replace': {
			"in": ('development/chrome/manifest.json',),
			"find": "VERSION_HERE", "replace": "${version}"
		}}},
		{'when': {'platform_is': 'firefox'}, 'do': {'find_and_replace': {
			"in": ('development/firefox/install.rdf','development/firefox/harness-options.json',),
			"find": "VERSION_HERE", "replace": "${version}"
		}}},
		{'when': {'platform_is': 'safari'}, 'do': {'find_and_replace': {
			"in": ('development/forge.safariextension/Info.plist',),
			"find": "VERSION_HERE", "replace": "${version}"
		}}},
		{'when': {'platform_is': 'ie'}, 'do': {'find_and_replace': {
			"in": ('development/ie/manifest.json',
				'development/ie/dist/setup-x86.nsi','development/ie/dist/setup-x64.nsi',),
			"find": "VERSION_HERE", "replace": "${version}"
		}}},
	]

def include_config(debug=False):
	if debug:
		return [
		]
	else:
		return [
			{'when': {'platform_is': 'chrome'}, 'do': {'write_config': {
				"filename": 'development/chrome/forge/app_config.js',
				"content": "window.forge = {}; window.forge.config = ${config};"
			}}},
			{'when': {'platform_is': 'safari'}, 'do': {'write_config': {
				"filename": 'development/forge.safariextension/forge/app_config.js',
				"content": "window.forge = {}; window.forge.config = ${config};"
			}}},
			{'when': {'platform_is': 'firefox'}, 'do': {'write_config': {
				"filename": 'development/firefox/resources/f/data/forge/app_config.js',
				"content": "window.forge = {}; window.forge.config = ${config};"
			}}},
			{'when': {'platform_is': 'ie'}, 'do': {'write_config': {
				"filename": 'development/ie/forge/app_config.js',
				"content": "window.forge = {}; window.forge.config = ${config};"
			}}},
		]

def run_plugin_build_steps(build):
	return [
	]

def migrate_to_plugins():
	return [
		{'do': {'migrate_to_plugins': ()}}
	]

def resolve_urls():
	return [
		{'do': {'resolve_urls': (
			'plugins.activations.config.activations.[].scripts.[]',
			'plugins.activations.config.activations.[].styles.[]',
			'plugins.icons.config.chrome.*',
			'plugins.icons.config.safari.*',
			'plugins.icons.config.firefox.*',
			'plugins.icons.config.ie.*',
			'plugins.button.config.default_icon',
			'plugins.button.config.default_popup',
			'plugins.button.config.default_icons.*'
		)}},
	]

def run_firefox_phase(build_type_dir):
	return [
		{'when': {'platform_is': 'firefox'}, 'do': {'run_firefox': (build_type_dir,)}},
	]
	
def run_chrome_phase():
	return [
		{'when': {'platform_is': 'chrome'}, 'do': {'run_chrome': ()}},
	]

def run_safari_phase():
	return [
		{'when': {'platform_is': 'safari'}, 'do': {'run_safari': ()}},
	]

def run_ie_phase():
	return [
		{'when': {'platform_is': 'ie'}, 'do': {'run_ie': ()}},
	]

def package(build_type_dir):
	return [
		{'when': {'platform_is': 'chrome'}, 'do': {'package_chrome': ()}},
		{'when': {'platform_is': 'safari'}, 'do': {'package_safari': ()}},
		{'when': {'platform_is': 'firefox'}, 'do': {'package_firefox': ()}},
		{'when': {'platform_is': 'ie'}, 'do': {'package_ie': ()}},
	]

def make_installers():
	return [
	]

def check_javascript():
	return [
		{'do': {'lint_javascript': ()}},
	]

def check_local_config_schema():
	return [
		{'do': {'check_local_config_schema': ()}},
	]

def migrate_config():
	return [
		{'do': {'migrate_config': ()}},
	]

def clean_phase():
	return [
		{'when': {'platform_is': 'firefox'}, 'do': {'clean_firefox': 'development'}},
	]
