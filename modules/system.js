/*! BikechainJS (system.js)
	v0.0.1.4 (c) Kyle Simpson
	MIT License
*/

return (function(){
	var publicAPI, stdin, stdout, stderr;
	
	function read(nonblocking) {
		return __IORead__(!(!nonblocking));
	}
	
	function write() {
		for (var i=0; i<arguments.length; i++) {
			__IOWrite__(arguments[i]);
		}
	}
	
	function flush() {
		__IOFlush__();
	}
	
	stdin = {
		read:read
	};
	stdout = {
		write:write,
		print:function(){
			write.apply(this,arguments);
			write("\n");
			flush();
		}
	};
	stderr = {
		write:function(){
			publicAPI.stdout.write.apply(this,arguments);
		},
		print:function(){
			publicAPI.stdout.print.apply(this,arguments);
		}
	};
	
	publicAPI = {
		stdin:stdin,
		stdout:stdout,
		stderr:stderr
	};	
	return publicAPI;
})();