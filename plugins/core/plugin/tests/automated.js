module("websql");

if (!forge.is.firefox() && !forge.is.ie()) {
	// WebSQL not support on Firefox and IE
	asyncTest("Transaction with rollback test", 3, function() {
		var db = openDatabase('foo', '1.0', 'foo', 2 * 1024 * 1024);
		db.transaction(function (tx) {
			tx.executeSql('CREATE TABLE IF NOT EXISTS foo (id unique, text)');  
			tx.executeSql('INSERT INTO foo (id, text) VALUES (1, "foobar")');
			ok(true, "Inserted row");
		});

		db.transaction(function (tx) {
			tx.executeSql('DROP TABLE foo');

			// known to fail - so should rollback the DROP statement
			tx.executeSql('INSERT INTO foo (id, text) VALUES (1, "foobar")');
		}, function (err) {
			ok(true, "Intentionally failing transcation");
		});

		db.transaction(function (tx) {
			tx.executeSql('SELECT * FROM foo', [], function (tx, results) {
				ok(true, "Found row");
				start();
			}, function (tx, err) {
				ok(false, "Error selecting row");
				start();
			});
		});
	});
};

module("tools.UUID");

test("Uniqueness", function() {
	notEqual(forge.tools.UUID(), forge.tools.UUID(), "Generate 2 ids" );
});

test("RFC 4122 compliant", function() {
	ok(forge.tools.UUID().match(/........-....-4...-....-............/), "Correct length and formatting, contains a 4 at the start of the 3rd block." );
});


if (forge.is.desktop()) {
	module("forge.message");

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
	
	asyncTest('broadcastBackground (random type)', function() {
		// Expect 2x1 reply, from the bg, listening to all message types
		var random = Math.random().toString()
		var replies = 0;
		var failed = false;
		var timeout;
		forge.message.broadcastBackground(random, {data: random}, function (content) {
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
	
	asyncTest('broadcastBackground (specific type)', function() {
		// Expect 2x2 replies, from the bg, both listening to all message types and specific type
		var random = Math.random().toString()
		var replies = 0;
		var failed = false;
		var timeout;
		forge.message.broadcastBackground('test', {data: random}, function (content) {
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
}

module("is");

var oneOf = function (arr) {
	var count = 0;
	$.each(arr, function (i, x) {
		if (x) {
			count++;
		}
	});
	return count == 1;
}

test("Type of platform", function() {
	ok(oneOf([forge.is.mobile(), forge.is.desktop(), forge.is.web()]),
		"Exactly one of mobile/desktop/web");
	ok(oneOf([forge.is.chrome(), forge.is.firefox(), forge.is.safari(),
			forge.is.ie()]),
		"Exactly one of chrome/firefox/safari/ie");
});

module("tools.getURL");

asyncTest("Full HTTP URL given", 1, function() {
	var url = "http://google.com";
	forge.tools.getURL(url, function (returned) {
		equal(returned, url, "URL: "+url);
		start();
	});
});
	
asyncTest("Full HTTPS URL given", 1, function() {
	var url2 = "https://google.com";
	forge.tools.getURL(url2, function (returned) {
		equal(returned, url2, "URL: "+url2);
		start();
	});
});


asyncTest("Local URL given (no leading slash)", 1, function() {
	var url = "index.html";
	forge.tools.getURL(url, function (returned) {
		if (forge.is.web()) equal(returned.substr(-10), "index.html", "URL: "+url);
		else                equal(returned.substr(-15), "/src/index.html", "URL: "+url);
		start();
	});
	
});

asyncTest("Local URL given (leading slash)", 1, function() {
	var url = "/index.html";
	forge.tools.getURL(url, function (returned) {
		if (forge.is.web()) equal(returned.substr(-10), "index.html", "URL: "+url);
		else                equal(returned.substr(-15), "/src/index.html", "URL: "+url);
		start();
	});
	
});


asyncTest("Non-string type", 1, function() {
	var url = 1;
	forge.tools.getURL(url, function (returned) {
		ok(returned, "URL: "+url);
		start();
	});

});


module("forge.activations");

test("All frames", function() {
	// We're not on an activation test page
	if (!document.getElementById('iframe')) {
		expect(0);
		return;
	}

	ok(!!document.getElementById('setvar1'), "setvar1.js loaded in top");
	ok(!!document.getElementById('setvar2'), "setvar2.js loaded in top");

	var contentDocument = null;
	if (forge.is.ie()) {
		contentDocument = document.getElementById('iframe').contentWindow.document;
	} else {
		contentDocument = document.getElementById('iframe').contentDocument;
	}

	if (!forge.is.safari()) {
		// Safari only supports one activation => if one has all_frames = false they all do
		ok(!!contentDocument.getElementById('setvar1'), "setvar1.js loaded in iframe");
	}
	ok(!contentDocument.getElementById('setvar2'), "setvar2.js not loaded in iframe");

});

