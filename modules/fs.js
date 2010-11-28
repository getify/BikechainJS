/*! BikechainJS (fs.js)
	v0.0.1.4 (c) Kyle Simpson
	MIT License
*/

return (function(){
	var publicAPI;
	
	function read(file) {
		return __FSRead__(file);
	}
	
	function write(file,content) {
		return __FSWrite__(file,content);
	}
	
	publicAPI = {
		read: read,
		write: write
	};	
	return publicAPI;
})();