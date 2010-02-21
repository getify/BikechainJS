/*! BikechainJS (os.js)
	v0.0.1 (c) Kyle Simpson
	MIT License
*/

return (function(global){
	var publicAPI;
	
	function iopipe(handle) {
		var pipes = {
			stdout:{
				read:function(){ return handle; }
			},
			stdin:{
				write:function(){ return; }
			},
			stderr:{
				read:function(){ return handle; }
			}
		};
		return pipes;
	}
	
	function command() {
		var exec = SYSEXEC.apply(global,arguments);
		
		return iopipe(exec);
	}
	
	publicAPI = {
		command:command
	};	
	return publicAPI;
	
})(this);