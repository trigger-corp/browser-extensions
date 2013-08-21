module("forge.message");

if (forge.is.desktop()) {
	// Only perform messaging tests on desktop
	
	// Listen for all messages
	forge.message.listen(function (content, reply) {
		reply(content);
		reply(content);
	});
	
	// Listen for a specific type of message
	forge.message.listen('test', function (content, reply) {
		reply(content);
		reply(content);
	});
	
	asyncTest('toFocussed (random type)', function() {
		// Expect 2x1 reply, from the focussed tab, listening to all message types
		var random = Math.random().toString()
		var replies = 0;
		var failed = false;
		var timeout;
		forge.message.toFocussed(random, {data: random}, function (content) {
			if (content.data === random) {
				replies++;
			} else {
				failed = true;
			}
			if (!timeout) {
				// If this is the first reply give 1 second for other replies to arrive
				timeout = setTimeout(function () {
					equal(failed, false, 'no failed responses');
					equal(replies, 2, 'number of correct responses');
					start();
				}, 1000);			
			}
		});
	});
	
	asyncTest('toFocussed (specific type)', function() {
		// Expect 2x2 replies, from the focussed tab, both listening to all message types and specific type
		var random = Math.random().toString()
		var replies = 0;
		var failed = false;
		var timeout;
		forge.message.toFocussed('test', {data: random}, function (content) {
			if (content.data === random) {
				replies++;
			} else {
				failed = true;
			}
			if (!timeout) {
				// If this is the first reply give 1 second for other replies to arrive
				timeout = setTimeout(function () {
					equal(failed, false, 'no failed responses');
					equal(replies, 4, 'number of correct responses');
					start();
				}, 1000);			
			}
		});
	});
	
	asyncTest('broadcast (random type)', function() {
		// Expect 2x2 replies, from both fg pages, listening to all message types
		var random = Math.random().toString()
		var replies = 0;
		var failed = false;
		var timeout;
		forge.message.broadcast(random, {data: random}, function (content) {
			if (content.data === random) {
				replies++;
			} else {
				failed = true;
			}
			if (!timeout) {
				// If this is the first reply give 1 second for other replies to arrive
				timeout = setTimeout(function () {
					equal(failed, false, 'no failed responses');
					equal(replies, 4, 'number of correct responses');
					start();
				}, 1000);			
			}
		});
	});
	
	asyncTest('broadcast (specific type)', function() {
		// Expect 2x4 replies, from both fg pages, both listening to all message types and specific type
		var random = Math.random().toString()
		var replies = 0;
		var failed = false;
		var timeout;
		forge.message.broadcast('test', {data: random}, function (content) {
			if (content.data === random) {
				replies++;
			} else {
				failed = true;
			}
			if (!timeout) {
				// If this is the first reply give 1 second for other replies to arrive
				timeout = setTimeout(function () {
					equal(failed, false, 'no failed responses');
					equal(replies, 8, 'number of correct responses');
					start();
				}, 1000);			
			}
		});
	});
}