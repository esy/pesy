1.3.2 (2019-08-02)
------------------

- Port from jbuilder to dune. (#24)

- Port to opam 2.0 and make dune a non build dependency (#25)

1.0.1 (2012-02-03)
------------------

- Nothing new other than the way of building the tar.gz package.

2008-07-13: Release 1.0.0, slightly incompatible with 0.9.0

Incompatibilities:
- Deprecated use of Easy_format.Param. Instead, inherit from Easy_format.list,
  Easy_format.label or Easy_format.atom.
- Atom nodes have now one additional argument for parameters.
- All record types have been extended with more fields.
  Using the "with" mechanism for inheritance is the best way of limiting
  future incompatibilities.

New features:
- Support for separators that stick to the next list item
- More wrapping options
- Added Custom kind of nodes for using Format directly or existing
  pretty-printers
- Support for markup and escaping, allowing to produce colorized output
  (HTML, terminal, ...) without interfering with the computation of
  line breaks and spacing.

0.9.0 (2008-07-09)
------------------

- Initial release
