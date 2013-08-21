/*
 * For Chrome.
 */

chrome.extension.onConnect.addListener(function(port) {
	if (port.name === 'bridge') {
		port.onMessage.addListener(function(msg) {
			function handleResult(status) {
				return function(content) {
					// create and send response message
					var reply = {callid: msg.callid, status: status, content: content};
					port.postMessage(reply);
				}
			}
			internal.priv.call(
					msg.method, msg.params,
					handleResult('success'), handleResult('error')
			);
		});
	} else if(port.name === 'message:to-non-priv') {
		// Special request from non-priv to pass this message on to other non-priv pages
		port.onMessage.addListener(function(message) {
			forge.message.broadcast(message.type, message.content, function (reply) {
				port.postMessage(reply);
			});
		});
	}
});


/**
 * Add a listener for broadcast messages sent by non-privileged pages where your app is active.
 *
 * NOTE: unlike other methods, we assume that the message.listen method has been overriden
 * separately at the non-privilged level.
 *
 * @param {string} type (optional) an arbitrary string: if included, the callback will only be fired
 *  for messages broadcast with the same type; if omitted, the callback will be fired for all messages
 * @param {function} callback a function which will be called
 *  with the contents of relevant broadcast messages as
 *  its first argument
 * @param {function} error Not used.
 */
forge.message.listen = function(type, callback, error)
{
	if (typeof(type) === 'function') {
		// no type passed in: shift arguments left one
		error = callback;
		callback = type;
		type = null;
	}

	chrome.extension.onConnect.addListener(function(port) {
		if (port.name === 'message:to-priv') {
			port.onMessage.addListener(function(message) {
				if (type === null || type === message.type) {
					callback && callback(message.content, function (reply) {
						// send back reply
						port.postMessage(reply);
					});
				}
			});
		}
	});
};
/**
 * Broadcast a message to all non-privileged pages where your extension is active.
 *
 * NOTE: this method requires the "tabs" permissions in your app configuration.
 *
 * @param {string} type an arbitrary string which limits the listeners which will receive this message
 * @param {*} content the message body which will be passed into active listeners
 * @param {function} callback reply function
 * @param {function} error Not used.
 */
forge.message.broadcast = function(type, content, callback, error) {
	chrome.windows.getAll({populate: true}, function (windows) {
		windows.forEach(function (win) {
			win.tabs.forEach(function (tab) {
				//skip hosted pages
				if (tab.url.indexOf('chrome-extension:') != 0) {
					var port = chrome.tabs.connect(tab.id);
					if (callback) {
						port.onMessage.addListener(function (message) {
							// this is a reply from a recipient non-privileged page
							callback(message);
						});
					}
					port.postMessage({type: type, content: content});
				}
			})
		});
	});
	// Also to popup
	var port = chrome.extension.connect();
	if (callback) {
		port.onMessage.addListener(function(message) {
			// this is a reply from a recipient non-privileged page
			callback(message);
		});
	}
	port.postMessage({type: type, content: content});
}

/**
 * @return {boolean}
 */
forge.is.desktop = function() {
	return true;
};

/**
 * @return {boolean}
 */
forge.is.chrome = function() {
	return true;
};

// Private API implementation
var apiImpl = {
	message: {
		toFocussed: function(params, callback, error) {
			chrome.windows.getCurrent(function (win) {
				chrome.tabs.getSelected(win.id, function (tab) {
					var port = chrome.tabs.connect(tab.id);
					port.onMessage.addListener(function (message) {
						// this is a reply from the focussed non-privileged page
						callback(message);
					});
					port.postMessage({type: params.type, content: params.content});
				});
			});
		}
	},
	notification: {
		create: function(params, success, error) {
			// 0 is PERMISSION_ALLOWED
			if (window.webkitNotifications.checkPermission() == 0) {
				var notification = window.webkitNotifications.createNotification(
						'', // icon
						params.title,
						params.text
				);
				notification.show();
				success();
			} else {
				error({
					message: "No permission for notifications, make sure notifications is included in permissions array of the configuration file",
					type: "UNAVAILABLE"
				});
			}
		}

	},
	prefs: {
		get: function (params, success, error) {
			success(window.localStorage.getItem(params.key));
		},
		set: function (params, success, error) {
			success(window.localStorage.setItem(params.key, params.value));
		},
		keys: function (params, success, error) {
			var keys = [];
			for (var i=0; i<window.localStorage.length; i++) {
				keys.push(window.localStorage.key(i));
			}
			success(keys);
		},
		all: function (params, success, error) {
			var all = {};
			for (var i=0; i<window.localStorage.length; i++) {
				var key = window.localStorage.key(i)
				all[key] = window.localStorage.getItem(key);
			}
			success(all);
		},
		clear: function (params, success, error) 	{
			success(window.localStorage.removeItem(params.key));
		},
		clearAll: function (params, success, error) {
			success(window.localStorage.clear());
		}
	},
	request: {
		/**
		 * Implementation of api.ajax
		 *
		 * success and error callbacks are taken from positional arguments,
		 * not from the options.success and options.error values.
		 */
		ajax: function (params_, success, error) {
			// Copy params to prevent overwriting of original success/error
			var params = $.extend({}, params_);
			params.success = success;
			params.error = function (xhr, status, err) {
				error({
					message: 'api.ajax with '+JSON.stringify(params)+'failed. '+status+': '+err,
					type: 'EXPECTED_FAILURE',
					status: status,
					statusCode: xhr.status,
					err: err
				});
			}
			$.ajax(params);
		}
	},
	tabs: {
		open: function (params, success, error) {
			chrome.tabs.create({url: params.url, selected: !params.keepFocus}, success);
		},
		closeCurrent: function (params, success, error) {
			if (params.hash) {
				var window, tab, url;

				chrome.windows.getAll({populate: true}, function (windows) {
					while (windows.length) {
						window = windows.pop();

						chrome.tabs.getAllInWindow(window.id, function (tabs) {
							while (tabs.length) {
								tab = tabs.pop();
								url = tab.url;

								if (url.indexOf(params.hash) > 0) {
									chrome.tabs.remove(tab.id);
									success();
									return;
								}
							}
						})
					}
				})
			} else {
				error({
					message: 'hash was not passed to as part of params to closeCurrent',
					type: 'UNEXPECTED_FAILURE'
				});
			}
		}
	},
	button: {
		setIcon: function (url, success, error) {
			// NOTE: When setIcon() is called, it specifies the relative path to an image inside the extension
			chrome.browserAction.setIcon({ "path": url } );
			success();
		},
		setURL: function (url, success, error) {
			if(url.length > 0) {
				if (url.indexOf('/') === 0) {
					url = 'src' + url;
				} else {
					url = 'src/' + url;
				}
			}

			chrome.browserAction.setPopup({popup: url});
			success();
		},
		onClicked: {
			addListener: function (params, callback, error) {
				chrome.browserAction.setPopup({popup: ''});
				chrome.browserAction.onClicked.addListener(callback);
			}
		},
		setBadge: function (number, success, error) {
			chrome.browserAction.setBadgeText({text: (number) ? number.toString() : ''});
			success();
		},
		setBadgeBackgroundColor: function (colorArray, success, error) {
			if (colorArray instanceof Array) {
				colorArray = {color:colorArray};
			}

			chrome.browserAction.setBadgeBackgroundColor(colorArray);
			success();
		},
		setTitle: function(title, success, error) {
			success(chrome.browserAction.setTitle({title: title}));
		}
	},
	logging: {
		log: function(params, success, error) {
			if (typeof console !== "undefined") {
				switch (params.level) {
					case 10:
						if (console.debug !== undefined && !(console.debug.toString && console.debug.toString().match('alert'))) {
							console.debug(params.message);
						}
						break;
					case 30:
						if (console.warn !== undefined && !(console.warn.toString && console.warn.toString().match('alert'))) {
							console.warn(params.message);
						}
						break;
					case 40:
					case 50:
						if (console.error !== undefined && !(console.error.toString && console.error.toString().match('alert'))) {
							console.error(params.message);
						}
						break;
					default:
					case 20:
						if (console.info !== undefined && !(console.info.toString && console.info.toString().match('alert'))) {
							console.info(params.message);
						}
						break;
				}
			}
			success();
		}
	},
	tools: {
		getURL: function(params, success, error) {
 			name = params.name.toString();
			if (name.indexOf("http://") === 0 || name.indexOf("https://") === 0) {
 				success(name);
			} else {
				success(chrome.extension.getURL('src'+(name.substring(0,1) == '/' ? '' : '/')+name));
			}
		}
	}
}
