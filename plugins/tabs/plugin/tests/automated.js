module("forge.tabs");

asyncTest("openWithOptions - fail", 1, function() {
	forge.tabs.openWithOptions({
	
	}, function () {
		ok(false);
		start();
	}, function () {
		ok(true);
		start();
	});
});

/*

Not sure how to test this

asyncTest("openWithOptions - trusted remote", 1, function() {
	forge.tabs.openWithOptions({
		url: "http://localhost/ops_webmynd_com/tests/trusted.html",
		pattern: "https://trigger.io/*"
	}, function (ret) {
		equal(ret.userCancelled, false);
		start();
	}, function () {
		ok(false);
		start();
	});
});
*/
