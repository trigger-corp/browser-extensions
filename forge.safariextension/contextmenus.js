document.addEventListener("contextmenu", function(event){
	safari.self.tab.setContextMenuEventUserInfo(event, {
		nodeName: event.target.nodeName,
		srcUrl: event.target.src
	});
}, false);