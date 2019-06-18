
# rimraf


[![CircleCI](https://circleci.com/gh/yourgithubhandle/rimraf/tree/master.svg?style=svg)](https://circleci.com/gh/yourgithubhandle/rimraf/tree/master)


**Contains the following libraries and executables:**

```
rimraf@0.0.0
│
├─test/
│   name:    TestRimraf.exe
│   require: rimraf/library
│
├─library/
│   library name: rimraf/library
│   require:
│
└─executable/
    name:    RimrafApp.exe
    require: rimraf/library
```

## Developing:

```
npm install -g esy
git clone <this-repo>
esy install
esy build
```

## Running Binary:

After building the project, you can run the main binary that is produced.

```
esy x RimrafApp.exe 
```

## Running Tests:

```
# Runs the "test" command in `package.json`.
esy test
```
