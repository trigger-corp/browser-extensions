(function () {
	function browserName() {
		var userAgent = navigator.userAgent;
		var browser = "Unknown";
		if (userAgent.indexOf('Chrome') != -1) {
			browser = "Chrome";
		} else if (userAgent.indexOf('Firefox') != -1) {
			browser = "Firefox";
		} else if (userAgent.indexOf('Safari') != -1) {
			browser = "Safari";
		}
		return browser;
	}

	function hexEncode(data) {
		var b16_digits = '0123456789abcdef';
		var b16_map = new Array();
		for (var i=0; i<256; i++) {
			b16_map[i] = b16_digits.charAt(i >> 4) + b16_digits.charAt(i & 15);
		}
		
		var result = new Array();
		for (var i=0; i<data.length; i++) {
			result[i] = b16_map[data.charCodeAt(i)];
		}
		
		return result.join('');
	}

	// Adapted from node.js
	function inspectObject(obj, showHidden, depth) {
		var seen = [];
		stylize = function (str, styleType) {
			return str;
		};

		function isRegExp(re) {
			return re instanceof RegExp || (typeof re === 'object' && Object.prototype.toString.call(re) === '[object RegExp]');
		}

		function isArray(ar) {
			return ar instanceof Array || Array.isArray(ar) || (ar && ar !== Object.prototype && isArray(ar.__proto__));
		}

		function isDate(d) {
			if (d instanceof Date) return true;
			if (typeof d !== 'object') return false;
			var properties = Date.prototype && Object.getOwnPropertyNames(Date.prototype);
			var proto = d.__proto__ && Object.getOwnPropertyNames(d.__proto__);
			return JSON.stringify(proto) === JSON.stringify(properties);
		}

		function format(value, recurseTimes) {
			try {
				// Provide a hook for user-specified inspect functions.
				// Check that value is an object with an inspect function on
				// it

				// Filter out the util module, it's inspect function
				// is special
				if (value && typeof value.inspect === 'function' &&

				// Also filter out any prototype objects using the
				// circular check.
				!(value.constructor && value.constructor.prototype === value)) {
					return value.inspect(recurseTimes);
				}
				// Primitive types cannot have properties
				switch (typeof value) {
				case 'undefined':
					return stylize('undefined', 'undefined');
				case 'string':
					var simple = '\'' + JSON.stringify(value).replace(/^"|"$/g, '').replace(/'/g, "\\'").replace(/\\"/g, '"') + '\'';
					return stylize(simple, 'string');
				case 'number':
					return stylize('' + value, 'number');
				case 'boolean':
					return stylize('' + value, 'boolean');
				}
				// For some reason typeof null is "object", so special case
				// here.
				if (value === null) {
					return stylize('null', 'null');
				}
				// Special case Document
				if (value instanceof Document) {
					return (new XMLSerializer()).serializeToString(value);
				}
				// Look up the keys of the object.
				var visible_keys = Object.keys(value);
				var keys = showHidden ? Object.getOwnPropertyNames(value) : visible_keys;
				// Functions without properties can be shortcutted.
				if (typeof value === 'function' && keys.length === 0) {
					var name = value.name ? ': ' + value.name : '';
					return stylize('[Function' + name + ']', 'special');
				}
				// RegExp without properties can be shortcutted
				if (isRegExp(value) && keys.length === 0) {
					return stylize('' + value, 'regexp');
				}
				// Dates without properties can be shortcutted
				if (isDate(value) && keys.length === 0) {
					return stylize(value.toUTCString(), 'date');
				}
				var base, type, braces;
				// Determine the object type
				if (isArray(value)) {
					type = 'Array';
					braces = ['[', ']'];
				} else {
					type = 'Object';
					braces = ['{', '}'];
				}
				// Make functions say that they are functions
				if (typeof value === 'function') {
					var n = value.name ? ': ' + value.name : '';
					base = ' [Function' + n + ']';
				} else {
					base = '';
				}
				// Make RegExps say that they are RegExps
				if (isRegExp(value)) {
					base = ' ' + value;
				}
				// Make dates with properties first say the date
				if (isDate(value)) {
					base = ' ' + value.toUTCString();
				}
				if (keys.length === 0) {
					return braces[0] + base + braces[1];
				}
				if (recurseTimes < 0) {
					if (isRegExp(value)) {
						return stylize('' + value, 'regexp');
					} else {
						return stylize('[Object]', 'special');
					}
				}
				seen.push(value);
				var output = keys.map(function (key) {
					var name, str;
					if (value.__lookupGetter__) {
						if (value.__lookupGetter__(key)) {
							if (value.__lookupSetter__(key)) {
								str = stylize('[Getter/Setter]', 'special');
							} else {
								str = stylize('[Getter]', 'special');
							}
						} else {
							if (value.__lookupSetter__(key)) {
								str = stylize('[Setter]', 'special');
							}
						}
					}
					if (visible_keys.indexOf(key) < 0) {
						name = '[' + key + ']';
					}
					if (!str) {
						if (seen.indexOf(value[key]) < 0) {
							if (recurseTimes === null) {
								str = format(value[key]);
							} else {
								str = format(value[key], recurseTimes - 1);
							}
							if (str.indexOf('\n') > -1) {
								if (isArray(value)) {
									str = str.split('\n').map(

									function (line) {
										return '  ' + line;
									}).join('\n').substr(2);
								} else {
									str = '\n' + str.split('\n').map(

									function (
									line) {
										return '   ' + line;
									}).join('\n');
								}
							}
						} else {
							str = stylize('[Circular]', 'special');
						}
					}
					if (typeof name === 'undefined') {
						if (type === 'Array' && key.match(/^\d+$/)) {
							return str;
						}
						name = JSON.stringify('' + key);
						if (name.match(/^"([a-zA-Z_][a-zA-Z_0-9]*)"$/)) {
							name = name.substr(1, name.length - 2);
							name = stylize(name, 'name');
						} else {
							name = name.replace(/'/g, "\\'").replace(/\\"/g, '"').replace(/(^"|"$)/g, "'");
							name = stylize(name, 'string');
						}
					}
					return name + ': ' + str;
				});
				seen.pop();
				var numLinesEst = 0;
				var length = output.reduce(function (prev, cur) {
					numLinesEst++;
					if (cur.indexOf('\n') >= 0) numLinesEst++;
					return prev + cur.length + 1;
				}, 0);
				if (length > 50) {
					output = braces[0] + (base === '' ? '' : base + '\n ') + ' ' + output.join(',\n  ') + ' ' + braces[1];
				} else {
					output = braces[0] + base + ' ' + output.join(', ') + ' ' + braces[1];
				}
				return output;
			} catch (e) {
				return '[No string representation]';
			}
		}
		return format(obj, (typeof depth === 'undefined' ? 2 : depth));
	}

	function platform() {
		return browserName().toLowerCase();
	}

	window._testResultClient = {
		_resultUrl: function () {
			return this.config.resultUrls[platform()];
		},

		push: function (type, body) {
			this._resultUrl() && forge.request.ajax({
				url: this._resultUrl(),
				type: "POST",
				data: {
					message: JSON.stringify({
						testRunId: this.config.testRunId,
						useragent: navigator.userAgent,
						type: type,
						target: platform(),
						time: new Date().getTime(),
						body: body || {}
					})
				}
			});
		},

		setSuiteName: function (suiteName) {
			this._suiteName = suiteName;
		},

		_logResults: function (results) {
			forge.logging.log("--TESTING RESULTS START");
			forge.logging.log(JSON.stringify(results));
			forge.logging.log("--TESTING RESULTS END");
		},

		_qunitHooks: {
			begin: function () {
				this._modules = [];
				this._currentModule = null;
				this._currentTest = null;
			},

			moduleStart: function (data) {
				this._currentModule = {
					name: data.name,
					tests: []
				};
			},

			moduleDone: function (data) {
				this._modules.push(this._currentModule);
				this._currentModule = null;
			},

			testStart: function (data) {
				this._currentTest = {
					name: data.name,
					assertions: []
				};
			},

			testDone: function (data) {
				this._currentModule.tests.push(this._currentTest);
				this._currentTest = null;
			},

			log: function (data) {
				this._currentTest.assertions.push(data);
			},

			done: function (data) {
				var results = {
					totals: data,
					modules: this._modules,
					suiteName: this._suiteName
				};
				this.push('done', results);
				this._logResults(results);
				this._modules = [];
			}
		},

		setupQUnitHooks: function (QUnit) {
			for (var hook in this._qunitHooks) {
				QUnit[hook] = $.proxy(this._qunitHooks[hook], this);
			}
		},

		// default config for testrun
		config: {
			testRunId: 'manualtestrun',
			resultUrls: {
				firefox: 'http://127.0.0.1:8425'
			}
		}
	};
}());

window._testResultClient.setupQUnitHooks(window.QUnit);
