/*
 * api-safari.js
 * 
 * Safari specific overrides to the generic Forge api.js
 */

/*
 * Set up listener through which all content script <-> background app
 * communication will happen
 */

var msgListeners = {};

internal.dispatchMessage = function(msgEvt) {
	// Received all messages, filter by those relevant to this addon
	if (msgEvt.name == ('forge-bridge'+forge.config.uuid)) {
		
		var msg = msgEvt.message;
		
		if (msg.method && msg.method === 'message') {
			// Handle incoming messages
			if (msgListeners[msg.event]) {
				msgListeners[msg.event].forEach(function (cb) {
					cb(msg.data);
				});
			}
		} else {
			internal.priv.receive(msg);
		}
	}
}

safari.self.addEventListener("message", internal.dispatchMessage, false);

internal.priv.send = function (data) {
	if (safari.self.tab) {
		safari.self.tab.dispatchMessage(('forge-bridge'+forge.config.uuid), data);
	} else {
		// Being called within a popover, call the dispatch function directly
		safari.extension.globalPage.contentWindow.forge._dispatchMessage({
			name: ('forge-bridge'+forge.config.uuid),
			message: data,
			isPopover: true,
			callback: function(reply, replyId) {
				if(replyId && msgListeners[replyId]){
						msgListeners[replyId].forEach(function (cb) {
							cb(reply);
						});
				}
				internal.priv.receive(JSON.stringify(reply));
			}
		});
	}
};

// Override default implementations for some methods
forge.is.desktop = function () {
	return true;
};
forge.is.safari = function () {
	return true;
}

// Messaging helper functions
var safariListen = function (event, cb) {
	if (msgListeners[event]) {
		msgListeners[event].push(cb);
	} else {
		msgListeners[event] = [cb];
	}
};
var safariBroadcast = function (event, data) {
	internal.priv.call("message", {event: event, data: data});
};

// Override default messaging methods
forge.message.listen = function (type, callback, error) {
	if (typeof(type) === 'function')
	{ // no type passed in: shift arguments left one
		error = callback;
		callback = type;
		type = null;
	}
	
	safariListen("broadcast", function(message) {
		if (type === null || type === message.type) {
			callback(message.content, function(reply) {
				if (message.uuid) {
					safariBroadcast(message.uuid, reply);
				}
			});
		}
	});
};
forge.message.broadcastBackground = function (type, content, callback, error) {
	var msgUUID = forge.tools.UUID();

	safariListen(msgUUID, function (data) {
		callback(data);
	});

	safariBroadcast('broadcastBackground', {
		type: type,
		content: content,
		uuid: msgUUID
	});
};
forge.message.broadcast = function (type, content, callback, error) {
	var msgUUID = forge.tools.UUID();

	safariListen(msgUUID, function (data) {
		callback(data);
	});

	safariBroadcast('broadcast', {
		type: type,
		content: content,
		uuid: msgUUID
	});
};
forge.message.toFocussed = function (type, content, callback, error) {
	var msgUUID = forge.tools.UUID();

	//set up reply handler
	safariListen(msgUUID, function (data) {
		callback(data);
	});

	safariBroadcast('toFocussed', {
		type: type,
		content: content,
		uuid: msgUUID
	});
};

forge['_dispatchMessage'] = internal.dispatchMessage;
