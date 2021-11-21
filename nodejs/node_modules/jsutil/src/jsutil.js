
var util = {

	mixin : function(dest, sources){
		dest = dest || {};
		for(var i = 1, ii = arguments.length; i < ii; i++){
			util._mixin(dest, arguments[i]);
		}
		return dest;
	},

	extend : function(ctor, props){
		for(var i = 1, ii = arguments.length; i < ii; i++){
			util._mixin(ctor.prototype, arguments[i]);
		}
		return ctor;
	},
	
	_mixin : function(dest, source, copyFunc){
		var name, s, i, empty = {};
		for(name in source){
			s = source[name];
			if(!(name in dest) || (dest[name] !== s && (!(name in empty) || empty[name] !== s))){
				dest[name] = copyFunc ? copyFunc(s) : s;
			}
		}
		return dest; 
	}
};

module.exports = util;

