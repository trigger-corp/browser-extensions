forge['event'] = {
	'menuPressed': {
		addListener: function (callback, error) {
			internal.disabledModule(error, "event");
		}
	},
	'backPressed': {
		addListener: function (callback, error) {
			internal.disabledModule(error, "event");
		},
		preventDefault: function (success, error) {
			internal.disabledModule(error, "event");
		}
	},
	'messagePushed': {
		addListener: function (callback, error) {
			internal.disabledModule(error, "event");
		}
	},
	'orientationChange': {
		addListener: function (callback, error) {
			internal.disabledModule(error, "event");
		}
	},
	'connectionStateChange': {
		addListener: function (callback, error) {
			internal.disabledModule(error, "event");
		}
	},
	'appPaused': {
		addListener: function (callback, error) {
			internal.disabledModule(error, "event");
		}
	},
	'appResumed': {
		addListener: function (callback, error) {
			internal.disabledModule(error, "event");
		}
	}
};