merlin-extend v0.5
==================

Remove META.transition (defining `merlin_extend`) because reasons (no pun
intended, fix #11).

merlin-extend v0.4 
==================
Mon Jun 24 11:49:32 CEST 2019

!!! `merlin_extend` has been renamed `merlin-extend`, old name is deprecated.

Port to dune and add a documentation website.
There was a problem with the previous naming conventions:
- opam package was `merlin-extend`
- ocamlfind package was `merlin_extend`

Dune enforces a single name, which made the transition slightly harder.
Now only `merlin-extend` should be used but there is still a `merlin_extend`
ocamlfind package to allow a smooth transition.

merlin-extend v0.3 
==================
2016-08-15

Initial release.
