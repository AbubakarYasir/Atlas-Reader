# Platform Roadmap

Atlas is **Windows-first**, not Windows-only.

The order is fixed unless explicitly revised:

1. Windows
2. Android
3. Linux
4. macOS
5. iOS/iPadOS

## Shared versus platform code

Shared:

- domain models/rules;
- document identity/capability;
- library/index logic;
- bookmark overlay/reconciliation;
- search/query model;
- PDF facade contracts;
- most application services;
- most QML components and design tokens.

Platform/adapted:

- storage/document references;
- watchers;
- file pickers;
- open-with/share intents;
- printing;
- lifecycle/power;
- accessibility platform qualification;
- stylus special handling;
- packaging/installer;
- secure credential integration if later needed.

## Windows

Primary implementation and release quality first.

Expected stack:

- MSVC 2022;
- Qt Quick scene graph using native Windows graphics backend;
- Win32/Windows APIs behind adapters where Qt is insufficient;
- Windows printing/file association/shell integration;
- pen/touch qualification;
- SSD/HDD/removable drive behavior.

Do not leak Win32 handles/paths into the portable domain.

## Android

Android is the first proof that the architecture was genuinely portable.

Key adaptations:

- Storage Access Framework/content URIs;
- scoped storage;
- activity/lifecycle state;
- share/open-with intents;
- touch/stylus interaction density;
- mobile navigation shell;
- background restrictions.

A Windows filesystem path cannot be the canonical document identity model.

## Linux

Desktop UI can reuse much of Windows shell, but qualification must include:

- Wayland and relevant X11 fallback;
- portals/file pickers;
- filesystem watcher differences;
- printing stack;
- distribution packaging;
- font/RTL consistency.

## macOS

Adapt:

- Finder/open-with;
- native menu conventions;
- sandbox/security-scoped access if distribution requires it;
- printing;
- Metal-backed rendering qualification;
- trackpad/gesture behavior.

## iOS/iPadOS

Adapt:

- document picker/security-scoped resources;
- app lifecycle/memory pressure;
- sharing;
- Pencil input;
- mobile/tablet responsive shell;
- platform distribution constraints.

## Cross-platform acceptance rule

A new platform may implement a different adapter/UI arrangement, but it must not silently change core rules for document security, bookmark storage state, identity, conflicts, or backup/recovery.

Platform parity is defined by user capability/contract, not identical widgets.