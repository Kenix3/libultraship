---
title: Epoch 2 Migration Plan
nav_order: 8
---

# Epoch 2 Migration Plan

This document describes the migration path from the `port-maintenance` line to **libultraship epoch 2** (`2.x.x.x`).

It is intended for maintainers upgrading an existing port to the second epoch architecture.

## Scope

This plan focuses on migration areas with the highest integration impact:

- Context creation and component lifecycle
- Bridge cache/singleton holder behavior
- Resource identifier/init-data changes
- Event system migration points
- Window/GUI/input surface changes
- Build/versioning updates

---

## 1) Build and versioning baseline

Update build/release assumptions to 4-part epoch semver:

- `LUS_VERSION_EPOCH`
- `LUS_VERSION_MAJOR`
- `LUS_VERSION_MINOR`
- `LUS_VERSION_PATCH`

Review optional build toggles introduced in this line:

- `INCLUDE_PROFILING`
- `COMPONENT_THREAD_SAFE`
- `LUS_BUILD_TESTS`

---

## 2) Context creation model (major change)

Epoch 2 centers initialization around `ship/core` components.

Ports should choose one of two setup styles:

### A) `CreateDefaultInstance(...)` helper-driven setup

Use when you want LUS to create a standard component graph.

**Pros**
- Fastest path to a running baseline.
- Standard ordering handled internally.

**Cons**
- Less control over concrete component construction.
- Harder to customize subsystem composition early.

### B) `CreateInstance(...)` + manual component graph (recommended for most ports)

Use when you want explicit control of startup and dependencies.

**Pros**
- Full ownership of component add-order and init flow.
- Easier to inject custom implementations.
- Better for long-term maintainability in complex ports.

**Cons**
- More boilerplate.
- Dependency sequencing must be enforced by the port.

### Practical migration note

Prefer explicit graph assembly when the port has custom window/input/resource/event behavior.

---

## 3) Bridge architecture update: singleton component holders

Epoch 2 bridge APIs use cached singleton-like holders for multiple components.

### What this means

Bridge calls now depend on those cached pointers being current.

### Required lifecycle steps

- After context/component assembly: populate bridge caches.
- After replacing any subsystem at runtime: refresh bridge caches.
- During shutdown: clear bridge caches.

If caches are stale, runtime faults are likely.

---

## 4) Resource system migration

`ResourceIdentifier` is now central to resource identity and scoping.

### Required updates

- Migrate resource load paths toward identifier-aware APIs.
- Update custom factories/importers that relied on older `ResourceInitData` field assumptions.
- Revalidate cache/owner/archive scoping behavior in custom loaders.

---

## 5) Event system migration

Prefer direct use of `Events` APIs.

Treat `EventSystem` naming as compatibility surface where still present.

### Required check

Revalidate listener priority behavior and ordering assumptions during migration.

---

## 6) Window/GUI/input migration

Key API and lifecycle changes to verify:

- File drop manager surface moved to `FileDrop` component.
- GUI elements follow component lifecycle patterns (`OnInit` flow).
- Constructor signatures and dependency injection changed in several window/gui/input classes.

---

## 7) Validation checklist

Before finalizing migration:

1. Build all supported targets.
2. Verify startup and component initialization order.
3. Validate archive mounting and resource load behavior.
4. Validate controller/keyboard/mouse flows.
5. Validate event dispatch ordering.
6. Validate bridge-backed APIs with cache refresh paths.
7. Validate clean shutdown (including bridge cache clear behavior).

---

## Suggested execution order

Use small, reviewable migration commits:

1. Compile-surface changes (includes/types/signatures).
2. Context creation and lifecycle conversion.
3. Bridge cache wiring and refresh/clear hooks.
4. Runtime validation fixes.
5. Cleanup and dead-code removal.
