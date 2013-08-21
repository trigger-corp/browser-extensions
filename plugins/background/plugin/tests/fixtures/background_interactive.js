forge.tools.getURL("/index.html", function (url) {
	forge.tabs.open(url);
});

// Listen for all messages
forge.message.listen(function (content, reply) {
	var random = Math.random().toString()
	forge.message.broadcast(random, content, function (content) {
		reply(content);
		reply(content);
	});
});

// Listen for a specific type of message
forge.message.listen('test', function (content, reply) {
	forge.message.broadcast('test', content, function (content) {
		reply(content);
		reply(content);
	});
});