;
function loggerproxy(message) {
    window.extensions.log("api-proxy.js", message);
};

/** 
 * Utilities: string_limit
 */
function string_limit(s, maxlen) {
    maxlen = (typeof maxlen === "undefined" ? 160 : maxlen);
    if (s.length < maxlen) {
        return s;
    }
    return s.substr(0, maxlen / 2) + " ... <schnip /> ... " + 
            s.substr(s.length - maxlen / 2);
};
window.string_limit = string_limit;


/** 
 * Utilities: dump
 */
function dump(object) {
    var json = "object could not be stringified";
    try {
        json = JSON.stringify(object);
    } catch (e) {}
    logger("    DUMP -> " + string_limit(json));
    return json;
};
window.dump = dump;


/**
 * Utilities: dumpXML
 */
function dumpXML(node) {
    if (node.hasChildNodes()) {
        logger("tagName: ", node.tagName);
        var nodes = node.childNodes.length;
        for (var i = 0; i < node.childNodes.length; i++) {
            dumpXML(node.childNodes(i));
        }
    } else {
        logger("text: ", node.text);
    }
};
window.dumpXML = dumpXML;


/** 
 * Utilities: Array.indexOf
 */
if (typeof Array.prototype.indexOf !== 'function') {
    Array.prototype.indexOf = function (object, index) {
        for (var i = (index || 0); i < this.length; i++) {
            if (this[i] === object) {
                return i;
            }
        }
        return -1;
    }
}


/** 
 * Utilities: safe_jstringify
 */
function safe_jstringify(o) {
    var s = "";
    try {
        s = JSON.stringify(o);
    } catch (e) {
        loggerproxy("[ERROR] failed serializing object to JSON" +
                    " -> " + typeof o +
                    " -> " + o +
                    " -> " + JSON.stringify(e));
    }
    //loggerproxy("safe_jstringify -> " + s);
    return s;
}

/** 
 * Utilities: safe_jparse
 */
function safe_jparse(s) {
    var o = {};
    try {
        o = JSON.parse(s);
    } catch (e) {
        loggerproxy("[ERROR] failed deserializing object from JSON" +
                    " -> " + s +
                    " -> " + JSON.stringify(e));
    }
    //loggerproxy("safe_jparse -> " + s);
    return o;
};

