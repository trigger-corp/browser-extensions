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


/**
 * Generate query string
 */
var generateQueryString = function (obj) {
	if (!obj) {
		return "";
	}
	if (!(obj instanceof Object)) {
		return new String(obj).toString();
	}
	
	var params = [];
	var processObj = function (obj, scope) {
		if (obj === null) {
			return;
		} else if (obj instanceof Array) {
			var index = 0;
			for (var x in obj) {
				var key = (scope ? scope : '') + '[' + index + ']';
				index += 1;
				if (!obj.hasOwnProperty(x)) continue;
				processObj(obj[x], key);
			}
		} else if (obj instanceof Object) {
			for (var x2 in obj) {
				if (!obj.hasOwnProperty(x2)) continue;
				var key2 = x2;
				if (scope) {
					key2 = scope + '[' + x2 + ']';
				}
				processObj(obj[x2], key2);
			}
		} else {
			params.push(encodeURIComponent(scope)+'='+encodeURIComponent(obj));
		}
	};
	processObj(obj);
	return params.join('&').replace('%20', '+');
};


/**
 * Generate a URI from an existing url and additional query data
 */
var generateURI = function (uri, queryData) {
	var newQuery = '';
	if (uri.indexOf('?') !== -1) {
		newQuery += uri.split('?')[1]+'&';
		uri = uri.split('?')[0];
	}
	newQuery += generateQueryString(queryData)+'&';
	// Remove trailing &
	newQuery = newQuery.substring(0,newQuery.length-1);
	return uri+(newQuery ? '?'+newQuery : '');
};

var generateMultipartString = function (obj, boundary) {
	if (typeof obj === "string") {
		return '';
	}
	var partQuery = '';
	for (var key in obj) {
		if (!obj.hasOwnProperty(key)) continue;
		if (obj[key] === null) continue;
		// TODO: recursive flatten, deal with arrays
		partQuery += '--'+boundary+'\r\n';
		partQuery += 'Content-Disposition: form-data; name="'+key.replace('"', '\\"')+'"\r\n\r\n';
		partQuery += obj[key].toString()+'\r\n';
	}
	return partQuery;
};


forge['request']['ajax'] = function (options, success, error) {
	/**
	 * Perform ajax request.
	 *
	 * See jQuery.ajax for further details, not all jQuery options are supported.
	 *
	 * @param {Object} options Contains all relevant options
	 */

	var files = (options.files ? options.files : null);
	var fileUploadMethod = (options.fileUploadMethod ? options.fileUploadMethod : 'multipart');
	var url = (options.url ? options.url : null);
	success = success ? success : (options.success ? options.success : undefined);
	error = error ? error : (options.error ? options.error : undefined);
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

	if (typeof accepts === "string") {
		// Given "text/html" instead of ["text/html"].
		accepts = [accepts];
	}
	var boundary = null;
	if (files) {
		type = 'POST';
		if (fileUploadMethod == 'multipart') {
			boundary = forge.tools.UUID().replace(/-/g,"");
			data = generateMultipartString(data, boundary);
			contentType = "multipart/form-data; boundary="+boundary;
		} else if (fileUploadMethod == 'raw') {
			// Limit to one file
			if (files.length > 1) {
				forge.logging.warning("Only one file can be uploaded at once with type 'raw'");
				files = [files[0]];
			}
			data = null;
			// XXX: This should probably be set in native code.
			contentType = "image/jpg";
		}
	} else {
		if (type == 'GET') {
			url = generateURI(url, data);
			data = null;
		} else if (data) {
			data = generateQueryString(data);
			if (!contentType) {
				contentType = "application/x-www-form-urlencoded";
			}
		}
	}

	if (cache) {
		cache = {};
		cache['wm'+Math.random()] = Math.random();
		url = generateURI(url, cache);
	}
	if (accepts) {
		headers['Accept'] = accepts.join(',');
	}
	if (contentType) {
		headers['Content-Type'] = contentType;
	}
	
	// Catalyst output
	var debug = {};
	if (window._forgeDebug) {
		try {
			debug.id = forge.tools.UUID();
			debug.fromUrl = window.location.href;
			debug.reqTime = (new Date()).getTime() / 1000.0;
			debug.method = type;
			debug.data = data;
			debug.url = url;
			
			_forgeDebug.wi.NetworkNotify.identifierForInitialRequest(debug.id, debug.url, {
				url: debug.fromUrl,
				frameId: 0,
				loaderId: 0
			}, []);
			
			_forgeDebug.wi.NetworkNotify.willSendRequest(debug.id, debug.reqTime, {
				url: debug.url,
				httpMethod: debug.method,
				httpHeaderFields: {},
				requestFormData: debug.data
			}, {isNull: true});
		} catch (e) {}
	}

	var complete = false;
	var timer = setTimeout(function () {
		if (complete) return;
		complete = true;
		if (window._forgeDebug) {
			try {
				debug.respTime = (new Date()).getTime() / 1000.0;
				debug.respText = data;
				_forgeDebug.wi.NetworkNotify.didReceiveResponse(debug.id, debug.reqTime, "XHR", {
					mimeType: "Unknown",
					textEncodingName: "",
					httpStatusCode: 1,
					httpStatusText: "Failure",
					httpHeaderFields: {},
					connectionReused: false,
					connectionID: 0,
					wasCached: false
				});
				
				_forgeDebug.wi.NetworkNotify.setInitialContent(debug.id, debug.respText, "XHR");
				
				_forgeDebug.wi.NetworkNotify.didFinishLoading(debug.id, debug.respTime);
			} catch (e) {}
		}
		error && error({
			message: 'Request timed out',
			type: 'EXPECTED_FAILURE'
		});
	}, timeout);

	forge.internal.call("request.ajax", {
		url: url,
		username: username,
		password: password,
		data: data,
		headers: headers,
		timeout: timeout,
		type: type,
		boundary: boundary,
		files: files,
		fileUploadMethod: fileUploadMethod
	}, function (data) {
		clearTimeout(timer);
		if (complete) return;
		complete = true;
		if (window._forgeDebug) {
			try {
				debug.respTime = (new Date()).getTime() / 1000.0;
				debug.respText = data;
				_forgeDebug.wi.NetworkNotify.didReceiveResponse(debug.id, debug.reqTime, "XHR", {
					mimeType: "Unknown",
					textEncodingName: "",
					httpStatusCode: 1,
					httpStatusText: "Success",
					httpHeaderFields: {},
					connectionReused: false,
					connectionID: 0,
					wasCached: false
				});
				
				_forgeDebug.wi.NetworkNotify.setInitialContent(debug.id, debug.respText, "XHR");
				
				_forgeDebug.wi.NetworkNotify.didFinishLoading(debug.id, debug.respTime);
			} catch (e) {}
		}
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
		success && success(data);
	}, function () {
		clearTimeout(timer);
		if (complete) return;
		complete = true;
		if (window._forgeDebug) {
			try {
				debug.respTime = (new Date()).getTime() / 1000.0;
				debug.respText = data;
				_forgeDebug.wi.NetworkNotify.didReceiveResponse(debug.id, debug.reqTime, "XHR", {
					mimeType: "Unknown",
					textEncodingName: "",
					httpStatusCode: 1,
					httpStatusText: "Failure",
					httpHeaderFields: {},
					connectionReused: false,
					connectionID: 0,
					wasCached: false
				});
				
				_forgeDebug.wi.NetworkNotify.setInitialContent(debug.id, debug.respText, "XHR");
				
				_forgeDebug.wi.NetworkNotify.didFinishLoading(debug.id, debug.respTime);
			} catch (e) {}
		}
		error && error.apply(this, arguments);
	});
};
