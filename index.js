const _getpass = require('./build/Release/getpass');

var getpassFunc = _getpass.getpass;
function isFunction(arg) {
  return typeof arg == "function";
}
_getpass.getpass = function () {
  var args = Array.from(arguments);
  var hasCallback = isFunction(args[0]) || isFunction(args[1]);
  if (hasCallback) {
    return getpassFunc.apply(this, arguments);
  } else {
    return new Promise((resolve, reject) => {
      if (args.length == 0) {
        getpassFunc((err, pass) => {
          if (err) {
            reject(err);
          }
          resolve(pass);
        })
      } else {
        getpassFunc(args[0], (err, pass) => {
          if (err) {
            reject(err);
          }
          resolve(pass);
        })
      }
    })
  }
}

module.exports = _getpass;
