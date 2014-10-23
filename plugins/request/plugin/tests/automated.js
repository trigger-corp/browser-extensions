module("forge.request.ajax");

// URL to a page which returns REQUEST_METHOD, HTTP headers, COOKIE, GET and POST data as a JSON object.
var testRoot = 'http://localhost/ops_webmynd_com/tests/';
var testData = testRoot + 'data.php';
var testCookie = testRoot + 'cookie.php';
var testJson = testRoot + 'test.json';
var testXml = testRoot + 'test.xml';
var test404 = testRoot + 'notthere.html';
var testSlow = testRoot + 'slow.php';
var testHttps = 'https://trigger.io';

asyncTest("Simple HTTPS", 1, function() {
	forge.request.ajax({
		url: testHttps,
		success: function (data) {
			ok(true, "Success");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("Simple GET request", 1, function() {
	forge.request.ajax({
		url: testData,
		success: function (data) {
			data = JSON.parse(data);
			equal(data.REQUEST_METHOD, "GET", "Check request method");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("GET with data", 2, function() {
	forge.request.ajax({
		url: testData,
		data: {test: "hello"},
		success: function (data) {
			data = JSON.parse(data);
			equal(data.REQUEST_METHOD, "GET", "Check request method");
			equal(data.GET_test, "hello", "Check GET data");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("GET with data to be merged", 3, function() {
	forge.request.ajax({
		url: testData+'?data=abc',
		data: {test: "hello"},
		success: function (data) {
			data = JSON.parse(data);
			equal(data.REQUEST_METHOD, "GET", "Check request method");
			equal(data.GET_test, "hello", "Check merged GET data");
			equal(data.GET_data, "abc", "Check original GET data");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("GET with header", 2, function() {
	forge.request.ajax({
		url: testData,
		headers: {"X-Test": "hello"},
		success: function (data) {
			data = JSON.parse(data);
			equal(data.REQUEST_METHOD, "GET", "Check request method");
			equal(data['HTTP_X_TEST'], "hello", "Check header");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("POST with data", 2, function() {
	forge.request.ajax({
		url: testData,
		data: {test: "hello"},
		type: "POST",
		success: function (data) {
			data = JSON.parse(data);
			equal(data.REQUEST_METHOD, "POST", "Check request method");
			equal(data.POST_test, "hello", "Check POST data");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("POST with header", 2, function() {
	forge.request.ajax({
		url: testData,
		headers: {"X-Test": "hello"},
		data: {test: "Test"},
		type: "POST",
		success: function (data) {
			data = JSON.parse(data);
			equal(data.REQUEST_METHOD, "POST", "Check request method");
			equal(data['HTTP_X_TEST'], "hello", "Check POST header");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("POST with data string", 2, function() {
	forge.request.ajax({
		url: testData,
		data: JSON.stringify({test: "hello"}),
		type: "POST",
		success: function (data) {
			data = JSON.parse(data);
			equal(data.REQUEST_METHOD, "POST", "Check request method");
			equal(data.RAW_POST, JSON.stringify({test: "hello"}), "Check raw POST data");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

if (!forge.is.firefox()) { // Not supported by jetpack currently
	asyncTest("PUT with data string", 2, function() {
		forge.request.ajax({
			url: testData,
			data: JSON.stringify({test: "hello"}),
			type: "PUT",
			success: function (data) {
				data = JSON.parse(data);
				equal(data.REQUEST_METHOD, "PUT", "Check request method");
				equal(data.RAW_POST, JSON.stringify({test: "hello"}), "Check raw POST data");
				start();
			},
			error: function () {
				ok(false, "Ajax error callback");
				start();
			}
		});
	});
}

asyncTest("Set cookie", 1, function() {
	forge.request.ajax({
		url: testCookie,
		success: function (data) {
			ok(true, "Success");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("Check cookie data", 1, function() {
	forge.request.ajax({
		url: testData,
		data: {test: "hello"},
		success: function (data) {
			data = JSON.parse(data);
			equal(data.COOKIE_TestCookie, "Test", "Cookie value");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

/*

Disabled as cookies + cors don't seem to play nice

if (forge.is.mobile()) {
	asyncTest("Check cookie data (jQuery)", 1, function() {
		$.ajax({
			url: testData,
			data: {test: "hello"},
			success: function (data) {
				data = JSON.parse(data);
				equal(data.COOKIE_TestCookie, "Test", "Cookie value");
				start();
			},
			error: function () {
				ok(false, "Ajax error callback");
				start();
			}
		});
	});
}
*/

asyncTest("DataType JSON as JSON", 1, function() {
	forge.request.ajax({
		url: testJson,
		dataType: "json",
		success: function (data) {
			equal(data.test, "data", "Check parsed value");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("DataType JSON as text", 1, function() {
	forge.request.ajax({
		url: testJson,
		dataType: "text",
		success: function (data) {
			equal(data.replace(/\n/g,''), '{"test": "data"}', "Check parsed value");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("DataType XML as XML", 1, function() {
	forge.request.ajax({
		url: testXml,
		dataType: "xml",
		success: function (data) {
			equal($(data.firstChild).text(), "data", "Check parsed value");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("DataType XML as text", 1, function() {
	forge.request.ajax({
		url: testXml,
		dataType: "text",
		success: function (data) {
			equal(data.replace(/\n/g,''), '<test>data</test>', "Check parsed value");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("Simple 404", 2, function() {
	forge.request.ajax({
		url: test404,
		success: function (data) {
			ok(false, "Success");
			start();
		},
		error: function (data) {
			ok(true, "Ajax error callback -> " + JSON.stringify(data));
			equal(data.statusCode, "404", "Correct status code");
			start();
		}
	});
});

asyncTest("Bad URL", 1, function() {
	forge.request.ajax({
		url: "badsdfgsdf:///url",
		success: function (data) {
			ok(false, "Success");
			start();
		},
		error: function () {
			ok(true, "Ajax error callback");
			start();
		}
	});
});

asyncTest("Slow GET (2s)", 1, function() {
	forge.request.ajax({
		url: testSlow,
		success: function (data) {
			ok(true, "Success");
			start();
		},
		error: function () {
			ok(false, "Ajax error callback");
			start();
		}
	});
});

asyncTest("Slow GET Timeout (1s)", 1, function() {
	forge.request.ajax({
		url: testSlow,
		timeout: 1000,
		success: function (data) {
			ok(false, "Page loaded, should have timed out.");
			start();
		},
		error: function () {
			ok(true, "Ajax error callback");
			start();
		}
	});
});

// TODO Support fixtures for non-inspector test-apps
if ("inspector" in forge) {
	asyncTest("File upload", 1, function() {
		forge.request.ajax({
			url: testRoot + "upload.php",
			files: [forge.inspector.getFixture("request", "test.txt")],
			success: function (data) {
				equal(data.substring(0,4), 'test', "Uploaded value");
				start();
			},
			error: function () {
				ok(false, "Ajax error callback");
				start();
			}
		});
	});
}
