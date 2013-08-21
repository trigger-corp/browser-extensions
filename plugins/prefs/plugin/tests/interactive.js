module("forge.prefs");

asyncTest("Set and get a pref", 1, function() {
	var pref = "test"+Math.random();
	var value = prompt("Enter a value");
	forge.prefs.set(pref, value, function () {
		forge.prefs.get(pref, function (newValue) {
			ok(confirm("Was '"+newValue+"' your value?"));
			start();
		});
	});
});