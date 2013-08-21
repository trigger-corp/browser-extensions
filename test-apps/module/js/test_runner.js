forge.inspector = {
	getFixture: function (plugin, file) {
		var location = window.location.origin + window.location.pathname;
		var url = decodeURI(location.substring(0, location.length-10));
		url = url + 'fixtures/' + plugin + (file.substring(0, 1) == '/' ? '' : '/') + file;
		return {
			uri: url
		};
	}
};

var getPlugins = function () {
	plugins = [];
	for (var x in forge.config.plugins) {
		plugins.push(x);
	}
	for (x in forge.config.modules) {
		plugins.push(x);
	}
	return plugins;
};
var runTests = function () {
	$('body').append('<script src="js/qunit-1.10.0.js"></sc'+'ript>');
	$('body').append('<script>QUnit.config.reorder = false;QUnit.config.autostart = false;QUnit.config.autorun = false;</sc'+'ript>');
	$.each(getPlugins(), function (i, plugin) {
		$('body').append('<script src="tests/'+plugin+'.js"></sc'+'ript>');
	});
	$('body').append('<script src="js/qunit_hooks.js"></sc'+'ript>');
};
var runOnce = function (func) {
	var run = true;
	return function () {
		if (run) {
			func();
		}
		run = false;
	};
};
var askQuestion = function (question, buttons) {
	// Backwards compatible with yes/no style call
	if (arguments.length == 3) {
		buttons = {
			Yes: arguments[1],
			No: arguments[2]
		};
	}
	$('#qunit-interact').html('');
	var div = $('<div class="alert alert-info alert-block">').append($('<p style="font-size: 1.2em">').html(question));
	var btnGroup = $('<div>');
	var addButton = function (button, cb) {
		btnGroup.append($('<button style="margin: 5px;" class="btn btn-primary">').text(button).click(function () {
			$('#qunit-interact').html('');
			cb && cb()
		}));
	}
	for (var button in buttons) {
		addButton(button, buttons[button]);
	}
	div.append(btnGroup);
	$('#qunit-interact').append(div);
};

$(function () {
	runTests();
});