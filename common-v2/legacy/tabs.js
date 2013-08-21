/**
 * Generate a regex (as a string) for a chrome match pattern
 */
var patternToReStr = function (str) {
	if (str == '<all_urls>') {
		str = '*://*'
	}
	str = str.split('://');
	var scheme = str[0];
	var host, path;
	if (str[1].indexOf('/') === -1) {
		host = str[1];
		path = '';
	} else {
		host = str[1].substring(0, str[1].indexOf('/'));
		path = str[1].substring(str[1].indexOf('/'));
	}

	var re = '';

	// Scheme
	if (scheme == '*') {
		re += '.*://';
	} else {
		re += scheme+'://';
	}

	// Host
	if (host == '*') {
		re += '.*';
	} else if (host.indexOf('*.') === 0) {
		re += '(.+\.)?'+host.substring(2);
	} else {
		re += host;
	}
	
	// Path
	re += path.replace(/\*/g, '.*');
	
	return "^"+re+"$";
};

forge['tabs'] = {
	/**
	 * Open a new browser window, or (on mobile) a modal view.
	 *
	 * @param {string} url The URL to open in the new window.
	 * @param {boolean} if true retains focus in the tab making the call (browser only)
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */
	'open': function (url, keepFocus, success, error) {
		if (typeof keepFocus === 'function') {
			// keepFocus boolean not passed as argument
			error = success;
			success = keepFocus;
			keepFocus = false; //default is to give focus to the new tab
		}

		internal.priv.call("tabs.open", {
			url: url,
			keepFocus: keepFocus
		}, success, error);
	},
	/**
	 * Open a new browser window, or (on mobile) a modal view. With options as an object
	 *
	 * @param {object} options Options
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */	
	'openWithOptions': function (options, success, error) {
		if (!("url" in options)) {
			return error({
				message: "No URL specified",
				type: "UNEXPECTED_FAILURE"
			});
		}
		if (options.pattern) {
			options.pattern = patternToReStr(options.pattern);
		}
		internal.priv.call("tabs.open", options, success, error);
	},
	
	/**
	 * Close the tab that makes the call, intended to be called from foreground
	 * @param {function({message: string}=} error
	 */
	'closeCurrent': function (error) {
		error = arguments[1] || error;
		var hash = forge.tools.UUID();
		location.hash = hash;

		internal.priv.call("tabs.closeCurrent", {
			hash: hash
		}, null, error);
	}
};
