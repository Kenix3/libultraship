---
title: Versioning
nav_order: 7
has_children: true
---

# Versioning

libultraship (LUS) uses [epoch semantic versioning](https://antfu.me/posts/epoch-semver).

## API definition

For the new epoch, versioning scope includes everything that is public within the `Ship` namespace.

This replaces the previous epoch rule that treated every C linkage function, variable, struct, class, public class method, or enum included from `libultraship.h` as part of the versioned API.

## Why epochs

LUS is a library undergoing active architectural improvements. From time to time, we perform large refactors that touch core systems - context initialization, component lifecycle, resource management, or event dispatch.

### The problem with traditional semver

With standard semantic versioning, major refactors create difficult tradeoffs:

- **Option A**: Land breaking changes on `main` and hold off tagging a new major release until the refactor is complete.
  - **Problem**: Commits landing on `main` mid-refactor are not production-ready. They may change again, lack clear migration paths, and have not been fully tested. Ports cannot safely use these commits.
  - **Additional issue**: If only the latest major version is maintained, ports cannot land small breaking changes during the refactor period (which may span years).

- **Option B**: Hold all refactor work on a long-lived feature branch.
  - **Problem**: The branch diverges significantly, merge conflicts accumulate, and collaboration becomes difficult. This happens regardless of versioning strategy.

- **Option C**: Release each breaking change as a new major version during refactor.
  - **Problem**: A major version bump does not clearly communicate whether it is one argument change or a complete architectural overhaul. Mid-refactor releases lack clear migration paths and stable foundations.

### How epochs solve this

Epochs add a leading number to the version: `epoch.major.minor.patch` (for example, `1.2.3.4` or `2.0.0.0`).

- **During a major refactor**: `main` moves to a new epoch (for example, `2.x.x.x`). This signals that `main` is not production-ready and commits may change during the refactor.
- **The previous epoch** (for example, `1.x.x.x`) continues as a maintained branch:
  - Critical fixes backported
  - **Small breaking changes can land** (incrementing major within that epoch)
  - Stable baseline for ports not yet ready to migrate
  - Production-ready releases continue

This approach:
- Provides a clear place to land breaking changes without releasing/supporting mid-refactor code
- Allows small breaking changes on the stable epoch while the refactor progresses
- When the refactor completes, the new epoch becomes the supported production line
- Version numbers clearly distinguish architectural overhauls from targeted changes

### When to expect a new epoch

A new epoch typically begins when:
- Core architectural patterns change (for example, context/component model redesign)
- Multiple subsystems require coordinated breaking changes
- The refactor is expected to span several months of incremental work

Small or targeted breaking changes within an epoch still follow semantic versioning rules (bump `major` within that epoch). For example, `2.23.0.1` is valid if 23 targeted breaking changes landed in the `2.x.x.x` epoch, each with a clear migration path and production-ready code.
