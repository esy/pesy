const fs = require("fs");
const https = require("https");
const path = require("path");
const cp = require("child_process");
const util = require("util");

let tag = "0.1.0-alpha.21";
let url = `https://github.com/esy/pesy-reason-template/archive/${tag}.zip`;
let downloadAs = "template.zip";
let extractedDirName = `pesy-reason-template-${tag}`;
let get = (url) =>
  new Promise((resolve, reject) => {
    let request = https.get(url, function (response) {
      resolve({ request, response });
    });
  });

let downloadFromHTTPStream = (response, p) =>
  new Promise((resolve, reject) => {
    response.pipe(fs.createWriteStream(p));
    response.on("end", resolve);
  });

let fetch = (url) => {
  return get(url).then(({ response }) => {
    if (response.statusCode === 302) {
      let redirectUri = response.headers["location"];
      if (redirectUri) {
        return fetch(redirectUri);
      } else {
        return Promise.reject(new Error("Redirect location was not found"));
      }
    } else {
      return downloadFromHTTPStream(
        response,
        path.resolve(templateDir, zipFile),
      );
    }
  });
};

let delay = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

let templateDir = "templates";
let zipTarget = path.resolve(__dirname, "..", templateDir, extractedDirName);
let zipFile = "template.zip";
let main = () => {
  if (!fs.existsSync(path.resolve(".", templateDir))) {
    fs.mkdirSync(path.resolve(".", templateDir));
  }
  let p;
  if (!fs.existsSync(path.resolve(".", templateDir, zipFile))) {
    p = fetch(url);
  } else {
    p = Promise.resolve();
  }
  p.then(() => delay(100))
    .then(() => {
      if (!fs.existsSync(zipTarget)) {
        console.log(
          `Extracting (entering: ${path.resolve(
            __dirname,
            "..",
            templateDir,
          )})...`,
        );
        return Promise.resolve(
          cp
            .execSync("unzip -o template.zip", {
              cwd: path.resolve(__dirname, "..", templateDir),
            })
            .toString(),
        );
      }
    })
    .catch((e) => {
      console.error(e);
      process.exit(-1);
    });
};

main();
