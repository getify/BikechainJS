/*! BikechainJS (engine.js)
	v0.0.1.1 (c) Kyle Simpson
	MIT License
*/


(function(imports){
	var global = this,
		FS, 
		SYSTEM,
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
	
	global.require = function(module,forceReload) {
		forceReload = !(!forceReload);
		var source;

		if (!FS) {
			source = imports.FSREAD("modules/fs.js");
			loaded_modules["fs"] = FS = Function.apply(global,imports_names.concat([source])).apply(global,imports_funcs);
		}
		
		if (!forceReload && typeof loaded_modules[module] !== "undefined") return loaded_modules[module];
		
		if (module !== "fs") {
			source = FS.read("modules/"+module+".js");
			return (loaded_modules[module] = Function.apply(global,imports_names.concat([source])).apply(global,imports_funcs));
		}
		return FS;
	}
	
	var sbfunc = global.require("sbfunction");
	
	global.require = sbfunc(global.require);	// sandbox "require" as special core function
	
	SYSTEM = global.require("system");

	global.include = sbfunc(function(src,forceReload){		// sandbox "include" as special core function
		forceReload = !(!forceReload);
		
		if (!forceReload && typeof loaded_includes[src] !== "undefined") return loaded_includes[src];
		
		loaded_includes[src] = FS.read(src);
		
		eval.call(this,loaded_includes[src]);
	});
	
	global.include_once = sbfunc(function(src){		// sandbox "include_once" as special core function
		if (typeof loaded_includes[src] === "undefined") global.include.call(this,src);
	});
	
	global.console = {};
	global.alert = global.console.log = global.console.warn = global.console.error = sbfunc(function(){	// sandbox alert and console.xxx as special core functions
		for (var i=0, len=arguments.length; i<len; i++) {
			SYSTEM.stdout.print(arguments[i]);
		}
	});
	
	global.exit = sbfunc(function(exit_code){	// sandbox "exit" as special core function
		exit_code = exit_code || 0;
		imports.SYSEXIT(exit_code);
	});
})