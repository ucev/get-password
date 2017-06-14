## getpassword

## Install

```bash
npm install --save getpassword
```

```javascript
const getpass = require('getpassword');
```

## Usage

### `getpass.getPass([options, ]callback)`

Gets a password from the terminal.

This function prints a prompt (by default `请输入密码:`) and then accepts input without echoing.

#### Parameters:

 * `options`, an Object, with properties:
   * `prompt`, an optional String
 * `callback`, a `Func(error, password)`, with arguments:
   * `error`, either `null` (no error) or an `Error` instance
   * `password`, a String

### `getpass.getPass([options])`

Gets a password from the terminal, return a `Promise`. If success, you can get your input-password from the `then` function.
