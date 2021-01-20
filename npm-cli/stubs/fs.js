let { promisify } = require("util");
let fs = require("fs");
let readFile = promisify(fs.readFile);
let writeFile = promisify(fs.writeFile);
let mkdirP = promisify(fs.mkdir);
let exists = promisify(fs.exists);
let stat = promisify(fs.stat);
let readdir = promisify(fs.readdir);
let unlink = promisify(fs.unlink);

let mkdir = (path) => mkdirP(path, { recursive: true });

module.exports = {
  readFile,
  writeFile,
  mkdir,
  exists,
  readdir,
  stat,
  unlink,
};
