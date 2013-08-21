forge['internal'] = {
	'ping': function (data, success, error) {
		internal.priv.call("internal.ping", {data: [data]}, success, error);
	},
	'call': internal.priv.call,
	'addEventListener': internal.addEventListener,
	listeners: internal.listeners
};