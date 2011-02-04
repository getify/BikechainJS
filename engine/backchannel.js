/*! BikechainJS (backchannel.js)
	v0.0.2 (c) Kyle Simpson
	MIT License
*/

(function(imports){
	var global = this, 
		cfg = {},
		default_cfg = {
			logging: false,
			log_format: "common",
			log_level: 1,
			error_log_level: 2,
			file_logging: false,
			file_log_file: "logs/bikechain.json",
			file_log_error_file: "logs/bikechain.error.json",
			system_event_logging: false,
			display_errors: false
		},
		prepared_cfg = JSON.minify(imports.cfg)
	;
	
	try {
		cfg = JSON.parse(prepared_cfg);
 	}
	catch (err) {
		throw new Error("Failed parsing engine configuration");
	}
	try {
		// merge default values for engine configuration
		for (var k in default_cfg) { if (default_cfg.hasOwnProperty(k)) {
			if (!cfg.hasOwnProperty(k)) cfg[k] = default_cfg[k];
		}}
 	}
	catch (err) {
		throw new Error("Failed initializing engine configuration");
	}
	
	global.getCfg = function(name) {
		return name ? cfg[name] : cfg;
	};
	
	global.setCfg = function(name,value) {
		cfg[name] = value;
	};
})