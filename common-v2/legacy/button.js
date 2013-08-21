forge['button'] = {
	/**
	 * Change the URL of the icon displayed in the browser button
	 * 
	 * @param {string} url URL to the icon
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */
	'setIcon': function (url, success, error) {
		internal.priv.call("button.setIcon", url, success, error);
	},

	/**
	 * Change the URL of the HTML page shown when a user clicks on the browser button
	 *
	 * @param {string} url URL to the HTML page
	 * @param {function()=} success Callback
	 * @param {function({message: string}=} error
	 */
	'setURL': function (url, success, error) {
		internal.priv.call("button.setURL", url, success, error);
	},

	/**
	 * Add a listener for when the toolbar button is clicked
	 * 
	 * @param {function()} callback function to invoked when the button is clicked
	 */
	'onClicked': {
		'addListener': function (callback) {
			internal.priv.call("button.onClicked.addListener", null, callback);
		}
	},

	/**
	 * Changes the toolbar button unread count
	 * 
	 * @param {number} number New count, 0 to clear
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */
	'setBadge': function (number, success, error) {
		internal.priv.call("button.setBadge", number, success, error);
	},

	/**
	 * Changes the toolbar button color
	 * 
	 * @param {Array.<number>} colorArray an array of four integers in the range [0,255]
	 * 			that make up the RGBA color of the badge. For example, opaque red is [255, 0, 0, 255].
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */
	'setBadgeBackgroundColor': function (colorArray, success, error) {
		internal.priv.call("button.setBadgeBackgroundColor", colorArray, success, error);
	},

	/**
	 * Set the tooltip text
	 *
	 * @param {string} title text to set as the tooltip
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */
	'setTitle': function (title, success, error) {
		internal.priv.call("button.setTitle", title, success, error);
	}
};