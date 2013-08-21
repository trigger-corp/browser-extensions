module("forge.tabs");



asyncTest("Open tab in foreground", 1, function() {
	forge.tools.getURL("fixtures/tabs/close.html", function (url) {
		forge.tabs.open(url, function () {
			askQuestion("Did a tab/view just open in the foreground with the text 'Close me!'?", {
				Yes: function () {
					ok(true, "Success");
					start();
				},
				No: function () {
					ok(false, "User claims failure");
					start();
				}
			});
		}, function (e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
	});
});

