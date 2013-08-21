 /* ***** BEGIN LICENSE BLOCK *****
 * Version: MIT/X11 License
 * 
 * Copyright (c) 2010 Erik Vold
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Contributor(s):
 *	 Erik Vold <erikvvold@gmail.com> (Original Author)
 *	 Greg Parris <greg.parris@gmail.com>
 *
 * ***** END LICENSE BLOCK ***** */
 
var roundRect = function (ctx, x, y, width, height, radius, fill, stroke) {
	if (typeof stroke == "undefined" ) {
		stroke = true;
	}
	if (typeof radius === "undefined") {
		radius = 5;
	}
	ctx.beginPath();
	ctx.moveTo(x + radius, y);
	ctx.lineTo(x + width - radius, y);
	ctx.quadraticCurveTo(x + width, y, x + width, y + radius);
	ctx.lineTo(x + width, y + height - radius);
	ctx.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
	ctx.lineTo(x + radius, y + height);
	ctx.quadraticCurveTo(x, y + height, x, y + height - radius);
	ctx.lineTo(x, y + radius);
	ctx.quadraticCurveTo(x, y, x + radius, y);
	ctx.closePath();
	if (stroke) {
		ctx.stroke();
	}
	if (fill) {
		ctx.fill();
	}				
}


var NS_XUL = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

// Persist toolbar location
var ss = require("simple-storage");
if (!ss.storage || !ss.storage.toolbar) {
	ss.storage.toolbar = {
		toolbarID: "",
		insertbefore: ""
	}
}
var storage = ss.storage.toolbar;

exports.ToolbarButton = function ToolbarButton(options) {
	var destroyed = false,
		event = new (require('events').EventEmitter.compose({
			command: function (button) {
				this._emit('command', options, button);
			}
		}));
	var badge;
	// Function to deal with external toolbar changes
	var saveTBNodeInfo = function(e) {
		var doc = this.document;
		var $ = function (id) { return doc.getElementById(id) };
		
		storage.toolbarID = $(options.id).parentNode.getAttribute("id") || "";
		storage.insertbefore = ($(options.id).nextSibling || "")
				&& $(options.id).nextSibling.getAttribute("id").replace(/^wrapper-/i, "");
				
		// change the current position for open windows
		for (var window in winUtils.windowIterator()) {
			if ("chrome://browser/content/browser.xul" != window.location) continue;

			var doc = window.document;
			var $ = function (id) { return doc.getElementById(id) };

			var tb = $(storage.toolbarID);
			var b4 = $(storage.insertbefore);

			if (tb) tb.insertItem(options.id, b4, null, false);
		}
	};
	
	var delegate = {
		onTrack: function (window) {
			if ("chrome://browser/content/browser.xul" != window.location || destroyed)
				return;

			var doc = window.document;
			badge = function (image, text, bg, color, cb) {
				if (!text) {
					cb(image);
					return;
				}
				var width = 19;
				var height = 19;
				var canvas = doc.createElementNS('http://www.w3.org/1999/xhtml', 'canvas');
				canvas.width = width;
				canvas.height = height;
				var ctx = canvas.getContext('2d');	
				var img = doc.createElementNS('http://www.w3.org/1999/xhtml', 'img');;
				img.src = image;
				img.onload = function() {
					// Load image

					var left = Math.round((width - Math.min(img.width, width))/2);
					var top = Math.round((height - Math.min(img.height, height))/2);
					ctx.drawImage(img,left,top);
					
					var badge = text;
					
					ctx.font = "10px sans-serif";
					var textwidth = ctx.measureText(badge).width;
					var textheight = 9;
					
					// Add box
					ctx.fillStyle = bg;
					ctx.shadowBlur = 1;
					ctx.shadowColor = bg;
					roundRect(ctx, width-3-textwidth, height-1-textheight, textwidth+2, textheight, 3, true, false)
					
					// Add text
					ctx.fillStyle = color;
					ctx.shadowBlur = 1;
					ctx.shadowColor = color;
					
					ctx.fillText(badge, width-2-textwidth, height-2);
					cb(canvas.toDataURL());
				};
			}
			function $(id) { return doc.getElementById(id) };
			function xul(type) { return doc.createElementNS(NS_XUL, type) };

			// create toolbar button
			var tbb = xul("toolbarbutton");
			tbb.setAttribute("id", options.id);
			tbb.setAttribute("type", "button");
			if (options.icon) {
				badge(options.icon, options.badgeText, options.badgeBGColor, "white", function (url) {
					tbb.setAttribute("image", url);
				});
			}
			tbb.setAttribute("class", "toolbarbutton-1 chromeclass-toolbar-additional");
			tbb.setAttribute("label", options.title);
			tbb.setAttribute("tooltiptext", options.title);
			tbb.addEventListener("command", function() {
				event.command(this);
			}, true);

			// add toolbarbutton to palette
			($("navigator-toolbox") || $("mail-toolbox")).palette.appendChild(tbb);

			// find a toolbar to insert the toolbarbutton into
			if (storage.toolbarID) {
				var tb = $(storage.toolbarID);
			}
			if (!tb) {
				var tb = $("nav-bar");
			}

			// found a toolbar to use?
			if (tb) {
				var b4;

				// find the toolbarbutton to insert before
				if (storage.insertbefore) {
					b4 = $(storage.insertbefore);
				}
				if (!b4) {
					var currentset = tb.getAttribute("currentset").split(",");
					var i = currentset.indexOf(options.id) + 1;

					// was the toolbarbutton id found in the curent set?
					if (i > 0) {
						var len = currentset.length;
						// find a toolbarbutton to the right which actually exists
						for (; i < len; i++) {
							b4 = $(currentset[i]);
							if (b4) break;
						}
					}
				}

				tb.insertItem(options.id, b4, null, false);
			}

			window.addEventListener("aftercustomization", saveTBNodeInfo, false);
		},
		onUntrack: function (window) {}
	};
	var winUtils = require("window-utils");
	var tracker = new winUtils.WindowTracker(delegate);

	var methods = {
		destroy: function() {
			if (destroyed) return;
			destroyed = true;
				
			// For each window: remove button, remove listener
			for (var window in winUtils.windowIterator()) {
				if ("chrome://browser/content/browser.xul" != window.location) continue;

				var tb = window.document.getElementById(options.id);
				if (tb) {
					tb.parentNode.removeChild(tb);
				}
				window.removeEventListener("aftercustomization", saveTBNodeInfo, false);
			}

		},
		update: function(opt) {
			if (opt.url !== undefined) {
				options.url = opt.url;
			}
			if (opt.icon !== undefined) {
				options.icon = opt.icon;
			}
			if (opt.title !== undefined) {
				options.title = opt.title;
			}
			if (opt.badgeText !== undefined) {
				options.badgeText = opt.badgeText;
			}
			if (opt.badgeBGColor !== undefined) {
				options.badgeBGColor = 'rgba('+opt.badgeBGColor[0]+','+opt.badgeBGColor[1]+','+opt.badgeBGColor[2]+','+opt.badgeBGColor[3]/255+')';
			}
			badge(options.icon, options.badgeText, options.badgeBGColor, 'white', function (url) {
				for (var window in winUtils.windowIterator()) {
					var doc = window.document;
					var $ = function (id) { return doc.getElementById(id) };
				
					var tbb = $(options.id);
					if (tbb) {
						tbb.setAttribute('image', url);
						tbb.setAttribute('tooltiptext', options.title);
						tbb.setAttribute('label', options.title);
					}
				}
			});
		},
		addListener: function(cb, force) {
			event.on("command", function () {
				if (!options.url || force) {
					cb.apply(this, arguments);
				}
			});
		}
	};
	// Destroy toolbar on extension unload
	require('unload').when(methods.destroy);
	return methods;
};

function toolbarbuttonExists(doc, id) {
	var toolbars = doc.getElementsByTagNameNS(NS_XUL, "toolbar");
	for (var i = toolbars.length - 1; ~i; i--) {
		if ((new RegExp("(?:^|,)" + id + "(?:,|$)")).test(toolbars[i].getAttribute("currentset")))
			return toolbars[i];
	}
	return false;
}