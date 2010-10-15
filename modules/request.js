/*! BikechainJS (request.js)
	v0.0.1.3 (c) Kyle Simpson
	MIT License
*/

return (function(){
				 
	var publicAPI;

	function parse(REQUEST_DATA) {
		// clone the object so as not to disturb the original -- since must be JSON, this is safe
		REQUEST_DATA = JSON.parse(JSON.stringify(REQUEST_DATA));
		
		// process HTTP Cookies from REQUEST_DATA
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
		
		// process HTTP POST data
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
		
		// process HTTP GET data
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
		
		// augment REQUEST_DATA with helpful translated variables for use in route matching
		REQUEST_DATA.SCRIPT_ROOT				= REQUEST_DATA.SCRIPT_NAME.replace(/^(\/?([^\/]+\/)*).*$/,"$1");
		REQUEST_DATA.RELATIVE_REQUEST_URI		= REQUEST_DATA.REQUEST_URI.replace(new RegExp("^"+REQUEST_DATA.SCRIPT_ROOT),"");
		REQUEST_DATA.RELATIVE_REQUEST_DIRECTORY	= REQUEST_DATA.RELATIVE_REQUEST_URI.replace(/^(\/?([^\/]+\/)*).*$/,"$1");
		REQUEST_DATA.REQUEST_FILENAME			= REQUEST_DATA.RELATIVE_REQUEST_URI.replace(/^(\/?([^\/]+\/)*)([^?]*).*$/,"$3");
		REQUEST_DATA.REQUEST_FILENAME_EXTENSION	= REQUEST_DATA.REQUEST_FILENAME.replace(/^.*?(\.([^.]+))?$/,"$2");
		REQUEST_DATA.RELATIVE_REQUEST_PATH		= REQUEST_DATA.RELATIVE_REQUEST_DIRECTORY + REQUEST_DATA.REQUEST_FILENAME;
		
		return REQUEST_DATA;
	}
	
	function value(REQUEST_DATA,name) {
		if (REQUEST_DATA.GET && REQUEST_DATA.GET[name]) {
			return REQUEST_DATA.GET[name];
		}
		else if (REQUEST_DATA.POST && REQUEST_DATA.POST[name]) {
			return REQUEST_DATA.POST[name];
		}
	}
	
	function exists(REQUEST_DATA,name) {
		return (typeof value(REQUEST_DATA,name) != "undefined");
	}

	publicAPI = {
		parse:parse,
		value:value,
		exists:exists
	};
	return publicAPI;

})();