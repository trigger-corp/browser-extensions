import sys

USAGE = """Usage: {prog} <pythonpath> <entrypoint>

e.g.
{prog} path/to/lib.zip mymodule.myfunction

Any extra arguments will appear in sys.argv for the invoked function.
""".format(prog=sys.executable)


def main():
	try:
		extra_path = sys.argv.pop(1).split(',')
		entry_point = sys.argv.pop(1)
	except IndexError:
		sys.stderr.write(USAGE)
		sys.exit(1)

	module_name, function_name = entry_point.split('.')
	
	sys.path = extra_path + sys.path
	module = __import__(module_name)

	getattr(module, function_name)()

if __name__ == "__main__":
	main()
