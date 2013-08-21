module("forge.button");

if (forge.is.desktop()) {
	asyncTest("Icon from config", 1, function() {
		askQuestion("Can you see a browser action button which is a red circle containing the number 1?", function () {
			ok(true, "Success");
			start();
		}, function () {
			ok(false, "User claims failure");
			start();
		});
	});

	asyncTest("Title from config", 1, function() {
		askQuestion("Does the browser action button have the tooltip 'Title 1' when you hover it?", function () {
			ok(true, "Success");
			start();
		}, function () {
			ok(false, "User claims failure");
			start();
		});
	});

	asyncTest("Popup URL from config", 1, function() {
		askQuestion("Click the button, does the page shown say 'Popup 1'?", function () {
			ok(true, "Success");
			start();
		}, function () {
			ok(false, "User claims failure");
			start();
		});
	});

	asyncTest("setBadge first time", 1, function() {
		forge.button.setBadge(1, function () {
			askQuestion("Does the button have a badge showing the number 1?", function () {
				ok(true, "Success");
				start();
			}, function () {
				ok(false, "User claims failure");
				start();
			});
		}, function (e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
	});
	
	asyncTest("setBadge second time", 1, function() {
		forge.button.setBadge(2, function () {
			askQuestion("Does the button now have a badge showing the number 2?", function () {
				ok(true, "Success");
				start();
			}, function () {
				ok(false, "User claims failure");
				start();
			});
		}, function (e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
	});
	
	if (!forge.is.safari()) {
		asyncTest("setBadge color (green)", 1, function() {
			forge.button.setBadgeBackgroundColor([0, 255, 0, 255], function () {
				askQuestion("Is the button badge now green and still displaying the number 2?", function () {
					ok(true, "Success");
					start();
				}, function () {
					ok(false, "User claims failure");
					start();
				});
			}, function (e) {
				ok(false, "API call failure: "+e.message);
				start();
			});
		});
		asyncTest("setBadge color (blue)", 1, function() {
			forge.button.setBadgeBackgroundColor([0, 0, 255, 255], function () {
				askQuestion("Is the button badge now blue and still displaying the number 2?", function () {
					ok(true, "Success");
					start();
				}, function () {
					ok(false, "User claims failure");
					start();
				});
			}, function (e) {
				ok(false, "API call failure: "+e.message);
				start();
			});
		});
	}
	
	asyncTest("setBadge to blank", 1, function() {
		forge.button.setBadge(0, function () {
			askQuestion("Does the button now have no badge?", function () {
				ok(true, "Success");
				start();
			}, function () {
				ok(false, "User claims failure");
				start();
			});
		}, function (e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
	});

	asyncTest("setTitle", 1, function() {
		forge.button.setTitle("Title 2", function () {
			askQuestion("Is the tooltip for the button now 'Title 2'?", function () {
				ok(true, "Success");
				start();
			}, function () {
				ok(false, "User claims failure");
				start();
			});
		}, function (e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
	});

	asyncTest("setIcon", 1, function() {
		var filename = forge.is.ie() ? "fixtures/button/2.ico" : "fixtures/button/2.png";
		forge.tools.getURL(filename, function (icon) {
			forge.button.setIcon(icon, function () {
				askQuestion("Does the icon for the button now display a 2 on a red background?", function () {
					ok(true, "Success");
					start();
				}, function () {
					ok(false, "User claims failure");
					start();
				});
			}, function (e) {
				ok(false, "API call failure: "+e.message);
				start();
			});
		});
	});

	asyncTest("setURL", 1, function() {
		forge.button.setURL("fixtures/button/popup2.html", function () {
			askQuestion("Click the button, does the page shown now say 'Popup 2'?", function () {
				ok(true, "Success");
				start();
			}, function () {
				ok(false, "User claims failure");
				start();
			});
		}, function (e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
	});

	asyncTest("onClicked.addListener 1 listener", 2, function() {
		var once = true;
		forge.button.onClicked.addListener(function () {
			if (once) {
				ok(true, "Click registered");
				askQuestion("No popup should have appeared.", function () {
					ok(true, "Success");
					start();
				}, function () {
					ok(false, "User claims failure");
					start();
				});
				once = false;
			}
		}, function (e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
		alert('Click the button again');
	});
	
	asyncTest("onClicked.addListener, 2 listeners", 1, function() {
		var count = 0;
		var clicked = function () {
			count++;
			if (count === 2) {
				ok(true, "Click registered twice");
				start();
			}
		
		}
		forge.button.onClicked.addListener(clicked, function (e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
		forge.button.onClicked.addListener(clicked, function (e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
		alert('Click the button again');
	});

	asyncTest("setURL", 1, function() {
		forge.button.setURL("fixtures/button/popup1.html", function () {
			askQuestion("Click the button, does the page shown say 'Popup 1' again?", function () {
				ok(true, "Success");
				start();
			}, function () {
				ok(false, "User claims failure");
				start();
			});
		}, function (e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
	});
	
	asyncTest("setURL", 1, function() {
		forge.button.setURL("", function () {
			askQuestion("Click the button, did nothing happen?", function () {
				ok(true, "Success");
				start();
			}, function () {
				ok(false, "User claims failure");
				start();
			});
		}, function (e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
	});

	// Listen for all messages
	forge.message.listen(function (content, reply) {
		reply(content);
		reply(content);
		reply(content);
	});
	
	// Listen for a specific type of message
	forge.message.listen('test', function (content, reply) {
		reply(content);
		reply(content);
		reply(content);
	});

	asyncTest("Popup messaging tests", 1, function() {
		forge.button.setURL("fixtures/button/popupmessage.html", function () {
			askQuestion("Click the button, does the page say the tests pass?", function () {
				ok(true, "Success");
				start();
			}, function () {
				ok(false, "User claims failure");
				start();
			});
		}, function (e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
	});

}
