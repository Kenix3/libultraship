---
title: Versioning
nav_order: 7
---

# Versioning

libultraship (LUS) uses [epoch semantic versioning](https://antfu.me/posts/epoch-semver).

## API definition

For LUS, the public API is every C linkage function, variable, struct, class, public class method, or enum included from `libultraship.h`.

## Why epochs

From time to time, LUS goes through large refactors. During those periods, `main` can be unstable while the refactor is in progress.

Epochs let us split that work cleanly:

- `main` moves forward in a new epoch for the refactor.
- The previous epoch enters maintenance mode.
- Fixes and selected features can continue to land in the maintained epoch while the new epoch stabilizes.

This keeps active refactor work unblocked without stopping support for the previously stable line.
