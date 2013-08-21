forge['reload'] = {
	'updateAvailable': function(success, error) {
		internal.priv.call("reload.updateAvailable", {}, success, error);
	},
	'update': function(success, error) {
		internal.priv.call("reload.update", {}, success, error);
	},
	'pauseUpdate': function(success, error) {
		internal.priv.call("reload.pauseUpdate", {}, success, error);
	},
	'applyNow': function(success, error) {
		forge.logging.error("reload.applyNow has been disabled, please see docs.trigger.io for more information.");
		error({message: "reload.applyNow has been disabled, please see docs.trigger.io for more information.", type: "UNAVAILABLE"});
	},
	'applyAndRestartApp': function(success, error) {
		internal.priv.call("reload.applyAndRestartApp", {}, success, error);
	},
	'switchStream': function(streamid, success, error) {
		internal.priv.call("reload.switchStream", {streamid: streamid}, success, error);
	},
	'updateReady': {
		addListener: function (callback, error) {
			internal.addEventListener('reload.updateReady', callback);
		}
	},
	'updateProgress': {
		addListener: function (callback, error) {
			internal.addEventListener('reload.updateProgress', callback);
		}
	}
};
