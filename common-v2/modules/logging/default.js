//
// Logging helper functions
//

// Adapted from node.js
var inspectObject = function (obj, showHidden, depth) {
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
};
var logMessage = function(message, level) {
	if ('logging' in forge.config) {
		var eyeCatcher = forge.config.logging.marker || 'FORGE';
	} else {
		var eyeCatcher = 'FORGE';
	}
	message = '[' + eyeCatcher + '] '
			+ (message.indexOf('\n') === -1 ? '' : '\n') + message;
	internal.priv.call("logging.log", {
		message: message,
		level: level
	});
	
	// Also log to the console if it exists.
	if (typeof console !== "undefined") {
		switch (level) {
			case 10:
				if (console.debug !== undefined && !(console.debug.toString && console.debug.toString().match('alert'))) {
					console.debug(message);
				}
				break;
			case 30:
				if (console.warn !== undefined && !(console.warn.toString && console.warn.toString().match('alert'))) {
					console.warn(message);
				}
				break;
			case 40:
			case 50:
				if (console.error !== undefined && !(console.error.toString && console.error.toString().match('alert'))) {
					console.error(message);
				}
				break;
			default:
			case 20:
				if (console.info !== undefined && !(console.info.toString && console.info.toString().match('alert'))) {
					console.info(message);
				}
				break;
		}
	}
};

var logNameToLevel = function(name, deflt) {
	if (name in forge.logging.LEVELS) {
		return forge.logging.LEVELS[name];
	} else {
		forge.logging.__logMessage('Unknown configured logging level: '+name);
		return deflt;
	}
};

var formatException = function(ex) {
	var exMsg = function(ex) {
		if (ex.message) {
			return ex.message;
		} else if (ex.description) {
			return ex.description;
		} else {
			return ''+ex;
		}
	}

	if (ex) {
		var str = '\nError: ' + exMsg(ex);
		try {
			if (ex.lineNumber) {
				str += ' on line number ' + ex.lineNumber;
			}
			if (ex.fileName) {
				var file = ex.fileName;
				str += ' in file ' + file.substr(file.lastIndexOf('/')+1);
			}
		} catch (e) {
		}
		if (ex.stack) {
			str += '\r\nStack trace:\r\n' + ex.stack;
		}
		return str;
	}
	return '';
};

forge['logging'] = {
	/**
	 * Log messages and exceptions to the console, if available
	 * @enum {number}
	 */
	LEVELS: {
		'ALL': 0,
		'DEBUG': 10,
		'INFO': 20,
		'WARNING': 30,
		'ERROR': 40,
		'CRITICAL': 50
	},

	'debug': function(message, exception) {
		forge.logging.log(message, exception, forge.logging.LEVELS.DEBUG);
	},
	'info': function(message, exception) {
		forge.logging.log(message, exception, forge.logging.LEVELS.INFO);
	},
	'warning': function(message, exception) {
		forge.logging.log(message, exception, forge.logging.LEVELS.WARNING);
	},
	'error': function(message, exception) {
		forge.logging.log(message, exception, forge.logging.LEVELS.ERROR);
	},
	'critical': function(message, exception) {
		forge.logging.log(message, exception, forge.logging.LEVELS.CRITICAL);
	},

	/**
	 * Log a message onto the console. An eyecatcher of [YOUR_UUID] will be automatically prepended.
	 * See the "logging.level" configuration directive, which controls how verbose the logging will be.
	 * @param {string} message the text you want to log
	 * @param {Error} exception (optional) an Error instance to log
	 * @param {number} level: one of "api.logging.DEBUG", "api.logging.INFO", "api.logging.WARNING", "api.logging.ERROR" or "api.logging.CRITICAL"
	 */
	'log': function(message, exception, level) {

		if (typeof(level) === 'undefined') {
			var level = forge.logging.LEVELS.INFO;
		}
		try {
			var confLevel = logNameToLevel(forge.config.logging.level, forge.logging.LEVELS.ALL);
		} catch(e) {
			var confLevel = forge.logging.LEVELS.ALL;
		}
		if (level >= confLevel) {
			logMessage(inspectObject(message, false, 10) + formatException(exception), level);
		}
	}
};