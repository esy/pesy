/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */;
module type SnapshotIO = {
  let existsFile: string => bool;
  let writeFile: (string, string) => unit;
  let removeFile: string => unit;
  let readFile: string => string;
  let readSnapshotNames: string => list(string);
  let mkdirp: string => unit;
};

module FileSystemSnapshotIO: SnapshotIO = {
  exception SnapshotDirectoryNotFound(string);
  let existsFile = Sys.file_exists;
  let writeFile = (name, contents) => {
    IO.mkdirp(Filename.dirname(name));
    IO.writeFile(name, contents);
  };
  let removeFile = Sys.remove;
  let readFile = IO.readFile;
  let readSnapshotNames = snapshotDir =>
    if (Sys.file_exists(snapshotDir)) {
      snapshotDir
    |> Sys.readdir
      |> Array.to_list
      |> List.map(fileName => Filename.concat(snapshotDir, fileName))
      |> List.filter(filePath => !Sys.is_directory(filePath));
    } else {
      raise(SnapshotDirectoryNotFound(snapshotDir));
    };
  let mkdirp = IO.mkdirp;
};
