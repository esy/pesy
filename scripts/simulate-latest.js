let path = require("path");
let fs = require("fs");
let packageJSONPath = path.join(process.argv[2]);
let packageJSON = require(packageJSONPath);

fs.writeFileSync(
  packageJSONPath,
  JSON.stringify(Object.assign({}, packageJSON, { version: process.argv[3] }))
);
