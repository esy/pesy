// rollup.config.js
import commonjs from "@rollup/plugin-commonjs";
import nodeResolve from "@rollup/plugin-node-resolve";
import json from "@rollup/plugin-json";

export default {
  input: "./pesy.js",
  output: {
    file: "pesy.bundle.js",
    format: "cjs",
    sourcemap: true,
  },
  plugins: [
    nodeResolve({ preferBuiltins: true }),
    json(),
    commonjs({
      ignore: ["child_process"],
    }),
  ],
  external: [
    "child_process",
    "fs",
    "util",
    "chalk",
    "source-map-support",
    "request",
    "request-progress",
    "process",
    "https",
    "walk-sync",
    "download-git-repo",
    "path",
    "readline",
    "readable-stream",
  ],
};
