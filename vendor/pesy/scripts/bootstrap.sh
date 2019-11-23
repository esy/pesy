#! /bin/bash

mkdir -p vendor
cd vendor

reason_tag="3.0.4"
curl -LO "https://github.com/facebook/reason/archive/${reason_tag}.tar.gz"
mv "${reason_tag}.tar.gz" reason.tar.gz
tar -xf "reason.tar.gz"
mv "reason-${reason_tag}" reason

dune_tag="1.9.3"
curl -LO "https://github.com/ocaml/dune/archive/${dune_tag}.tar.gz"
mv "${dune_tag}.tar.gz" dune.tar.gz
tar -xf "dune.tar.gz"
mv "dune-${dune_tag}" dune

cmdliner_tag="1.0.4"
curl -LO "https://github.com/dbuenzli/cmdliner/archive/v${cmdliner_tag}.tar.gz"
mv "v${cmdliner_tag}.tar.gz" cmdliner.tar.gz
tar -xf "cmdliner.tar.gz"
mv "cmdliner-${cmdliner_tag}" cmdliner

sexplib_tag="0.12.0"
curl -LO "https://github.com/janestreet/sexplib/archive/v${sexplib_tag}.tar.gz"
mv "v${sexplib_tag}.tar.gz" sexplib.tar.gz
tar -xf "sexplib.tar.gz"
mv "sexplib-${sexplib_tag}" sexplib

yojson_tag="1.7.0"
curl -LO "https://github.com/ocaml-community/yojson/archive/${yojson_tag}.tar.gz"
mv "${yojson_tag}.tar.gz" yojson.tar.gz
tar -xf "yojson.tar.gz"
mv "yojson-${yojson_tag}" yojson

reason_native_commit="fd4da6d2f0f50cf4a9b5323a8706ff21557db5bb"
curl -LO "https://github.com/facebookexperimental/reason-native/archive/${reason_native_commit}.tar.gz"
mv "${reason_native_commit}.tar.gz" reason_native.tar.gz
tar -xf "reason_native.tar.gz"
mv "reason-native-${reason_native_commit}" reason-native

# Clean up
rm vendor/*.tar.*
