#! /bin/bash

# TODO: Add npm install -g verdaccio

root=$PWD
custom_registry_url=http://localhost:4873
original_npm_registry_url=https://registry.npmjs.org # `npm get registry`
original_yarn_registry_url=https://registry.yarnpkg.com # `yarn config get registry`
version=0.5.0-alpha.17


function cleanup {

  if [[ ! -z "$root" ]]
  then
      rm -rf $root/scripts/storage
      rm -rf $root/scripts/htpasswd
  fi

  # TODO: kill verdaccio. Makes it convenient for local dev
  # ps -aux | grep 'verdaccio' | grep -v grep | awk '{print $2}' | xargs kill -9

  echo Resetting registries
  npm set registry "$original_npm_registry_url" # https://registry.npmjs.org
  yarn config set registry "$original_yarn_registry_url" # http://registry.yarnpkg.com/
  export NPM_CONFIG_REGISTRY=
  
  echo Clean up _release
  rm -rf _release
}


function main {
    echo 'Cleaning up.'
    cleanup

    echo esy install
    esy i

    echo esy build
    esy b --install

    # echo esy npm-release
    # esy npm-release || exit -1

    # echo Entering _release
    # cd _release || exit -1

    # echo Simulate latest stable release
    # node $root/scripts/simulate-latest.js $root/_release/package.json $version

    # echo Npm packing...
    # npm pack

    cd npm-cli

    echo Globally instaling the packed pesy
    npm i -g .

    tmp_registry_log=`mktemp`
    echo "Booting up verdaccio (log: $tmp_registry_log) (config: $root/scripts/verdaccio.yaml) "

    (cd && nohup npx verdaccio@3.8.2 -c $root/scripts/verdaccio.yaml | tee $tmp_registry_log &)
    grep -q 'http address' <(tail -f $tmp_registry_log)

    echo Set registry to local registry
    npm set registry "$custom_registry_url"
    yarn config set registry "$custom_registry_url"

    (npx npm-auth-to-token@1.0.0 -u user -p password -e user@example.com -r "$custom_registry_url")

    echo Publishing to local npm
    npm publish .

    echo NPM info
    npm info pesy

    # echo Writing pesy meta data to pesy-meta.jso
    # curl -H "Accept: application/vnd.npm.install-v1+json" http://localhost:4873/pesy | python -m json.tool > $root/pesy-meta.json

    pwd

    echo Entering root again
    cd ..

    pwd

    export NPM_CONFIG_REGISTRY="$custom_registry_url"
    ./_build/install/default/bin/TestBootstrapper.exe
    PESY_CLONE_PATH=$PESY_CLONE_PATH ./_build/install/default/bin/TestPesyConfigure.exe

    echo 'Cleaning up again'
    cleanup
}

if [ ! -z "$1" ] && [ "$1" == "clean" ]
then
    cleanup
else
    if [ -z "${PESY_CLONE_PATH}" ]
    then
        echo PESY_CLONE_PATH not defined in env
        exit -1
    fi
    main
fi
