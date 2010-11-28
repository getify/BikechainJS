/*! BikechainJS (request.js)
	v0.0.1.4 (c) Kyle Simpson
	MIT License
*/

return (function(){
				 
	var publicAPI,
		SYSTEM = require("system"),
		RESPONSE = require("response"),
		REQUEST_DATA
	;

	function Process() {
		// import the REQUEST_DATA from the web server layer
		try {
			REQUEST_DATA = SYSTEM.stdin.read(false);
			if (!REQUEST_DATA) throw new Error(""); // REQUEST empty, trigger error-catch clause
			REQUEST_DATA = JSON.parse(REQUEST_DATA);
		}
		catch (err) {
			RESPONSE.Header("Status: 500 Internal Error");
			exit();
		}
		
		// process `HTTP Cookie` data
		if (REQUEST_DATA.HTTP_COOKIE) {
			REQUEST_DATA.COOKIES = {};
			var cookies = REQUEST_DATA.HTTP_COOKIE.split(";"), parts, name, value;
			for (var i=0, len = cookies.length; i<len; i++) {
				parts = cookies[i].split("=");
				name = decodeURIComponent(String.trim(parts[0]));
				if (parts[1]) value = decodeURIComponent(parts[1]);
				else value = null;
				REQUEST_DATA.COOKIES[name] = value;
			}
		}
		
		// process `HTTP Post` data
		if (REQUEST_DATA.REQUEST_METHOD == "POST") {
			REQUEST_DATA.POST = {};
			var post_params = REQUEST_DATA.POST_DATA.split("&"), parts, name, value;
			for (var i=0, len = post_params.length; i<len; i++) {
				parts = post_params[i].split("=");
				name = decodeURIComponent(String.trim(parts[0]));
				if (parts[1]) value = decodeURIComponent(parts[1]);
				else value = null;
				REQUEST_DATA.POST[name] = value;
			}
		}
		
		// process `HTTP Get` data
		if (REQUEST_DATA.QUERY_STRING != "") {
			REQUEST_DATA.GET = {};
			var get_params = REQUEST_DATA.QUERY_STRING.split("&"), parts, name, value;
			for (var i=0, len = get_params.length; i<len; i++) {
				parts = get_params[i].split("=");
				name = decodeURIComponent(String.trim(parts[0]));
				if (parts[1]) value = decodeURIComponent(parts[1]);
				else value = null;
				REQUEST_DATA.GET[name] = value;
			}
		}
		
		// parse session cookie data from REQUEST_DATA
		if (REQUEST_DATA.COOKIES && typeof REQUEST_DATA.COOKIES.__session__ != "undefined") {
			var parts = REQUEST_DATA.COOKIES.__session__.split("|");
			REQUEST_DATA.SESSION_NAME = parts[0];
			if (parts[1]) REQUEST_DATA.SESSION_ID = parts[1];
		}

		// augment REQUEST_DATA with helpful translated variables for use in route matching
		REQUEST_DATA.SCRIPT_ROOT				= REQUEST_DATA.SCRIPT_NAME.replace(/^(\/?([^\/]+\/)*).*$/,"$1");
		REQUEST_DATA.RELATIVE_REQUEST_URI		= REQUEST_DATA.REQUEST_URI.replace(new RegExp("^"+REQUEST_DATA.SCRIPT_ROOT),"");
		REQUEST_DATA.RELATIVE_REQUEST_DIRECTORY	= REQUEST_DATA.RELATIVE_REQUEST_URI.replace(/^(\/?([^\/]+\/)*).*$/,"$1");
		REQUEST_DATA.REQUEST_FILENAME			= REQUEST_DATA.RELATIVE_REQUEST_URI.replace(/^(\/?([^\/]+\/)*)([^?]*).*$/,"$3");
		REQUEST_DATA.REQUEST_FILENAME_EXTENSION	= REQUEST_DATA.REQUEST_FILENAME.replace(/^.*?(\.([^.]+))?$/,"$2");
		REQUEST_DATA.RELATIVE_REQUEST_PATH		= REQUEST_DATA.RELATIVE_REQUEST_DIRECTORY + REQUEST_DATA.REQUEST_FILENAME;
		
		return REQUEST_DATA;
	}
	
	function GetData() {
		return REQUEST_DATA;
	}
	
	function GetValue(name) {
		if (REQUEST_DATA.GET && typeof REQUEST_DATA.GET[name] != "undefined") {
			return REQUEST_DATA.GET[name];
		}
		else if (REQUEST_DATA.POST && typeof REQUEST_DATA.POST[name] != "undefined") {
			return REQUEST_DATA.POST[name];
		}
	}
	
	function IsValueSet(name) {
		return (typeof GetValue(name) != "undefined");
	}
	
	function GetSession() {
		return ((REQUEST_DATA.COOKIES && typeof REQUEST_DATA.COOKIES.__session__ != "undefined") ? REQUEST_DATA.COOKIES.__session__ : null);
	}

	publicAPI = {
		Process: Process,
		GetData: GetData,
		GetValue: GetValue,
		IsValueSet: IsValueSet,
		GetSession: GetSession
	};
	return publicAPI;

})();