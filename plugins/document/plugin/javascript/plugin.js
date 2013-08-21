forge['document'] = {
	'reload': function() {
		return document.location.reload();
	},
	'location': function (success, error) {
		success(document.location);
	}
};
