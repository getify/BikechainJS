/*! BikechainJS (response.js)
	v0.0.1.4 (c) Kyle Simpson
	MIT License
*/

return (function(){

	// signal engine of need for content-type header because of web request context
	__SetContentTypeNeeded__(1); 

	var publicAPI,
		SYSTEM = require("system"),
		headers_sent = false,
		content_type_sent = false,
		output_sent = false
	;

	function Header(header) {
		if (output_sent) {
			throw new Error("Output already sent.");
		}
		SYSTEM.stdout.print(header);
		headers_sent = true;
		
		if (!content_type_sent && /^content-type:/i.test(header)) {
			content_type_sent = true;
			__SetContentTypeNeeded__(0);	// signal engine that content-type header sent and thus no longer needed
		}
	}
	
	function Output(out) {
		if (!content_type_sent && !__IsContentTypeNeeded__()) {
			content_type_sent = true;
			headers_sent = true;
		}
		if (!content_type_sent) {
			Header("Content-type: text/plain");
		}
		if (headers_sent && !output_sent) {	// send header/body separator
			SYSTEM.stdout.write("\n");
		}
	
		SYSTEM.stdout.write(out);
		output_sent = true;
	}
	
	function SetCookie(name,value,domain,path,expires) {
		var header = "Set-Cookie: "+name+"="+encodeURIComponent(value)+";";
		if (expires!=null) {
			if (typeof expires != "string") {
				expires = (new Date(expires)).toUTCString();
				expires = expires.replace(/(\d{2,2})\s([a-zA-Z]{3,3})\s(\d{4,4})/,"$1-$2-$3");
			}
			header += " expires="+expires+";";
		}
		if (path!=null) header += " path="+path+";";
		if (domain!=null) header += " domain="+domain+";";
		header += " HttpOnly";
		Header(header);
	}
	
	function SetSessionCookie(session_name,session_id,domain,path,expires) {
		SetCookie("__session__",session_name+"|"+session_id,domain,path,expires);
	}
	
	function HandleSessionCookie(RESPONSE_DATA) {
		var REQUEST = require("request"),
			REQUEST_DATA = REQUEST.GetData()
		;
	
		if (RESPONSE_DATA.SESSION_NAME && RESPONSE_DATA.SESSION_ID && (RESPONSE_DATA.SESSION_FORCE || !REQUEST.GetSession())) {
			SetSessionCookie(RESPONSE_DATA.SESSION_NAME,RESPONSE_DATA.SESSION_ID,REQUEST_DATA.HTTP_HOST,"/");	
		}
	}	
	
	publicAPI = {
		Header: Header,
		Output: Output,
		SetCookie: SetCookie,
		SetSessionCookie: SetSessionCookie,
		HandleSessionCookie: HandleSessionCookie
	};
	return publicAPI;

})();