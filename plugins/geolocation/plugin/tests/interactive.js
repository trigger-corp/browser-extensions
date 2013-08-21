module("geolocation");

asyncTest("Geolocation success", 1, function() {
	var options = {};
	function runTest() {
		forge.geolocation.getCurrentPosition(options, function(position) {
			askQuestion("Is this your correct location: " +
						position.coords.latitude + ", " +
						position.coords.longitude + "?" +
						"<br/><small>London is 51.5, 0. San Francisco is 37.7, 122.4</small>",
						{ Yes: function() {
							ok(true, "Success");
							start();
						}, No: function() {
							ok(false, "User claims failure");
							start();
						}});
		}, function(e) {
			ok(false, "API call failure: "+e.message);
			start();
		});
	}
	askQuestion("Allow geolocation, if prompted", runTest, runTest);
});
