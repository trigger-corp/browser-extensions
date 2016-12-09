forge['cookies'] = {
	'get': function(name, success, error) {
		internal.priv.call("cookies.get", {
			name: name
		}, success, error);
	},
	'remove': function(name) {
		internal.priv.call("cookies.remove", {
			name: name
		});
	}
};
