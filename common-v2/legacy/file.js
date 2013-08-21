forge['file'] = {
	/**
	 * Allow the user to select an image and give a file object representing it.
	 *
	 * @param {Object} props
	 * @param {function({uri: string, name: string})=} success
	 * @param {function({message: string}=} error
	 */
	'getImage': function (props, success, error) {
		if (typeof props === "function") {
			error = success;
			success = props;
			props = {};
		}
		if (!props) {
			props = {};
		}
		internal.priv.call("file.getImage", props, success && function (uri) {
			var file = {
				uri: uri,
				name: 'Image',
				type: 'image'
			};
			if (props.width) {
				file.width = props.width;
			}
			if (props.height) {
				file.height = props.height;
			}
			success(file)
		}, error);
	},
	/**
	 * Allow the user to select a video and give a file object representing it.
	 *
	 * @param {Object} props
	 * @param {function({uri: string, name: string})=} success
	 * @param {function({message: string}=} error
	 */
	'getVideo': function (props, success, error) {
		if (typeof props === "function") {
			error = success;
			success = props;
			props = {};
		}
		if (!props) {
			props = {};
		}
		internal.priv.call("file.getVideo", props, success && function (uri) {
			var file = {
				uri: uri,
				name: 'Video',
				type: 'video'
			};
			success(file)
		}, error);
	},
	/**
	 * Get file object for a local file.
	 *
	 * @param {string} name
	 * @param {function(string)=} success
	 * @param {function({message: string}=} error
	 */
	'getLocal': function (path, success, error) {
		forge.tools.getURL(path,
			function (url) {
				success({uri: url, name: path});
			}, error
		);
	},
	/**
	 * Get the base64 value for a files contents.
	 *
	 * @param {{uri: string, name: string}} file
	 * @param {function(string)=} success
	 * @param {function({message: string}=} error
	 */
	'base64': function (file, success, error) {
		internal.priv.call("file.base64", file, success, error);
	},
	/**
	 * Get the string value for a files contents.
	 *
	 * @param {{uri: string, name: string}} file
	 * @param {function(string)=} success
	 * @param {function({message: string}=} error
	 */
	'string': function (file, success, error) {
		forge.request.ajax({
			url: file.uri,
			success: success,
			error: error
		});
	},
	/**
	 * Get the URL an image which is no bigger than the given height and width.
	 *
	 * URL must be useable in the current scope of the code, may return a base64 data: URI.
	 *
	 * @param {{uri: string, name: string}} file
	 * @param {Object} props
	 * @param {function(string)=} success
	 * @param {function({message: string}=} error
	 */
	'URL': function (file, props, success, error) {
		if (typeof props === "function") {
			error = success;
			success = props;
		}
		// Avoid mutating original file
		var newFile = {}
		for (prop in file) {
			newFile[prop] = file[prop];
		}
		newFile.height = props.height || file.height || undefined;
		newFile.width = props.width || file.width || undefined;
		internal.priv.call("file.URL", newFile, success, error);
	},
	/**
	 * Check a file object represents a file which exists.
	 *
	 * @param {{uri: string, name: string}} file
	 * @param {function(boolean)=} success
	 * @param {function({message: string}=} error
	 */
	'isFile': function (file, success, error) {
		if (!file || !("uri" in file)) {
			success(false);
		} else {
			internal.priv.call("file.isFile", file, success, error);
		}
	},
	/**
	 * Download and save a URL, return the file object representing saved file.
	 *
	 * @param {string} URL
	 * @param {function({uri: string})=} success
	 * @param {function({message: string}=} error
	 */
	'cacheURL': function (url, success, error) {
		internal.priv.call("file.cacheURL",
			{url: url},
			success && function (uri) { success({uri: uri}) },
			error
		);
	},
	'saveURL': function (url, success, error) {
		internal.priv.call("file.saveURL",
			{url: url},
			success && function (uri) { success({uri: uri}) },
			error
		);
	},
	/**
	 * Delete a file.
	 *
	 * @param {{uri: string} file
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */
	'remove': function (file, success, error) {
		internal.priv.call("file.remove", file, success, error);
	},
	/**
	 * Delete all cached files
	 *
	 * @param {function()=} success
	 * @param {function({message: string}=} error
	 */
	'clearCache': function (success, error) {
		internal.priv.call("file.clearCache", {}, success, error);
	}
};