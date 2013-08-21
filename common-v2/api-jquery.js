// Forge jQuery reference

var $ = window.jQuery.noConflict(true);

// Really remove jQuery globals (affects Zepto and probably others)
if (window.jQuery && window.jQuery === undefined) {
	delete window.jQuery;
}
if (window.$ && window.$ === undefined) {
	delete window.$;
}

// Never treat AJAX requests as cross domain
$.ajaxPrefilter(function (options) {
	if (options.crossDomain) {
		options.crossDomain = false;
	}
});