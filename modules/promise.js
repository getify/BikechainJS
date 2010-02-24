/*! BikechainJS (promise.js)
	v0.0.1 (c) Kyle Simpson
	MIT License
*/

return (function(){
	var undef;
	function Promise(){}
	Promise.prototype.constructor = Promise;
	
	return function(cb) {
		var publicAPI, queue = [], old_ret, promise_fulfilled = false;
		
		function fulfill(val) {
			var ret_val = val;
			if (typeof ret_val != "undefined") old_ret = ret_val;
			
			try {
				return val;
			}
			finally {
				for (var i=0,len=queue.length; i<len; i++) {
					if (typeof ret_val != "undefined") old_ret = ret_val;
					
					ret_val = queue[0].call(publicAPI,ret_val);
					
					if (typeof ret_val == "undefined") { ret_val = old_ret; }
					else if (ret_val && !(ret_val instanceof Promise)) old_ret = ret_val;
										
					queue.shift();
					
					if (ret_val && ret_val instanceof Promise) {
						promise_fulfilled = false;
						ret_val.then(function(P){ promise_fulfilled = true; return (old_ret = fulfill(P.value)); });
						break;
					}
				}
			}
		}
				
		publicAPI = new Promise();
		
		publicAPI.then = function(cb){
			if (typeof cb == "function") queue[queue.length] = function(val){ return cb.call(publicAPI,{value:val}); };	// then() callback
			else queue[queue.length] = function(val){ return cb; };	// then() value
			if (promise_fulfilled) fulfill(old_ret);
			return publicAPI;
		};
		
		if (cb == null) {	// empty promise
			promise_fulfilled = true;
		}
		else if (typeof cb == "function") {	// promise callback
			cb.call(publicAPI,{fulfill:function(val){
				promise_fulfilled = true;
				fulfill.call(publicAPI,val);
			},value:undef});
		}
		else {	// immediate promise value
			promise_fulfilled = true;
			return publicAPI.then(cb);
		}
		
		return publicAPI;
	};
})();
