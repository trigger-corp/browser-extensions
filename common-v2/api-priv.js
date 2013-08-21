var nullFunction = function () {};
/**
 * Override the internal.priv.call function to actually call a
 * method on the underlying private API, rather than serialize
 * and dispatch a method call.
 *
 * @param {string} methodName Name of the API method.
 * @param {*} params Key-values to pass to privileged code.
 * @param {function(*)} success Called if native method is successful.
 * @param {function({message: string}=} error
 */
internal.priv.call = function (methodName, params, success, error) {
	var strparams = "parameters could not be stringified";
	try {
		strparams = JSON.stringify(params);
	} catch (e) { }
	forge.logging.debug("Received call to "+methodName+" with parameters: "+strparams);

	if (!success) {
		success = nullFunction;
	}
	if (!error) {
		error = nullFunction;
	}
	if (typeof methodName === 'undefined') {
		return error("internal.priv.call methodName is undefined");
	}
	// descend into API with dot-separated names
	var methodParts = methodName.split('.'),
	method = apiImpl;

	for (var i=0; i<methodParts.length && method; i++) {
		method = method[methodParts[i]];
	}

	if (typeof method !== 'function') {
		// tried to call non-existent API method
		return error({message: methodName+' does not exist', type: 'UNAVAILABLE'});
	}
	
	method(params, function () {
		if (methodName === "logging.log") {
			// Don't recurse the logger
		} else {
			var strargs = "arguments could not be stringified";
			try {
				strargs = JSON.stringify(arguments);
			} catch (e) { }
			forge.logging.debug('Call to '+methodName+'('+strparams+') success: '+strargs);
		}
		success.apply(this, arguments);
	}, function () {
		var strargs = "arguments could not be stringified";
		try {
			strargs = JSON.stringify(arguments);
		} catch (e) { }
		if (methodName === "logging.log") {
			// Don't recurse the logger
			alert('Call to '+methodName+'('+strparams+') failed: '+strargs);
		} else {
			forge.logging.warning('Call to '+methodName+'('+strparams+') failed: '+strargs);
		}
		error.apply(this, arguments);
	});
};

/**
 * Only available in the foreground.
 * @param {function({message: string}=} error
 */
forge.tabs.closeCurrent = function (error) {
	var msg = 'tabs.closeCurrent is intended to be used from foreground';
	error({message: msg, type: 'UNAVAILABLE'});
	forge.logging.error('Call to tabs.closeCurrent failed: '+msg);
};

//need to override this to prevent duplicate messages in the background
logMessage = function(message, level) {
	if ('logging' in forge.config) {
		var eyeCatcher = forge.config.logging.marker || 'FORGE';
	} else {
		var eyeCatcher = 'FORGE';
	}
	message = '[' + eyeCatcher + ' BG' + '] '
			+ (message.indexOf('\n') === -1 ? '' : '\n') + message;

	// Also log to the console if it exists.
	if (typeof console !== "undefined") {
		switch (level) {
			case 10:
				if (console.debug !== undefined && !(console.debug.toString && console.debug.toString().match('alert'))) {
					console.debug(message);
				}
				break;
			case 30:
				if (console.warn !== undefined && !(console.warn.toString && console.warn.toString().match('alert'))) {
					console.warn(message);
				}
				break;
			case 40:
			case 50:
				if (console.error !== undefined && !(console.error.toString && console.error.toString().match('alert'))) {
					console.error(message);
				}
				break;
			default:
			case 20:
				if (console.info !== undefined && !(console.info.toString && console.info.toString().match('alert'))) {
					console.info(message);
				}
				break;
		}
	}
}
