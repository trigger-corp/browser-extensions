from lib import predicate

@predicate
def is_external(build):
	return bool(build.external)

@predicate
def do_package(build):
	return getattr(build, "package", False)

@predicate
def config_property_exists(build, property):
	properties = property.split('.')
	at = build.config
	for x in properties:
		if x in at:
			at = at[x]
		else:
			return False
	return True
@predicate
def config_property_does_not_exist(build, property):
	return not config_property_exists(build, property)

@predicate
def config_property_true(build, property):
	properties = property.split('.')
	at = build.config
	for x in properties:
		if x in at:
			at = at[x]
		else:
			return False
	return at == True
@predicate
def config_property_false_or_missing(build, property):
	return not config_property_true(build, property)

@predicate
def config_property_equals(build, property, value):
	properties = property.split('.')
	at = build.config
	for x in properties:
		if x in at:
			at = at[x]
		else:
			return False
	return at == value
	
@predicate
def platform_is(build, platform):
	return platform == 'all' or (set(platform.split(',')) & set(build.enabled_platforms))
	
@predicate
def platform_is_not(build, platform):
	return not platform_is(build, platform)

