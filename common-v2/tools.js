forge['tools'] = {
	/**
	 * Creates an RFC 4122 compliant UUID.
	 *
	 * http://www.rfc-archive.org/getrfc.php?rfc=4122
	 *
	 * @return {string} A new UUID.
	 */
	'UUID': function() {
		// Implemented in JS on all platforms. No point going to native for this.
		return "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx".replace(/[xy]/g, function(c) {
			var r = Math.random() * 16 | 0;
			var v = c == "x" ? r : (r & 0x3 | 0x8);
			return v.toString(16);
		}).toUpperCase();
	},
	/**
	 * Resolve this name to a fully-qualified local or remote resource.
	 * The resource is not checked for existence.
	 * This method does not load the resource. For that, use "getPage()".
	 *
	 * For example, unqualified name: "my/resource.html"
	 * On Chrome: "chrome-extension://djggepjbfnnmhppnebibkbomfmnmkjln/my/resource.html"
	 *
	 * @param {string} resourceName Unqualified resource.
	 * @param {function(string)=} success Response data
	 * @param {function({message: string}=} error
	 */
	'getURL': function(resourceName, success, error) {
		internal.priv.call("tools.getURL", {
			name: resourceName.toString()
		}, success, error);
	}
};
