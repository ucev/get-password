const getpass = require('../index.js')
getpass.getpass({prompt: "请输入密码："}).then((pass) => {
  console.log(`你输入的密码是: ${pass}`)
}).catch(e => {
  console.log(e)
});
