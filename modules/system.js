/*! BikechainJS (system.js)
	v0.0.2 (c) Kyle Simpson
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
	
	function err_write() {
		for (var i=0; i<arguments.length; i++) {
			__IOErrWrite__(arguments[i]);
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
			var args = [].slice.call(arguments);
			args.push("\n");
			write.apply(this,args);
			flush();
		}
	};
	stderr = {
		write:err_write,
		print:function(){
			var args = [].slice.call(arguments);
			args.push("\n");
			err_write.apply(this,args);
			flush();
		}
	};
	
	publicAPI = {
		stdin:stdin,
		stdout:stdout,
		stderr:stderr
	};	
	return publicAPI;
})();