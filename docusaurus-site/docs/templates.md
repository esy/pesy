---
id: templates
title: Templates
---

# Templates 

It is possible to create templates to be used by pesy. This is a experimental feature with potential breaking changes without notice (we need the flexibility at this point in time).

It works by downloading a git repo and then replacing special strings in filenames and files inside the repo. The special strings are currently these:

| In filename                      | In contents                | Replaced with              |
| -------------------------------- | -------------------------- | -------------------------- |
| \_\_PACKAGE_NAME\_\_             | <PACKAGE_NAME>             | package_name_kebab         |
| \_\_PACKAGE_NAME_FULL\_\_        | <PACKAGE_NAME_FULL>        | package_name_kebab         |
| \_\_PACKAGE_NAME_UPPER_CAMEL\_\_ | <PACKAGE_NAME_UPPER_CAMEL> | PackageNameUpperCamelCase  |
| N/A                              | \<VERSION\>                | version                    |
| N/A                              | <PUBLIC_LIB_NAME>          | package_name_kebab/library |
| N/A                              | <TEST_LIB_NAME>            | package_name_kebab/test    |

The default template can be found here: https://github.com/esy/pesy-reason-template.

To use a custom template run `pesy --template=github:your-name/your-pesy-template`
