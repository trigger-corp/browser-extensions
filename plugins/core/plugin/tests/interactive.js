module("core");

asyncTest("Simple log", 1, function() {
	forge.logging.log("Log message");
	
	askQuestion("Did you see a foreground log message with content 'Log message'?", function () {
		ok(true, "Success");
		start();
	}, function () {
		ok(false, "User claims failure");
		start();
	});
});

asyncTest("Complex log (object)", 1, function() {
	forge.logging.log({test: "data"});
	
	askQuestion("Did you see a foreground log message with content '{test: \"data\"}'?", function () {
		ok(true, "Success");
		start();
	}, function () {
		ok(false, "User claims failure");
		start();
	});
});

asyncTest("Complex log (date)", 1, function() {
	forge.logging.log(new Date());
	
	askQuestion("Did you see a foreground log message showing the current date/time?", function () {
		ok(true, "Success");
		start();
	}, function () {
		ok(false, "User claims failure");
		start();
	});
});


asyncTest("Complex log (date)", 1, function() {
	forge.logging.log($.parseXML('<data>test</data>'));
	
	askQuestion("Did you see a foreground log message showing the XML document '<data>test</data>'", function () {
		ok(true, "Success");
		start();
	}, function () {
		ok(false, "User claims failure");
		start();
	});
});
