/*! BikechainJS (fs.js)
	v0.0.1.3 (c) Kyle Simpson
	MIT License
*/

return (function(){
	var publicAPI;
	
	function read(file) {
		return FSREAD(file);
	}
	
	function write(file,content) {
		return FSWRITE(file,content);
	}
	
	publicAPI = {
		read:read,
		write:write
	};	
	return publicAPI;
})();