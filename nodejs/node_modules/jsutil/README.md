# jsutil.js

## TODO

## Installation

Using npm:

    npm install jsutil
	
## Usage:

	var jsutil = require("jsutil");
	
	var defaultOptions = {
		"host" : "xxx",
		"response_times" : "1",
		"response_mode" : "0"
	};
	
	var alphaOptions = jsutil.mixin({}, defaultOptions, {"user" : "alpha", "passwd" : "alpha"});
	
	var devOptions = jsutil.mixin({}, defaultOptions, {"user" : "dev", "passwd" : "dev"});
