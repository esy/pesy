const url = require("url");

function run(urlStr, callback) {
  let httpm;
  let urlObj = url.parse(urlStr);
  switch (urlObj.protocol) {
    case "http:":
      httpm = require("http");
      break;
    case "https:":
      httpm = require("https");
      break;
    default:
      throw `Unrecognised protocol in provided url: ${urlStr}`;
  }
  httpm.get(urlObj, function (response) {
    if (response.statusCode >= 300 && response.statusCode < 400) {
      let urlStr = response.headers.location;
      run(urlStr, (err, resolvedUrl) => {
        if (err) {
          callback(err);
        } else {
          callback(null, resolvedUrl);
        }
      });
    } else if (response.statusCode >= 200 && response.statusCode < 300) {
      return callback(null, urlStr);
    } else callback("Server returned 4xx/5xx for " + urlStr);
  });
}

module.exports.run = run;
