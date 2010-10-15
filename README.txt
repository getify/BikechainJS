BikechainJS Engine
v0.0.1.3 (c) 2010 Kyle Simpson
MIT License


BikechainJS is a minimal server-side JavaScript wrapper environment (engine) around V8. It consists of a single executable "engine" which takes one or more JavaScript files as arguments, and executes them.

"engine.js" is the bootstrapper for the global environment setup. It is required (in the same directory as the "engine") and is automatically loaded, so must not be manually specified to be loaded.


--------------

Installation:

1. Make sure you have a compiled, functional V8:  http://code.google.com/p/v8/
 -- NOTE: You need to use V8 v2.1.2+ because a bug was fixed with revision 3924

2. If you created the "shared" V8 library (recommended), proceed to step 4.

3. If you created the "static" V8 library, edit the makefile to reference to proper static library file.

4. Run "make" and then "make clean".

5. You should now have a "engine" executable in the root directory. You can execute a JavaScript file by passing it as a parameter to engine, like this:

./engine dosomething.js


--------------

Provided global environment:

1. require(module-name): require() is used to load a module. The module name is case-sensitive and must not contain the file extension (.js). A loaded module does not automatically create anything in the global namespace. Instead, the module instance is instead returned from the require() call.

2. include(path-to-file,[forceReload]): include() is used to load/parse a javascript file into the executing environment. The filename should be specified completely, including any relative or absolute path as necessary. "forceReload" is an optional bool parameter (defaults to false) forces the module sub-system to reload the module manually from the file system. Otherwise, modules are cached when loaded to improve performance.

3. include_once(path-to-file): will ensure an exact file (via path) only gets loaded/parsed once.

4. alert(), console.log(), console.warn(), and console.error() are all defined and mapped to [system].stdout.print().

5. exit() to immediately stop execution of any javascript in this instance and flush output.


--------------

Modules:

Several modules are available to be loaded into the executing environment by using the global require() function.

1. "system": System (standard I/O)

 *  [system].stdout.write(str): writes "str" to the standard output
 *  [system].stdout.print(str): writes "str" to the standard output, followed by a new-line
 *  [system].stdout.flush(): flushes the output buffer (if necessary)
 *  [system].stdin.read(): if stdin has any buffered, reads from stdin up until an EOF. Otherwise, read returns empty immediately (non-blocking).
 *  [system].stderr.write(str): same as stdout.write()
 *  [system].stderr.print(str): same as stdout.print()
 *  [system].stderr.flush(): same as stdout.flush()


2. "fs": Filesystem (file I/O)

 *  [fs].read(file): returns the entire contents of the file
 *  [fs].write(file,str): writes "str" to "file"


3. "os": Operating System (process execution)

 *  [os].command(cmd, [..cmds, ..]): execute a command on the local system, specified by "cmd" and "cmds" parameters
    -- returns iopipe:
         [iopipe].stdout.read(): reads the output from the executed command


4. "promise": "Promises" (sync/async deferral)

 * [promise](func): Example: var Promise = require("promise"); Promise(func);
   --Passes a promise "P" object to func as first parameter
     -- "P" has a .fulfill([val]) function which specifies the promise is finished/fulfilled, and optionally passes along a "val" value.
   --Returns an object that can be chained off of, with a .then(func) function, which gets a promise object "P" passed to it
     -- "P" has a .value property which is the chained/passed value from the fulfilled promise

 Example:

 var Promise = require("promise");
 Promise(function(P){
 	doAsync(function(){ P.fullfill("Done"); });
 })
 .then(function(P){
 	alert(P.value); // "Done"
 });


5. "sbfunction": "Sandbox Functions" (protects special core functions by sandboxing them)

 * [sb](func): Example: var sbfunc = require("sbfunction"); func = sbfunc(func);
   --sandboxes the passed in function and returns it

 Example:

 function myfunc() { ... }

 var sbfunc = require("sbfunction");
 var myfunc = sbfunc(myfunc);

6. "request": Request Handler (utilities for managing the inbound REQUEST)

 * [request].parse(REQUEST): parses the request for GET, POST, COOKIE, and other helpful environment variables
   --Returns the augmented REQUEST object
 * [request].value(REQUEST, name): retrieves the value (if any) of the variable `name` from GET or POST
   --Returns the string value of variable `name`
 * [request].exists(REQUEST, name): determines if the variable `name` exists in GET or POST
   --Returns true/false boolean

7. "response": Response Handler (utilities for managing the outbound RESPONSE)

 * [response].Header(name, value): sends a response header
 * [response].Output(name, value): ends response headers' section and begin content body
 * [response].SetCookie(name, value, domain, path, expires): sends a Set-Cookie header
 * [response].SessionCookie(session_name, session_id, domain, path, expires): sends a session cookie header

8. "router": URI Router (handles routing decisions)

 * [router].RegisterRouters(routes_filename): reads and registers route rules from `routes_filename`
   --Returns true/false boolean
 * [router].HandleRequest(REQUEST): determines if the REQUEST should be handled, based on the route rules
   --Returns true/false boolean
 * [router].RequestPath(): retrieves the RELATIVE_REQUEST_PATH from the REQUEST
   --Returns the string value of the RELATIVE_REQUEST_PATH of the REQUEST
