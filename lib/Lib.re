module Mode = Mode;
module EsyCommand = EsyCommand;

module Utils = PesyEsyPesyUtils.Utils;
open Utils;
open NoLwt;

type fileOperation =
  | UPDATE(string)
  | CREATE(string);

let gen = (projectPath, pkgPath) => {
  let conf = PesyConf.get(pkgPath);
  let pkgs = PesyConf.pkgs(conf);
  let rootName = PesyConf.rootName(conf);

  let rootNameOpamFile = rootName ++ ".opam";

  let operations = ref([]);
  pkgs
  |> List.iter(((subpackage, _)) => {
       mkdirp(Path.(projectPath / subpackage));
       let duneFilePath = Path.(projectPath / subpackage / "dune");
       write(
         duneFilePath,
         {|(* -*- tuareg -*- *)

open Jbuild_plugin.V1

let () =
  run_and_read_lines ("pesy dune-file " ^ Sys.getcwd ())
  |> String.concat "\n"
  |> send
|},
       );
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
     });

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

let build = manifestFile => PesyConf.get(manifestFile) |> PesyConf.rootName;

let duneFile = (projectPath, manifestFile, subpackagePath) => {
  let conf = PesyConf.get(manifestFile);
  let pkgs = PesyConf.pkgs(conf);
  let rootName = PesyConf.rootName(conf);
  let pesyPackages =
    List.map(PesyConf.toPesyConf(projectPath, rootName), pkgs);
  let dunePackages =
    PesyConf.toDunePackages(projectPath, rootName, pesyPackages);
  let normalize = x =>
    x
    |> Str.global_replace(Str.regexp("\\"), "/")
    |> Str.global_replace(Str.regexp("[/|\\]$"), "")
    |> (x => Sys.unix ? x : String.lowercase_ascii(x))
  List.iter(
    dpkg => {
      let (path, duneFile) = dpkg;
      if (path
          |> normalize == (Path.(projectPath / subpackagePath) |> normalize)) {
        print_endline(DuneFile.toString(duneFile));
      } else {
        ();
      };
    },
    dunePackages,
  );
};
