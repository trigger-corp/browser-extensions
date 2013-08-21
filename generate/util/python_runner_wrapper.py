"""The logic for our binary. This just imports some python code which contains the dispatch logic."""

import include_everything

if __name__ == "__main__":
	python_runner = __import__('python_runner')
	python_runner.main()
