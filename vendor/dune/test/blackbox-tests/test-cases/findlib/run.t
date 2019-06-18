  $ dune external-lib-deps @install
  These are the external library dependencies in the default context:
  - a
  - b
  - c

Reproduction case for #484. The error should point to src/dune

  $ dune build @install
  File "src/dune", line 4, characters 14-15:
  4 |  (libraries   a b c))
                    ^
  Error: Library "a" not found.
  Hint: try: dune external-lib-deps --missing @install
  [1]

When passing --dev, the profile should be displayed only once (#1106):

  $ jbuilder build --dev @install
  The jbuilder binary is deprecated and will cease to be maintained in July 2019.
  Please switch to dune instead.
  File "src/dune", line 4, characters 14-15:
  4 |  (libraries   a b c))
                    ^
  Error: Library "a" not found.
  Hint: try: dune external-lib-deps --missing --dev @install
  [1]

Note that the hint above is wrong. It doesn't matter too much as this
is for jbuilder which is deprecated and it doesn't seem worth making
the code of dune more complicated to fix the hint.

With dune and an explicit profile, it is the same:

  $ dune build --profile dev @install
  File "src/dune", line 4, characters 14-15:
  4 |  (libraries   a b c))
                    ^
  Error: Library "a" not found.
  Hint: try: dune external-lib-deps --missing --profile dev @install
  [1]
