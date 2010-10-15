/*! BikechainJS (sbfunction.js)
	v0.0.1.3 (c) Kyle Simpson
	MIT License
*/

/*  Adapted from:
 *  Fuse JavaScript framework, version Alpha
 *  (c) 2008-2010 John-David Dalton
 *
 *  FuseJS is freely distributable under the terms of an MIT-style license.
 *  For details, see the FuseJS web site: http://www.fusejs.com/
 *
 *--------------------------------------------------------------------------*/

return (function(global) {
  function SubFunc(func) {
    func.__proto__ = funcPlugin;
    return func;
  }

  var funcPlugin, __Function = global.Function;

  // make prototype values conform to ECMA spec and inherit from regular natives
  (SubFunc.prototype = new __Function('')).__proto__ = __Function.prototype;

  // assign constructor property
  (funcPlugin = SubFunc.prototype).constructor = SubFunc;

  return SubFunc;
})(this);
