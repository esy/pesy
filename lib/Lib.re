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

let normalize = x =>
  x
  |> Str.global_replace(Str.regexp("\\"), "/")
  |> Str.global_replace(Str.regexp("[/|\\]$"), "")
  |> (x => Sys.unix ? x : String.lowercase_ascii(x));

exception InvalidPackagePath(string);
let duneFile = (projectPath, manifestFile, subpackagePath) => {
  let normalizedSubpackagePath = normalize(subpackagePath);
  let conf = PesyConf.get(manifestFile);
  let pkgs = PesyConf.pkgs(conf);

  let duneProjectPath = Path.(projectPath / "dune-project");
  let duneVersion =
    DuneFile.ofFile(duneProjectPath) |> DuneProject.findLangVersion;

  let rootName = PesyConf.rootName(conf);
  let pesyPackages =
    pkgs |> List.map(PesyConf.toPesyConf(projectPath, rootName, ~duneVersion));
  /** TODO: Why compute for every subpackage? */
  PesyConf.(
    switch (
      pesyPackages
      |> List.find_opt(pesyPackage =>
           normalize(pesyPackage.pkg_path) == normalizedSubpackagePath
         )
    ) {
    | None =>
      raise(
        InvalidPackagePath(
          "No package found with path: " ++ normalizedSubpackagePath,
        ),
      )
    | Some(pesyPackage) =>
      let (_, duneFile) =
        PesyConf.toDunePackages(projectPath, rootName, pesyPackage);
      DuneFile.toString(duneFile) |> print_endline;
    }
  );
};

let duneEject = (projectPath, manifestFile, subpackageNameOrPath) => {
  let normalizedSubpackageNameOrPath = normalize(subpackageNameOrPath);
  let conf = PesyConf.get(manifestFile);
  let pkgs = PesyConf.pkgs(conf);

  let duneProjectPath = Path.(projectPath / "dune-project");
  let duneVersion =
    DuneFile.ofFile(duneProjectPath) |> DuneProject.findLangVersion;


  let rootName = PesyConf.rootName(conf);
  let pesyPackages =
    pkgs
    |> List.map(((pkgName, _) as pkg) =>
         (pkgName, PesyConf.toPesyConf(projectPath, rootName, pkg, ~duneVersion))
       );

  switch (
    pesyPackages
    |> List.find_opt(((pkgName, pesyPackage)) => {
         PesyConf.(
           normalize(pesyPackage.pkg_path) == normalizedSubpackageNameOrPath
           || pkgName == normalizedSubpackageNameOrPath
         )
       })
  ) {
  | None =>
    raise(
      InvalidPackagePath(
        "No package found with path or name: "
        ++ normalizedSubpackageNameOrPath,
      ),
    )
  | Some((pkgNameToEject, pesyPackage)) =>
    let (_, duneFile) =
      PesyConf.toDunePackages(projectPath, rootName, pesyPackage);
    let newPkgs =
      pkgs |> List.filter(((pkgName, _)) => pkgName != pkgNameToEject);
    let newConf =
      switch (conf) {
      | `Assoc(newJson) =>
        `Assoc(
          newJson
          |> List.map(((fieldName, _) as field) =>
               fieldName != "buildDirs"
                 ? field : (fieldName, `Assoc(newPkgs))
             ),
        )
      | _ => conf
      };

    write(
      Path.(projectPath / "package.json"),
      JSON.pretty_to_string(newConf),
    );
    write(Path.(pesyPackage.pkg_path / "dune"), DuneFile.toString(duneFile));
  };
};
