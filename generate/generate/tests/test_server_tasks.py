from nose.tools import ok_, assert_false

from generate import build, server_tasks

class TestNeedsHttpsActivation(object):
	def setup(self):
		self.build = build.Build({
			"modules": {
				"activations": [
					{"patterns": []}
				],
				"request": {}
			}
		}, "", "")
		
	def test_no_patterns(self):
		server_tasks.need_https_access(self.build)
		assert_false(self.build.config["activate_on_https"])
		
	def test_no_https_patterns(self):
		self.build.config["modules"]["activations"] = [
				{"patterns": ["http://google.com/"]}
		]
		server_tasks.need_https_access(self.build)
		assert_false(self.build.config["activate_on_https"])
		
	def test_https_pattern(self):
		self.build.config["modules"]["activations"] = [
				{"patterns": ["https://google.com/"]}
		]
		server_tasks.need_https_access(self.build)
		ok_(self.build.config["activate_on_https"])
		
	def test_asterisk_pattern(self):
		self.build.config["modules"]["activations"] = [
				{"patterns": ["*://google.com/"]}
		]
		server_tasks.need_https_access(self.build)
		ok_(self.build.config["activate_on_https"])
		
	def test_all_urls(self):
		self.build.config["modules"]["activations"] = [
				{"patterns": ["<all_urls>"]}
		]
		server_tasks.need_https_access(self.build)
		ok_(self.build.config["activate_on_https"])
		
	def test_several_match(self):
		self.build.config["modules"]["activations"] = [
				{"patterns": ["<all_urls>", "http://www.google.com/*", "https://www.google.com/*"]}
		]
		server_tasks.need_https_access(self.build)
		ok_(self.build.config["activate_on_https"])
		
	def test_several_non_match(self):
		self.build.config["modules"]["activations"] = [
				{"patterns": ["http://reddit.com/", "http://cnn.com/", "http://google.com/"]}
		]
		server_tasks.need_https_access(self.build)
		assert_false(self.build.config["activate_on_https"])
	
	def test_in_permissions(self):
		self.build.config["modules"]["activations"] = [
				{"patterns": ["http://www.google.com/*"]}
		]
		self.build.config["modules"]["request"]["permissions"] = [ "<all_urls>" ]
		server_tasks.need_https_access(self.build)
		ok_(self.build.config["activate_on_https"])
