# Phases for legacy (module rather than plugin) build process which is still used for:
# ie, chrome, safari, firefox

from os import path

_icon_path_for_customer = {
	"safari": "development/forge.safariextension/",
	"firefox": "development/firefox/",
}

locations = {
	'chrome': 'development/chrome/src',
	'firefox': 'development/firefox/resources/f/data/src',
	'safari': 'development/forge.safariextension/src',
	'ie': 'development/ie/src',
}

def create_all_js():
	return [
		{'do': {'add_to_all_js': 'common-v2/legacy/button.js'}},
		{'do': {'add_to_all_js': 'common-v2/legacy/document.js'}},
		{'do': {'add_to_all_js': 'common-v2/legacy/prefs.js'}},
		{'do': {'add_to_all_js': 'common-v2/legacy/tabs.js'}},
		{'do': {'add_to_all_js': 'common-v2/legacy/message.js'}},
		{'do': {'add_to_all_js': 'common-v2/legacy/notification.js'}},
		{'do': {'add_to_all_js': 'common-v2/legacy/file.js'}},
		{'do': {'add_to_all_js': 'common-v2/legacy/request.js'}},
		{'do': {'add_to_all_js': 'common-v2/legacy/geolocation.js'}},
	]

def platform_specific_templating(build):
	content_security_policy = ""
	try:
		content_security_policy = build.config['core']['chrome']['content_security_policy']
	except Exception:
		pass

	web_accessible_resources = ""
	try:
		web_accessible_resources = build.config['core']['chrome']['web_accessible_resources']
	except Exception:
		pass

	return [
		{'when': {'platform_is': 'chrome', 'config_property_exists': 'core.chrome.content_security_policy'}, 'do': {'set_in_json': {
			"filename": 'chrome/manifest.json',
			"key": "content_security_policy",
			"value": content_security_policy
		}}},
		{'when': {'platform_is': 'chrome', 'config_property_exists': 'core.chrome.web_accessible_resources'}, 'do': {'set_in_json': {
			"filename": 'chrome/manifest.json',
			"key": "web_accessible_resources",
			"value": web_accessible_resources
		}}},
	]

def customer_phase():
	icon_path = _icon_path_for_customer
	def icon(platform, sub_path):
		return path.join(icon_path[platform], sub_path)

	return [
		{'when': {'platform_is': 'firefox'}, 'do': {'wrap_activations': locations["firefox"]}},
		{'when': {'platform_is': 'safari'}, 'do': {'wrap_activations': locations["safari"]}},
		{'when': {'platform_is': 'chrome'}, 'do': {'populate_icons': ("chrome", [16, 48, 128])}},
		{'when': {'platform_is': 'firefox'}, 'do': {'populate_icons': ("firefox", [32, 64])}},
		{'when': {'platform_is': 'safari'}, 'do': {'populate_icons': ("safari", [32, 48, 64])}},

		{'when': {'platform_is': 'safari', 'icon_available': ('safari', '32')}, 'do': {'copy_files': {
			'from': '${plugins["icons"]["config"]["safari"]["32"]}',
			'to': icon("safari", 'icon-32.png')
		}}},
		{'when': {'platform_is': 'safari', 'icon_available': ('safari', '48')}, 'do': {'copy_files': {
			'from': '${plugins["icons"]["config"]["safari"]["48"]}',
			'to': icon("safari", 'icon-48.png')
		}}},
		{'when': {'platform_is': 'safari', 'icon_available': ('safari', '64')}, 'do': {'copy_files': {
			'from': '${plugins["icons"]["config"]["safari"]["64"]}',
			'to': icon("safari", 'icon-64.png')
		}}},
		
		{'when': {'platform_is': 'firefox', 'icon_available': ('firefox', '32')}, 'do': {'copy_files': {
			'from': '${plugins["icons"]["config"]["firefox"]["32"]}',
			'to': icon("firefox", 'icon.png')
		}}},
		{'when': {'platform_is': 'firefox', 'icon_available': ('firefox', '64')}, 'do': {'copy_files': {
			'from': '${plugins["icons"]["config"]["firefox"]["64"]}',
			'to': icon("firefox", 'icon64.png')
		}}},

	]
