module("forge.document");

if (!forge.is.safari()) {
	asyncTest("Simple notification", 1, function() {
		forge.notification.create("title", "message content", function () {
			askQuestion("Did you see a notification with title 'title' and message 'message content'?", {
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
}

