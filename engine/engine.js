/*! BikechainJS (engine.js)
	v0.0.1.9 (c) Kyle Simpson
	MIT License
*/


(function(bikechain_root_path, imports){
	var global = this,
		FS, 
		SYSTEM,
		SANDBOX,
		imports_names = [],
		imports_funcs = [],
		loaded_modules = {},
		loaded_includes = {}
	;
	
	for (var i in imports) {
		if (imports.hasOwnProperty(i))	{
			imports_names[imports_names.length] = i;
			imports_funcs[imports_funcs.length] = imports[i];
		}
	}
	
	// special function `require` for loading modules
	global.require = function(module_path,forceReload) {
		forceReload = !(!forceReload);
		var module_repo_path = bikechain_root_path+"modules/", 
			source, module_name
		;
		
		// does module_path need to be canonicized to the module repo directory?
		if (!module_path.match(/^\.{0,2}\//)) {
			module_path = module_repo_path + module_path;
		}
		// does path module name need a .js file extention appended to it?
		if (!module_path.match(/\.js$/i)) {
			module_path += ".js";
		}
		// identify the module name, if it can be matched
		module_name = module_path.match(/\/?([^\/]+)\.js$/i);
		if (module_name) {
			module_name = module_name[1];
		}
		else { // otherwise, bail now
			throw new Error("Failed loading unknown module from path: "+module_path);
		}

		if (!FS) {
			try { source = imports.__FSRead__(module_repo_path+"fs.js"); }
			catch (err) { throw new Error("Failed loading module `fs` from path: "+module_repo_path+"fs.js"); }
			
			loaded_modules["fs"] = FS = Function.apply(global,imports_names.concat([source])).apply(global,imports_funcs);
		}
		if (!forceReload && typeof loaded_modules[module_name] !== "undefined") {
			return loaded_modules[module_name];
		}
		if (module_name != "fs" || forceReload) {
			try { source = FS.read(module_path); }
			catch (err) { throw new Error("Failed loading module `"+module_name+"` from path: "+module_path); }
			
			return (loaded_modules[module_name] = Function.apply(global,imports_names.concat([source])).apply(global,imports_funcs));
		}
		return FS;
	}
	
	// `sbfunction` protects special core functions
	SANDBOX = global.require("sbfunction");
	
	// sandbox `require` as special core function
	global.require = SANDBOX(global.require);
	
	SYSTEM = global.require("system");

	// sandbox `include` as special core function
	global.include = SANDBOX(function(src,forceReload){
		forceReload = !(!forceReload);
		if (!forceReload && typeof loaded_includes[src] != "undefined") {
			return loaded_includes[src];
		}
		try { loaded_includes[src] = FS.read(src); }
		catch (err) { throw new Error("Failed including file from path: "+src); }
		
		eval.call(this,loaded_includes[src]);
	});
	
	// sandbox `include_once` as special core function
	global.include_once = SANDBOX(function(src){
		if (typeof loaded_includes[src] == "undefined") {
			try { global.include.call(this,src); }
			catch (err) { throw new Error("Failed including file from path: "+src); }
		}
	});
	
	// sandbox alert and console.xxx as special core functions
	global.console = {};
	global.alert = SANDBOX(function(){
		for (var i=0, len=arguments.length; i<len; i++) {
			SYSTEM.stdout.print(arguments[i]);
		}
	});
	global.console.info = SANDBOX(function(){
		for (var i=0, len=arguments.length; i<len; i++) {
			imports.__Console__("notice",arguments[i]);
		}
	});
	global.console.log = SANDBOX(function(){
		for (var i=0, len=arguments.length; i<len; i++) {
			imports.__Console__("log",arguments[i]);
		}
	});
	global.console.warn = SANDBOX(function(){
		for (var i=0, len=arguments.length; i<len; i++) {
			imports.__Console__("warn",arguments[i]);
		}
	});
	global.console.error = SANDBOX(function(){
		for (var i=0, len=arguments.length; i<len; i++) {
			imports.__Console__("error",arguments[i]);
		}
	});
	
	// sandbox "exit" as special core function
	global.exit = SANDBOX(function(exit_code){
		exit_code = exit_code || 0;
		imports.__SysExit__(exit_code);
	});

	// pull in custom extension JSON.minify() to strip comments from JSON strings read from files
	include_once(bikechain_root_path+"misc/minify.json.js");
	
	// pull in String.trim() if not defined yet by the JavaScript environment
	include_once(bikechain_root_path+"misc/string.trim.js");
})