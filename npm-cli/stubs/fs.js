let { promisify } = require("util");
let fs = require("fs");
let readFile = promisify(fs.readFile);
let writeFile = promisify(fs.writeFile);
let mkdir = promisify(fs.mkdir);
let exists = promisify(fs.exists);
let stat = promisify(fs.stat);
let opendir = promisify(fs.opendir);

module.exports = { readFile, writeFile, mkdir, exists, opendir, stat };
