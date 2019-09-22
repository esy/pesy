/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */;
let isSome = (o: option('a)): bool =>
  switch (o) {
  | Some(_) => true
  | None => false
  };
let isNone = (o: option('a)): bool =>
  switch (o) {
  | Some(_) => false
  | None => true
  };
let valuex = (o: option('a)): 'a =>
  switch (o) {
  | Some(value) => value
  | None => CommonErrors.fatal("Expected option to have value but got None")
  };
let valueOr = (default: 'a, o: option('a)): 'a =>
  switch (o) {
  | Some(value) => value
  | None => default
  };
let toBool = valueOr(false);

module Infix = {
  let (>>=) = (o, f) =>
    switch (o) {
    | Some(x) => f(x)
    | None => None
    };
  let (|?:) = (o, default) =>
    switch (o) {
    | Some(value) => value
    | None => default
    };
  let (>>|) = (opt, fn) =>
    switch (opt) {
    | Some(value) => Some(fn(value))
    | None => None
    };
};
