module Mode = Mode;
module EsyCommand = EsyCommand;

module Utils = PesyEsyPesyUtils.Utils;
open Utils;
open NoLwt;

type fileOperation =
  | UPDATE(string)
  | CREATE(string);

let gen = (projectPath, pkgPath) => {
  open PesyConf;
  let json = JSON.fromFile(pkgPath);
  let (rootName, pesyPackages) = toPesyConf(projectPath, json);
  let rootNameOpamFile = rootName ++ ".opam";
  let dunePackages = toDunePackages(projectPath, rootName, pesyPackages);

  let operations = ref([]);
  List.iter(
    dpkg => {
      let (path, duneFile) = dpkg;
      let duneFilePath = Path.(path / "dune");

      mkdirp(path);

      let duneFileStr = DuneFile.toString(duneFile);
      try(
        if (duneFile != DuneFile.ofFile(duneFilePath)) {
          write(duneFilePath, duneFileStr);
          operations :=
            [
              UPDATE(
                Str.global_replace(
                  Str.regexp(Path.(projectPath / "")),
                  "",
                  duneFilePath,
                ),
              ),
              ...operations^,
            ];
        }
      ) {
      | _ =>
        write(duneFilePath, duneFileStr);
        operations :=
          [
            CREATE(
              Str.global_replace(
                Str.regexp(Path.(projectPath / "")),
                "",
                duneFilePath,
              ),
            ),
            ...operations^,
          ];
      };
    },
    dunePackages,
  );

  let foundAnOpamFile = ref(false);
  let dirForEachEntry = (f, dirname) => {
    let d = Unix.opendir(dirname);
    try(
      while (true) {
        f(Unix.readdir(d));
      }
    ) {
    | End_of_file => Unix.closedir(d)
    };
  };
  let contains = (n, s) =>
    try(Str.search_forward(Str.regexp(s), n, 0) != (-1)) {
    | Not_found => false
    };

  dirForEachEntry(
    n =>
      if (contains(n, ".opam") && ! foundAnOpamFile^) {
        foundAnOpamFile := true;
        if (n != rootNameOpamFile) {
          copyFile(
            Path.(projectPath / n),
            Path.(projectPath / rootNameOpamFile),
          );
          Unix.unlink(Path.(projectPath / n));
          operations := [CREATE(rootNameOpamFile), ...operations^];
        };
      },
    projectPath,
  );
  operations^;
};

let log = operations => {
  print_newline();
  List.iter(
    o => {
      switch (o) {
      | CREATE(x) =>
        print_endline(
          <Pastel> "    Created " <Pastel bold=true> x </Pastel> </Pastel>,
        )
      | UPDATE(x) =>
        print_endline(
          <Pastel> "    Updated " <Pastel bold=true> x </Pastel> </Pastel>,
        )
      };
      ();
    },
    operations,
  );
  print_newline();
};

let generateBuildFiles = projectRoot => {
  let packageJSONPath = Path.(projectRoot / "package.json");
  gen(projectRoot, packageJSONPath);
};

let build = projectRoot => {
  let packageJSONPath = Path.(projectRoot / "package.json");
  /* Will throw if there validation errors. We catch then at Pesy.re
   * Else, returns the build target name
   */
  PesyConf.validateDuneFiles(projectRoot, packageJSONPath);
};
