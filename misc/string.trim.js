if (!String.trim) {
	String.trim = function(str) { // from: http://blog.stevenlevithan.com/archives/faster-trim-javascript
		var str = str.replace(/^\s\s*/, ''),ws = /\s/,i = str.length;
		while (ws.test(str.charAt(--i)));
		return str.slice(0, i + 1);
	};
}
