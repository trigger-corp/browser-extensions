forge['notification'] = {
	/**
	 * Create a standard un-customized notification.
	 *
	 * @param {string} title
	 * @param {string} text
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */
	'create': function (title, text, success, error) {
		internal.priv.call("notification.create", {
			title: title,
			text: text
		}, success, error);
	},

	/**
	 * Set the badge number for the application icon.
	 * Setting the badge number to 0 removes the badge.
	 *
	 * @param {number} number
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */
	'setBadgeNumber': function (number, success, error) {
		internal.priv.call("notification.setBadgeNumber", {
			number: number
		}, success, error);
	}
};
