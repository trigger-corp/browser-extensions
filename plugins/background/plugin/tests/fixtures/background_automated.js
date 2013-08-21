if (window._testResultClient) {
	_testResultClient.setSuiteName('background');
}

forge.tools.getURL("/index.html", function (url) {
	forge.tabs.open(url);
});
forge.tabs.open('http://ops.trigger.io/75d92dce/tests/empty.html');

$(function () {
	$('body').append('<h1 id="qunit-header">Forge Test Platform</h1>' +
			'<h2 id="qunit-banner"></h2>' +
			'<div id="qunit-testrunner-toolbar"></div>' +
			'<h2 id="qunit-userAgent"></h2>' +
			'<ol id="qunit-tests"></ol>' +
			'<div id="qunit-fixture">test markup, will be hidden</div>');

	setTimeout(function () {
		QUnit.start();
	}, 5000);
});
