forge['request'] = {
	/**
	 * Get the response data from a URL. Imposes no cross-domain restrictions.
	 *
	 * See "ajax()" for more advanced options like setting headers, etc.
	 *
	 * @param {string} url
	 * @param {function(*)=} success Response data
	 * @param {function({message: string}=} error
	 */
	'get': function(url, success, error) {
		forge.request.ajax({
			url: url,
			dataType: "text",
			success: success && function () {
				try {
					arguments[0] = JSON.parse(arguments[0]);
				} catch (e) {}
				success.apply(this, arguments);
			},
			error: error
		});
	}
};

forge['request']['ajax'] = function (options) {
	/**
	 * Perform ajax request.
	 *
	 * See jQuery.ajax for further details, not all jQuery options are supported.
	 *
	 * @param {Object} options Contains all relevant options
	 */
	var dataType = options.dataType;
	if (dataType == 'xml' ) {
		options.dataType = 'text';
	}
	var success = options.success && function (data) {
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
			}
		} catch (e) {}
		options.success && options.success(data);
	};
	var error = options.error && function (error) {
		if (error.status == 'error' && !error.err) {
			forge.logging.log('AJAX request to '+options.url+' failed, have you included that url in the permissions section of the config file for this app?');
		}
		options.error && options.error(error);
	};
	internal.priv.call("request.ajax", options, success, error);
};