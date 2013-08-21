/*
 * api-chrome.js
 * 
 * Chrome specific overrides to the generic Forge api.js
 */

/**
 * Set up port through which all content script <-> background app
 * communication will happen
 */
var port = chrome.extension.connect({name: 'bridge'});
port.onMessage.addListener(function (msg) {
	internal.priv.receive(msg);
});

/**
 * Calls native code from JS
 * @param {*} data Object to send to privileged/native code.
 */
internal.priv.send = function(data) {
	port.postMessage(data);
}

/**
 * @return {boolean}
 */
forge.is.desktop = function () {
	return true;
}

/**
 * @return {boolean}
 */
forge.is.chrome = function () {
	return true;
}

/**
 * Add a listener for broadcast messages sent by other pages where your extension is active.
 *
 * @param {string} type (optional) an arbitrary string: if included, the callback will only be fired
 *  for messages broadcast with the same type; if omitted, the callback will be fired for all messages
 * @param {function(*, function(*))=} callback
 * @param {function({message: string}=} error
 */
forge.message.listen = function(type, callback, error) {
	if (typeof type === 'function') {
		// no type passed in: shift arguments left one
		error = callback;
		callback = type;
		type = null;
	}

	chrome.extension.onConnect.addListener(function(port) {
		//don't want to hear anything intended for background
		if (port.name === 'message:to-priv' ||
				port.name === 'message:to-non-priv' ||
				port.name === 'bridge') {
			return;
		}

		port.onMessage.addListener(function(message) {
			if (type === null || type === message.type) {
				callback(message.content, function(reply) {
					// send back reply
					port.postMessage(reply);
				});
			}
		});
	});
}
/**
 * Broadcast a message to listeners set up in your background code.
 *
 * @param {string} type an arbitrary string which limits the listeners which will receive this message
 * @param {*} content the message body which will be passed into active listeners
 * @param {function(*)=} callback
 * @param {function({message: string}=} error
 */
forge.message.broadcastBackground = function(type, content, callback, error) {
	var port = chrome.extension.connect({name:'message:to-priv'});
	if (callback) {
		port.onMessage.addListener(function(message) {
			// one of the listeners has replied to us
			callback(message);
		})
	}
	port.postMessage({type: type, content: content});
};
/**
 * Broadcast a message to all other pages where your extension is active.
 *
 * @param {string} type an arbitrary string which limits the listeners which will receive this message
 * @param {*} content the message body which will be passed into active listeners
 * @param {function(*)=} callback
 * @param {function({message: string}=} error
 */
forge.message.broadcast = function(type, content, callback, error) {
	var port = chrome.extension.connect({name:'message:to-non-priv'});
	if (callback) {
		port.onMessage.addListener(function(message) {
			// one of the listeners has replied to us
			callback(message);
		});
	}
	port.postMessage({type: type, content: content});
};
