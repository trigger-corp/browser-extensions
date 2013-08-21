QUnit.load();

QUnit.begin(function () {
		forge.logging.log("<TESTSTART>");
});

QUnit.done(function (details) {
	forge.logging.log(JSON.stringify({type:"<TESTDONE>", results: details}));
});

setTimeout(function () {
	QUnit.start();
}, 100);