/*
 * api-firefox.js
 *
 * Firefox specific overrides to the generic Forge api.js
 */

// Copy reference to objects temporarily placed in window._forge
if (self && self.on) {
	// In a content script we have direct access to self
	// In a local page this will be set for us by api-proxy.js
	internal.self = self;
} else if (window._forge && window._forge.self) {
	internal.self = window._forge.self;
}

if (window._forge && window._forge.background) {
	internal.background = true;
}
// Remove this unused global variable (we try to only expose window.forge)
delete window._forge;

// Setup listener for incoming messages
var msgListeners = {};
internal.self.on("message", function(msg) {
	if (msg.type && msg.type == 'message') {
		// Handle incoming forge.message objects
		if (msgListeners[msg.event]) {
			msgListeners[msg.event].forEach(function (cb) {
				cb(msg.data);
			});
		}
	} else if (msg.type && msg.type == 'css') {
		// Inject CSS into the page
		var addStyles = function () {
			msg.files.forEach(function (file) {
				var head = document.getElementsByTagName('head')[0],
					style = document.createElement('style'),
					rules = document.createTextNode(file);

				style.type = 'text/css';
				if (style.styleSheet) {
					style.styleSheet.cssText = rules.nodeValue;
				} else {
					style.appendChild(rules);
				}
				head.appendChild(style);
			});
		};
		if (window.forge._disableFrames === undefined || window.location == window.parent.location) {
			if (document.readyState === "loading") {
				document.addEventListener("DOMContentLoaded", addStyles, false);
			} else {
				addStyles();
			}
		}
	} else {
		// Standard API objects, deal with in platform-agnostic code
		internal.priv.receive(msg);
	}
});

// Send object to privileged code
internal.priv.send = function(data) {
	internal.self.postMessage(data, "*");
}

forge.is.desktop = function() {
	return true;
}

forge.is.firefox = function() {
	return true;
}

// Messaging helpers
var jetpackListen = function (event, cb) {
	if (msgListeners[event]) {
		msgListeners[event].push(cb);
	} else {
		msgListeners[event] = [cb];
	}
};
var jetpackBroadcast = function (event, data) {
	internal.priv.call("message", {event: event, data: data});
};

forge.message = {
	listen: function(type, callback, error) {
		if (typeof(type) === 'function')
		{ // no type passed in: shift arguments left one
			error = callback;
			callback = type;
			type = null;
		}

		var event = internal.background ? 'broadcastBackground' : 'broadcast';

		jetpackListen(event, function(message) {
			if (type === null || type === message.type) {
				callback(message.content, function(reply) {
					if (message.uuid) {
						jetpackBroadcast(message.uuid, reply);
					}
				});
			}
		});
	},
	broadcastBackground: function(type, content, callback, error) {
		var msgUUID = forge.tools.UUID();
		jetpackBroadcast('broadcastBackground', {
			type: type,
			content: content,
			uuid: msgUUID
		});
		jetpackListen(msgUUID, function (data) {
			callback(data);
		});
	},
	broadcast: function(type, content, callback, error) {
		var msgUUID = forge.tools.UUID();
		jetpackBroadcast('broadcast', {
			type: type,
			content: content,
			uuid: msgUUID
		});
		jetpackListen(msgUUID, function (data) {
			callback(data);
		});
	},
	toFocussed: function(type, content, callback, error) {
		var msgUUID = forge.tools.UUID();
		jetpackBroadcast('toFocussed', {
			type: type,
			content: content,
			uuid: msgUUID
		});
		jetpackListen(msgUUID, function (data) {
			callback(data);
		});
	}
}

forge.request.ajax = function(options) {
	var url = (options.url ? options.url : null);
	var success = (options.success ? options.success : undefined);
	var error = (options.error ? options.error : undefined);
	var username = (options.username ? options.username : null);
	var password = (options.password ? options.password : null);
	var accepts = (options.accepts ? options.accepts : ["*/*"]);
	var cache = (options.cache ? options.cache : false);
	var contentType = (options.contentType ? options.contentType : null);
	var data = (options.data ? options.data : null);
	var dataType = (options.dataType ? options.dataType : null);
	var headers = (options.headers ? options.headers : {});
	var timeout = (options.timeout ? options.timeout : 60000);
	var type = (options.type ? options.type : 'GET');

	// Tidy up possibly-common mistakes.
	if (typeof accepts === "string")
	{
		// Given "text/html" instead of ["text/html"].
		accepts = [accepts];
	}

	if (type == 'GET') {
		url = internal.generateURI(url, data);
		data = null;
	}
	if (cache) {
		cache = {};
		cache['wm'+Math.random()] = Math.random();
		url = internal.generateURI(url, cache);
	}
	if (data) {
		data = internal.generateQueryString(data);
		if (!contentType) {
			contentType = "application/x-www-form-urlencoded";
		}
	}
	if (accepts && !headers['Accept']) {
		headers['Accept'] = accepts.join(',');
	}
	if (contentType) {
		headers['Content-Type'] = contentType;
	}

	internal.priv.call("request.ajax", {
		url: url,
		username: username,
		password: password,
		data: data,
		headers: headers,
		type: type,
		timeout: timeout
	}, function (data) {
		try {
			if (dataType == 'xml') {
				// Borrowed from jQuery.
				var tmp, xml;
				if ( window.DOMParser ) { // Standard
					tmp = new DOMParser();
					xml = tmp.parseFromString(data , "text/xml");
				} else { // IE
					xml = new ActiveXObject( "Microsoft.XMLDOM" );
					xml.async = "false";
					xml.loadXML(data);
				}

				data = xml;
			} else if (dataType == 'json') {
				data = JSON.parse(data);
			}
		} catch (e) {
		}
		success(data);
	}, error);
}

forge.file.string = function (file, success, error) {
	internal.priv.call("file.string", file, success, error);
};
