type result =
  | Ok
  | Failure;

let is_directory = p => {
  let stats = Unix.lstat(p);
  switch (stats.st_kind) {
  | S_DIR => true
  | S_LNK
  | S_SOCK
  | S_REG
  | S_CHR
  | S_BLK
  | S_FIFO => false
  };
};

let rec del_file = p =>
  try(
    {
      Unix.unlink(p);
      Ok;
    }
  ) {
  | Unix.Unix_error(Unix.ENOENT, _, _) => Ok
  | Unix.Unix_error(Unix.EISDIR | Unix.EPERM, _, _) => Ok
  | Unix.Unix_error(Unix.EACCES, _, _) when Sys.win32 => Ok
  | Unix.Unix_error(Unix.EINTR, _, _) => del_file(p)
  | Unix.Unix_error(_e, _, _) => Failure
  };

let rec del_dir = p => {
  let rec del_members = (dHandle, p) => {
    switch (
      try(Some(Unix.readdir(dHandle))) {
      | End_of_file => None
      }
    ) {
    | None => Ok
    | Some(".." | ".") => del_members(dHandle, p)
    | Some(f) =>
      switch (run(Filename.concat(p, f))) {
      | _ => del_members(dHandle, p)
      }
    };
  };
  let dh = Unix.opendir(p);
  switch (del_members(dh, p)) {
  | Ok =>
    Unix.closedir(dh);
    if (is_directory(p)) {
      Unix.rmdir(p);
    } else {
      Unix.unlink(p);
    };
    Ok;
  | Failure => Failure
  };
}

and run: string => result =
  p =>
    try(is_directory(p) ? del_dir(p) : del_file(p)) {
    | Unix.Unix_error(e, _, _) =>
      switch (e) {
      | Unix.ENOENT => Ok
      | _ => Failure
      }
    | _ => Failure
    };
