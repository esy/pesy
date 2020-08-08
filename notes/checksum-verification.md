# Checksum verification

As we create the build artifacts to publish to NPM, we also generate the SHA1 hash
of the `.tgz` file created by `npm pack`, in a manner similar to how npm does.
This way, you can verify that the package published to NPM is infact the same
set of binaries that were built on CI.

You can verify this by following this simple steps.

1. Head over to CI logs as per the release version

   a. [Pre-beta](https://dev.azure.com/pesy/pesy/_build/results?buildId=103)

2) Navigate to the `Release Job` section

![release-job](./images/release-job.png "Release job section")

3. Look for 'Calculating sha1'

![calculating-sha1](./images/calculating-sha1.png "Calculating sha1 option in the logs")

4. Verify its the same as the one in `npm info pesy`. Of course, ensure that the version
   you see in `npm info pesy` is the same the one in the logs.

![sha1-logs](./images/sha1-logs.png "SHA1 hash in the logs")

You can also download the package straight from the CI and check if it is the
same as the one on NPM.

1. In the same logs, on the top right you would see a blue button labeled `Artifacts`

![top-right-corner](./images/top-right-corner.png "Artifacts button on the top right corner")

2. In the sub menu drawn up by `Artifacts`, click on `Release`. This is the job where we
   collect are platform binaries and package them for NPM. You'll only find platform-specfic
   binaries in the other jobs.

![release-option](./images/release-option.png "Release option")

3. A file explorer like interface opens on clicking `Release` as explained in the previous step.
   Click on the `Release` folder - the only option. We run `npm pack` in this directory structure.

![artifacts-explorer](./images/artifacts-explorer.png "Artifacts explorer")

4. `pesy-<version>.tgz` is the tar file that published to npm. You can uncompress and inspect its
   contents, or check its SHA1 integrity and ensure it's the same as the one on NPM

![download](./images/download.png "Download option for pesy-<version>.tgz")

5. You might have to tap on the file once to show the kebab menu.

![tap-for-kebab](./images/tap-for-kebab.png "Tap for Kebab menu")


