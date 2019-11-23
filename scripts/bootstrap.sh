#! /bin/bash

mkdir -p vendor
cd vendor

# reason_tag="3.0.4"
# curl -LO "https://github.com/facebook/reason/archive/${reason_tag}.tar.gz"
# mv "${reason_tag}.tar.gz" reason.tar.gz
# tar -xf "reason.tar.gz"
# mv "reason-${reason_tag}" reason

# dune_tag="1.11.3"
# curl -LO "https://github.com/ocaml/dune/archive/${dune_tag}.tar.gz"
# mv "${dune_tag}.tar.gz" dune.tar.gz
# tar -xf "dune.tar.gz"
# mv "dune-${dune_tag}" dune

# cmdliner_tag="1.0.4"
# curl -LO "https://github.com/dbuenzli/cmdliner/archive/v${cmdliner_tag}.tar.gz"
# mv "v${cmdliner_tag}.tar.gz" cmdliner.tar.gz
# tar -xf "cmdliner.tar.gz"
# mv "cmdliner-${cmdliner_tag}" cmdliner

# sexplib_tag="0.12.0"
# curl -LO "https://github.com/janestreet/sexplib/archive/v${sexplib_tag}.tar.gz"
# mv "v${sexplib_tag}.tar.gz" sexplib.tar.gz
# tar -xf "sexplib.tar.gz"
# mv "sexplib-${sexplib_tag}" sexplib

# yojson_tag="1.7.0"
# curl -LO "https://github.com/ocaml-community/yojson/archive/${yojson_tag}.tar.gz"
# mv "${yojson_tag}.tar.gz" yojson.tar.gz
# tar -xf "yojson.tar.gz"
# mv "yojson-${yojson_tag}" yojson

# reason_native_commit="fd4da6d2f0f50cf4a9b5323a8706ff21557db5bb"
# curl -LO "https://github.com/facebookexperimental/reason-native/archive/${reason_native_commit}.tar.gz"
# mv "${reason_native_commit}.tar.gz" reason_native.tar.gz
# tar -xf "reason_native.tar.gz"
# mv "reason-native-${reason_native_commit}" reason-native

# tag="v0.5"
# pkg="merlin-extend"
# curl -LO "https://github.com/let-def/${pkg}/releases/download/${tag}/merlin-extend-${tag}.tbz"
# tar -xf "${pkg}-${tag}.tbz"
# mv "${pkg}-${tag}" "${pkg}"
# rm "${pkg}-${tag}.tbz"

# tag="v3.3.2"
# pkg="merlin"
# curl -LO "https://github.com/ocaml/${pkg}/releases/download/${tag}/${pkg}-${tag}.tbz"
# tar -xf "${pkg}-${tag}.tbz"
# mv "${pkg}-${tag}" "${pkg}"
# rm "${pkg}-${tag}.tbz"

# tag="20190626"
# pkg="menhir"
# curl -LO "https://gitlab.inria.fr/fpottier/${pkg}/repository/${tag}/archive.tar.gz"
# tar -xf "archive.tar.gz"
# mv "menhir-20190626-9158c209f7a86967c837831b6ba64f031e4f3d76" "${pkg}"
# rm "archive.tar.gz"

# tag="1.3.2"
# pkg="easy-format"
# curl -LO "https://github.com/ocaml-community/${pkg}/releases/download/${tag}/${pkg}-${tag}.tbz"
# tar -xf "${pkg}-${tag}.tbz"
# mv "${pkg}-${tag}" "${pkg}"
# rm "${pkg}-${tag}.tbz"

tag="v0.12.2"
pkg="base"
curl -LO "https://github.com/janestreet/${pkg}/archive/${tag}.tar.gz"
tar -xf $tag.tar.gz
mv $pkg-0.12.2 $pkg
rm $tag.tar.gz

# Clean up
# rm vendor/*.tar.*
