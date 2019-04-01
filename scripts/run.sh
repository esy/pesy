#! /bin/bash

rm -rf _release
esy i &&
esy b &&
esy npm-release &&
cd _release &&
npm pack &&
npm i -g ./pesy-0.5.0-alpha.2.tgz &&
cd ..
./_build/install/default/bin/TestBootstrapper.exe
./_build/install/default/bin/TestPesyConfigure.exe

