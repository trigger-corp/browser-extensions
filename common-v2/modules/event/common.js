forge['event'] = {
	'menuPressed': {
		addListener: function (callback, error) {
			internal.addEventListener('event.menuPressed', callback);
		}
	},
	'backPressed': {
		addListener: function (callback, error) {
			internal.addEventListener('event.backPressed', function () {
				callback(function () {
					internal.priv.call('event.backPressed_closeApplication', {});
				});
			});
		},
		preventDefault: function (success, error) {
			internal.priv.call('event.backPressed_preventDefault', {}, success, error);
		},
		restoreDefault: function (success, error) {
			internal.priv.call('event.backPressed_restoreDefault', {}, success, error);
		}
	},
	'messagePushed': {
		addListener: function (callback, error) {
			internal.addEventListener('event.messagePushed', callback);
		}
	},
	'orientationChange': {
		addListener: function (callback, error) {
			internal.addEventListener('event.orientationChange', callback);
			
			if (nullObj && internal.currentOrientation !== nullObj) {
				internal.priv.receive({
					event: 'event.orientationChange'
				});
			}
		}
	},
	'connectionStateChange': {
		addListener: function (callback, error) {
			internal.addEventListener('event.connectionStateChange', callback);
			
			if (nullObj && internal.currentConnectionState !== nullObj) {
				internal.priv.receive({
					event: 'event.connectionStateChange'
				});
			}
		}
	},
	'appPaused': {
		addListener: function (callback, error) {
			internal.addEventListener('event.appPaused', callback);
		}
	},
	'appResumed': {
		addListener: function (callback, error) {
			internal.addEventListener('event.appResumed', callback);
		}
	}
};
