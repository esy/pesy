---
id: reweb
title: Web Development with ReWeb
---

ReWeb is a web framework for Reason/OCaml developers that encourages
composable patterns (inspired by 'You Servers as a Function', by
Marius Eriksen) and hopes to be a one stop solution like Rails
(inspired by Jasim Basheer's Rails on OCaml)

`pesy` provides a intuitive and productive setup to get started with
ReWeb.

To get started, run the following command and specify the ReWeb
template

```sh
$ pesy my-reweb-app --template prometheansacrifice/reweb
```

![screenshot-image-here]()

This is will bootstrap a working ReWeb server. You can run it with

```sh
PORT=3000 esy start
```

Let's create a simple blog CMS. To keep things simple (and
relevant to the pesy workflow), we'll keep all the data in-memory
without a persistent database.



Let's add a small authentication layer using JWT signing.
