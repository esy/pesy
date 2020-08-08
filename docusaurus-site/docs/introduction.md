---
id: introduction
title: Introduction
---


# What is pesy?

`pesy` is a command line tool to assist with your native Reason/OCaml
workflow using the package.json itself.

`esy` - Reason community's favorite package manager is driven by a
`package.json` that contains dependencies, tasks (like docs, tests, or
basically anything you specify in the "scripts" section like you do
with npm) etc. It's very convenient to expect an `npm init` like
command that can quickly get us a great starting point. With native
projects, simply having a `package.json` dumped is never enough - we
need some mimimal build setup with good set of default and intuitive
config. This is where `pesy` comes in!

`pesy` provides 

1. A bootstrapper script to quickly create a project template
2. An alternative JSON syntax around Dune that is more NPM like
3. Built in tasks that take full advantages of esy's capabilities

Checkout [A simple native Reason project with pesy](/docs/simple-native) to get an
idea of what developing with `pesy` feels like.
