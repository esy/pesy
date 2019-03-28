let crypto = require("crypto"),
  path = require("path"),
  fs = require("fs");

let algorithm = "sha1",
  shasum = crypto.createHash(algorithm);

let filename = path.join(process.cwd(), process.argv[2]),
  s = fs.ReadStream(filename);

s.on("data", function(data) {
  shasum.update(data);
});

s.on("end", function() {
  var hash = shasum.digest("hex");
  console.log(`SHA1: ${hash}`);
});
