let crypto = require("crypto"),
  path = require("path"),
  fs = require("fs");

let algorithm = "sha1",
  shasum = crypto.createHash(algorithm);

let cwd = process.cwd();
let pkg = require(path.join(cwd, "package.json")),
  s = fs.ReadStream(
    path.join(cwd, "_release", `${pkg.name}-${pkg.version}.tgz`)
  );

s.on("data", function(data) {
  shasum.update(data);
});

s.on("end", function() {
  var hash = shasum.digest("hex");
  console.log(`SHA1: ${hash}`);
});
