/*! BikechainJS (engine.js)
	v0.0.1.4 (c) Kyle Simpson
	MIT License
*/


(function(imports){
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
		var module_repo_path = "bikechain/modules/", 
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
			return void 0;
		}

		if (!FS) {
			source = imports.__FSRead__(module_repo_path+"fs.js");
			loaded_modules["fs"] = FS = Function.apply(global,imports_names.concat([source])).apply(global,imports_funcs);
		}
		if (!forceReload && typeof loaded_modules[module_name] !== "undefined") {
			return loaded_modules[module_name];
		}
		if (module_name != "fs" || forceReload) {
			source = FS.read(module_path);
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
		loaded_includes[src] = FS.read(src);
		eval.call(this,loaded_includes[src]);
	});
	
	// sandbox `include_once` as special core function
	global.include_once = SANDBOX(function(src){
		if (typeof loaded_includes[src] == "undefined") {
			global.include.call(this,src);
		}
	});
	
	// sandbox alert and console.xxx as special core functions
	global.console = {};
	global.alert = global.console.log = global.console.warn = global.console.error = SANDBOX(function(){
		for (var i=0, len=arguments.length; i<len; i++) {
			SYSTEM.stdout.print(arguments[i]);
		}
	});
	
	// sandbox "exit" as special core function
	global.exit = SANDBOX(function(exit_code){
		exit_code = exit_code || 0;
		imports.__SysExit__(exit_code);
	});

	// pull in custom extension JSON.minify() to strip comments from JSON strings read from files
	include_once("bikechain/misc/minify.json.js");
	
	// pull in String.trim() if not defined yet by the JavaScript environment
	include_once("bikechain/misc/string.trim.js");
})