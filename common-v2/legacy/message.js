forge['message'] = {
	/**
	 * Add a listener for broadcast messages sent by other pages where your extension is active.
	 *
	 * @param {string} type (optional) an arbitrary string: if included, the callback will only be fired
	 *  for messages broadcast with the same type; if omitted, the callback will be fired for all messages
	 * @param {function(*, function(*))=} callback
	 * @param {function({message: string}=} error
	 */
	'listen': function(type, callback, error) {
		error && error({
			message: 'Forge Error: message.listen must be overridden by platform specific code',
			type: 'UNAVAILABLE'
		});
	},

	/**
	 * Broadcast a message to all other pages where your extension is active.
	 *
	 * @param {string} type an arbitrary string which limits the listeners which will receive this message
	 * @param {*} content the message body which will be passed into active listeners
	 * @param {function(*)=} callback
	 * @param {function({message: string}=} error
	 */
	'broadcast': function(type, content, callback, error) {
		error && error({
			message: 'Forge Error: message.broadcast must be overridden by platform specific code',
			type: 'UNAVAILABLE'
		});
	},

	/**
	 * Broadcast a message to listeners set up in your background code.
	 *
	 * @param {string} type an arbitrary string which limits the listeners which will receive this message
	 * @param {*} content the message body which will be passed into active listeners
	 * @param {function(*)=} callback
	 * @param {function({message: string}=} error
	 */
	'broadcastBackground': function(type, content, callback, error) {
		error && error({
			message: 'Forge Error: message.broadcastBackground must be overridden by platform specific code',
			type: 'UNAVAILABLE'
		});
	},
	
	/**
	 * Send a message to just the currently focussed browser tab.
	 *
	 * @param {string} type an arbitrary string which limits the listeners which will receive this message
	 * @param {*} content the message body which will be passed into active listeners
	 * @param {function(*)=} callback
	 * @param {function({message: string}=} error
	 */
	'toFocussed': function(type, content, callback, error) {
		internal.priv.call("message.toFocussed", {
			type: type,
			content: content
		}, callback, error);
	}
};