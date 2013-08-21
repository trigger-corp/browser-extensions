if (window._testResultClient) {
	_testResultClient.setSuiteName('activation');
}

forge.tools.getURL('qunit-git.css', function (url) {
	$('head').append('<link rel="stylesheet" href="'+url+'" type="text/css" media="screen">');
});

setTimeout(function () {
	QUnit.start();
}, 5000);
