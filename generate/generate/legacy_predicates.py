from lib import predicate

@predicate
def icon_available(build, platform, size):
	return "plugins" in build.config and \
		"icons" in build.config["plugins"] and \
		(size in build.config["plugins"]["icons"]["config"] or \
		size in build.config["plugins"]["icons"]["config"].get(platform, {}))
