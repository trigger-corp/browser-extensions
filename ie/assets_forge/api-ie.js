/**
 * api-ie.js
 * 
 * IE specific overrides to the generic Forge api.js
 */


/**
 * debug logger
 */
function logger(message) {
    window.extensions.log("api-ie.js", message);
};


/**
 * console.log|error|debug 
 */
function fallbackLogger(level) {
    return function(message) {
        window.extensions.log("fallback-" + level, message);
    };
};
window.console = (window.console ? window.console : {});
window.console.log   = (window.console.log ? window.console.log : fallbackLogger("log"));
window.console.error = (window.console.error ? window.console.error : fallbackLogger("error"));
window.console.debug = (window.console.debug ? window.console.debug : fallbackLogger("debug"));


/**
 * Identity
 */
forge.is.desktop = function() {
    return true;
};

forge.is.ie = function() {
    return true;
};


/**
 * Calls from content-script to background app.
 *
 * @param {Object} with keys:
 * {Number} callid unique ID for this call
 * {String} method Name of the JS API method
 * {Object} params Key-values to pass to Android
 */
internal.priv.send = function(data) {
    forge.message.broadcastBackground("bridge", data, function(reply) {
        internal.priv.receive(reply);
    });
};


/**
 * Add a listener for broadcast messages sent by other pages where your app is active.
 *
 * @param {String} type (optional) an arbitrary string: if included, the callback will only be fired
 *  for messages broadcast with the same type; if omitted, the callback will be fired for all messages
 * @param {Function} callback a function which will be called with the contents of relevant
 * broadcast messages as its first argument and a reply function as its second argument.
 * @param {Function} errorCallback Not used.
 */
forge.message.listen = function(type, callback, error) {
    if (typeof type === 'function') { 
        // no type passed in: shift arguments left one
        error = callback;
        callback = type;
        type = null;
    }
    type = (type === null ? "*" : type);
    if (typeof callback !== 'function') callback = function(){};
    if (typeof error !== 'function') error = function(e) {
        logger("forge.message.listen error -> " + e);
    };
    /*logger("forge.message.listen" +
           " -> " + forge.config.uuid +
           " -> " + type +
           " -> " + typeof callback +
           " -> " + typeof error);*/

    var tabId = window.extensions.get_tabId();
    window.messaging.fg_listen(forge.config.uuid, tabId, type, function(content, reply) {
        callback(safe_jparse(content), function(content) {
            reply(safe_jstringify(content));
        });
    }, error);
};


/**
 * Broadcast a message to listeners set up in your background app.
 *
 * @param {String} type Limits the listeners which will receive this message.
 * @param {Any} content The message body which will be passed into active listeners.
 * @param {Function} callback Reply function which will be invoked if any listeners reply.
 */
forge.message.broadcastBackground = function(type, content, callback, error) {
    if (typeof(type) === 'function') { 
        // no type passed in: shift arguments left one
        error = callback;
        callback = content;
        content = type;
        type = null;
    } 
    type = (type === null ? "*" : type);   
    if (typeof callback !== 'function') callback = function(){};
    if (typeof error !== 'function') error = function(e) {
        logger("forge.message.broadcastBackground error -> " + e);
    };
    /*logger("forge.message.broadcastBackground" +
           " -> " + forge.config.uuid +
           " -> " + type +
           " -> " + safe_jstringify(content) +
           " -> " + typeof callback +
           " -> " + typeof error);*/

    window.messaging.fg_broadcastBackground(forge.config.uuid, type, 
                                            safe_jstringify(content),
                                            function(content) {
                                                callback(safe_jparse(content));
                                            }, error);
};


/**
 * Broadcast a message to listeners set up in your non-privileged pages.
 *
 * @param {String} type Limits the listeners which will receive this message.
 * @param {Any} content The message body which will be passed into active listeners.
 * @param {Function} callback Reply function which will be invoked if any listeners reply.
 */
forge.message.broadcast = function(type, content, callback, error) {
    if (typeof(type) === 'function') { 
        // no type passed in: shift arguments left one
        error = callback;
        callback = content;
        content = type;
        type = null;
    } 
    type = (type === null ? "*" : type);   
    if (typeof callback !== 'function') callback = function(){};
    if (typeof error !== 'function') error = function(e) {
        logger("forge.message.broadcast error -> " + e);
    };
    /*logger("forge.message.broadcast" +
           " -> " + forge.config.uuid +
           " -> " + type +
           " -> " + content +
           " -> " + typeof callback +
           " -> " + typeof error);*/

    window.messaging.fg_broadcast(forge.config.uuid, type, 
                                  safe_jstringify(content), 
                                  function(content) {
                                      callback(safe_jparse(content));
                                  }, error);
};


forge.message.toFocussed = function(type, content, callback, error) {
    if (typeof(type) === 'function') { 
        // no type passed in: shift arguments left one
        error = callback;
        callback = content;
        content = type;
        type = null;
    } 
    type = (type === null ? "*" : type);   
    if (typeof callback !== 'function') callback = function(){};
    if (typeof error !== 'function') error = function(e) {
        logger("forge.message.toFocussed error -> " + e);
    };
    /*logger("forge.message.toFocussed" +
           " -> " + forge.config.uuid +
           " -> " + type +
           " -> " + content +
           " -> " + typeof callback +
           " -> " + typeof error);*/

    window.messaging.fg_toFocussed(forge.config.uuid, type, 
                                   safe_jstringify(content), 
                                   function(content) {
                                       callback(safe_jparse(content));
                                   }, error);
};


/**
 * Close the tab that makes the call, intended to be called from foreground
 * @param {function({message: string}=} error
 */
forge.tabs.closeCurrent = function(error) {
    window.accessible.closeCurrent(typeof error === "function" 
                                   ? error : function(){});
};

forge.tabs.open = function(url, selected, success, error) {
    if (typeof selected === 'function') {
        error = success;
        success = selected;
        selected = false;
    }

    window.accessible.open(url, selected, 
                           typeof success === "function" ? success : function(){}, 
                           typeof error   === "function" ? error   : function(){});
};


/**
 * Reload the current document and all its scripts
 */
forge.document.reload = function() {
    document.location.href = document.location.href;
};


/**
 * Returns a location object for the current document
 */
forge.document.location = function(success, error) {
    if (!success) success = function() {};
    var a = document.createElement('a');
    a.href = window.extensions.get_location();
    success(a);
};
