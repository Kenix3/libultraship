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

With standard semantic versioning, a breaking change forces a major version bump (for example, `1.x.x` to `2.0.0`). This creates a dilemma:

- **Option A**: Keep breaking changes on `main`, making it unstable for months while the refactor lands incrementally.
  - **Result**: Port maintainers cannot safely pull fixes or features during the refactor period.

- **Option B**: Hold all refactor work on a long-lived feature branch.
  - **Result**: The branch diverges significantly, merge conflicts accumulate, and collaboration becomes difficult.

- **Option C**: Release frequent major versions (`1.0` to `2.0` to `3.0` in quick succession).
  - **Result**: Version numbers become less meaningful, and users face constant migration churn.

### How epochs solve this

Epochs add a leading number to the version: `epoch.major.minor.patch` (for example, `1.2.3.4` or `2.0.0.0`).

- **During a major refactor**: `main` moves to a new epoch (for example, `2.x.x.x`), where breaking changes land incrementally.
- **The previous epoch** (for example, `1.x.x.x`) continues as a maintained branch:
  - Critical fixes backported
  - Selected features may be cherry-picked
  - Stable baseline for ports not yet ready to migrate

This approach:
- Keeps `main` moving forward without long-lived feature branches
- Provides a stable baseline for ports during transition periods
- Avoids rapid major version churn
- Clearly signals architectural dividing lines

### When to expect a new epoch

A new epoch typically begins when:
- Core architectural patterns change (for example, context/component model redesign)
- Multiple subsystems require coordinated breaking changes
- The refactor is expected to span several months of incremental work

Minor breaking changes within an epoch still follow semantic versioning rules (bump `major` within that epoch).
