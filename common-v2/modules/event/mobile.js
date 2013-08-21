var nullObj = {};
internal.currentOrientation = nullObj;
internal.currentConnectionState = nullObj;

// Internal orientation event
internal.addEventListener('internal.orientationChange', function (data) {
	if (internal.currentOrientation != data.orientation) {
		internal.currentOrientation = data.orientation;
		// Trigger public orientation event
		internal.priv.receive({
			event: 'event.orientationChange'
		});
	}
});

internal.addEventListener('internal.connectionStateChange', function (data) {
	if (data.connected != internal.currentConnectionState.connected || data.wifi != internal.currentConnectionState.wifi) {
		internal.currentConnectionState = data;
		internal.priv.receive({
			event: 'event.connectionStateChange'
		});
	}
});