/*! BikechainJS (os.js)
	v0.0.1.4 (c) Kyle Simpson
	MIT License
*/

return (function(global){
	var publicAPI;
	
	function iopipe(handle) {
		var pipes = {
			stdout:{
				read:function(){ return handle; }
			},
			// TODO: support writing to stdin of process without having to send as first param of `execute` call
			/*stdin:{
				write:function(){ return; }
			},*/
			stderr:{
				read:function(){ return handle; }
			}
		};
		return pipes;
	}
	
	function execute() {
		var exec = __SysExec__.apply(global,arguments);
		
		return iopipe(exec);
	}
	
	publicAPI = {
		execute: execute
	};	
	return publicAPI;
	
})(this);