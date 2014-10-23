/* global unsafeWindow, cloneInto, exportFunction, createObjectIn, self */
unsafeWindow._forge = cloneInto(unsafeWindow._forge || {}, unsafeWindow);
unsafeWindow._forge.self = createObjectIn(unsafeWindow._forge, {defineAs: "self"});
exportFunction(self.on, unsafeWindow._forge.self, {defineAs:"on", allowCallbacks:true});
exportFunction(self.postMessage, unsafeWindow._forge.self, {defineAs:"postMessage", allowCallbacks:true});
