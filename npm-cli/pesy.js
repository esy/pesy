const Pesy = require("./lib/js/src/Pesy.bs.js");

try {
  Pesy.main(process.argv.slice(1)).catch((e) =>
    console.error("Pesy.main rejected promise", e)
  );
} catch (e) {
  console.error("Pesy.main threw an exception", e);
}
