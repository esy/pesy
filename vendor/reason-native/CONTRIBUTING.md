# Contributing to reason-native
We want to make contributing to this project as easy and transparent as
possible.

## Our Development Process
GitHub is the source of truth for this repository. See the README for information
on how to install, build, and modify code within this repository.

## Pull Requests
We actively welcome your pull requests.

1. Fork the repo and create your branch from `master`.
2. If you've added code that should be tested, add tests.
3. If you've changed APIs, update the documentation.
4. Ensure the test suite passes.
5. Make sure your code lints.
6. If you haven't already, complete the Contributor License Agreement ("CLA").


## Releasing

To release packages, run the `./scripts/esy-prepublish.js` script supplying the
relative paths to various `.json` files you wish to publish. To release package
`@reason-native/foo` which has a `foo.json` file in the repo root, run

```sh
node ./scripts/esy-prepublish.js ./foo.json
```

Then follow the instructions that are printed.



## Contributor License Agreement ("CLA")
In order to accept your pull request, we need you to submit a CLA. You only need
to do this once to work on any of Facebook's open source projects.

Complete your CLA here: <https://code.facebook.com/cla>

## Issues
We use GitHub issues to track public bugs. Please ensure your description is
clear and has sufficient instructions to be able to reproduce the issue.

Facebook has a [bounty program](https://www.facebook.com/whitehat/) for the safe
disclosure of security bugs. In those cases, please go through the process
outlined on that page and do not file a public issue.

## Coding Style  
* 2 spaces for indentation rather than tabs
* 80 character line length
* Format code using `refmt`

## License
By contributing to reason-native, you agree that your contributions will be licensed
under the LICENSE file in the root directory of this source tree.
