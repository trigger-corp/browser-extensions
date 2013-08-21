forge['prefs'] = {
	/**
	 * Get a preference stored by your application.
	 *
	 * @param {string} key The key of your preference.
	 * @param {function(string)=} success
	 * @param {function({message: string}=} error
	 */
	'get': function (key, success, error) {
		forge.internal.call("prefs.get", {
			key: key.toString()
		}, success && function (value) {
			if (value === "undefined") {
				value = undefined;
			} else {
				try {
					value = JSON.parse(value);
				} catch (e) {
					error({
						message: e.toString()
					});
					return;
				}
			}
			success(value);
		}, error);
	},
	/**
	 * Set a preference.
	 *
	 * @param {string} key The preference key.
	 * @param {string} value The preference value.
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */
	'set': function (key, value, success, error) {
		if (value === undefined) {
			value = "undefined";
		} else {
			value = JSON.stringify(value);
		}
		forge.internal.call("prefs.set", {
			key: key.toString(),
			value: value
		}, success, error);
	},
	/**
	 * Find the keys of all preferences that have been set.
	 *
	 * @param {function(Array.<string>)=} success
	 * @param {function({message: string}=} error
	  */
	'keys': function(success, error) {
		forge.internal.call("prefs.keys", {}, success, error);
	},
	/**
	 * Find the keys and values of all preferences that have been set.
	 *
	 * @param {function(Object.<string, *>)=} success
	 * @param {function({message: string}=} error
	 */
	'all': function(success, error) {
		var success = success && function (prefs) {
			for (key in prefs) {
				if (prefs[key] === "undefined") {
					prefs[key] = undefined;
				} else {
					prefs[key] = JSON.parse(prefs[key]);
				}
			}
			success(prefs);
		}
		forge.internal.call("prefs.all", {}, success, error);
	},
	/**
	 * Expunge a single persisted setting, reverting it back to its default value (if available).
	 *
	 * @param {string} key Preference to forget.
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */
	'clear': function(key, success, error) {
		forge.internal.call("prefs.clear", {
			key: key.toString()
		}, success, error);
	},
	/**
	 * Expunge all persisted settings, reverting back to defaults (if available).
	 *
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */
	'clearAll': function(success, error) {
		forge.internal.call("prefs.clearAll", {}, success, error);
	}
};