const crypto = require("crypto");
const fs = require("fs");

function sha256(data, secret) {
  crypto.createHmac("sha256", secret).update(data).digest(secret);
}

function shaFile(path, algo) {
  return new Promise((resolve, reject) => {
    const shasum = crypto.createHash(algo);
    let s = fs.ReadStream(path);
    s.on("data", function (data) {
      shasum.update(data);
    });

    s.on("end", function () {
      var hash = shasum.digest("hex");
      resolve(hash);
    });
    s.on("error", function (error) {
      reject(error);
    });
  });
}

function sha1File(path) {
  return shaFile(path, "sha1");
}

function sha256File(path) {
  return shaFile(path, "sha256");
}

module.exports = { sha256, sha1File, sha256File };
