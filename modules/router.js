/*! BikechainJS (router.js)
	v0.0.1.4 (c) Kyle Simpson
	MIT License
*/

return (function(){

	var publicAPI,
		uri_routing_rules = []
	;

	function checkRules(REQUEST) {
		var REQUEST_DATA = REQUEST.GetData(), i, len, handle = true, 
			field, default_field = "RELATIVE_REQUEST_PATH", regex
		;
		
		for (i=0, len=uri_routing_rules.length; i<len; i++) {
			field = uri_routing_rules[i].field || default_field;
			regex = new RegExp(uri_routing_rules[i].handle || uri_routing_rules[i].ignore);
			if (typeof REQUEST_DATA[field] != "undefined" && regex.test(REQUEST_DATA[field])) {
				handle = (typeof uri_routing_rules[i].handle != "undefined");
			}
		}
		
		return handle;
	}
	
	function RegisterRoutes(routes_filename) {
		var FS = require("fs"), rules;

		try {
			rules = FS.read(routes_filename);
			if (!rules) throw new Error(""); // trigger error-catch clause since rules is empty
			rules = JSON.parse(JSON.minify(rules)); 
			rules.concat(); // ensure `rules` is an array
		}
		catch (err) { return false; }
		
		uri_routing_rules = uri_routing_rules.concat(rules);
		
		return true;
	}
	
	function HandleRequest(REQUEST) {
		if (checkRules(REQUEST)) {
			return true;
		}
		return false;
	}
	
	function GetRequestPath(REQUEST) {
		var REQUEST_DATA = REQUEST.GetData(), i, len, field, prefix = "", 
			default_field = "RELATIVE_REQUEST_PATH", regex, rules = []
		;
		
		for (i=0, len=uri_routing_rules.length; i<len; i++) {
			field = uri_routing_rules[i].field || default_field;
			regex = new RegExp(uri_routing_rules[i].handle || uri_routing_rules[i].ignore);
			if (typeof REQUEST_DATA[field] != "undefined" && regex.test(REQUEST_DATA[field]) && typeof uri_routing_rules[i].path_prefix != "undefined") {
				prefix = uri_routing_rules[i].path_prefix;
			}
		}
		return prefix + REQUEST_DATA.RELATIVE_REQUEST_PATH;
	}
	
	function GetHandleRules(REQUEST) {
		var REQUEST_DATA = REQUEST.GetData(), i, len, field, 
			default_field = "RELATIVE_REQUEST_PATH", regex, rules = []
		;
		
		for (i=0, len=uri_routing_rules.length; i<len; i++) {
			field = uri_routing_rules[i].field || default_field;
			if (uri_routing_rules[i].handle) {
				regex = new RegExp(uri_routing_rules[i].handle);
				if (typeof REQUEST_DATA[field] != "undefined" && regex.test(REQUEST_DATA[field])) {
					rules.push(uri_routing_rules[i]);
				}
			}
		}
		return rules;
	}

	publicAPI = {
		RegisterRoutes: RegisterRoutes,
		HandleRequest: HandleRequest,
		GetRequestPath: GetRequestPath,
		GetHandleRules: GetHandleRules
	};
	return publicAPI;

})();