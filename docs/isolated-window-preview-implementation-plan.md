# Isolated Window Preview Implementation Plan

## Objective

Move live window preview rendering out of the Latte Dock process so that PipeWire, screencasting, popup creation, and preview rendering cannot block the dock.

The feature must fail closed: if the isolated preview process is slow, unavailable, malformed, or otherwise unsafe, the preview must be hidden and the normal title tooltip must remain available.

Live previews remain disabled by the default `None` hover action. Users enable
the isolated preview path explicitly through the task hover-action setting.

## Scope and constraints

- Do not commit, push, or merge without explicit user approval.
- All source comments and documentation must be in English.
- Preserve existing task, grouping, activation, and tooltip behavior.
- Do not use blocking PipeWire, DBus, screenshot, or window-management calls in the dock process.
- Do not use `QWindow::setPosition()` for Plasma shell popups on Wayland. Use `PlasmaQuick::Dialog::adjustGeometry()`.
- GCC and Clang builds must complete without warnings or errors.
- Existing user changes in the worktree must be preserved.

## Proposed architecture

```
latte-dock-ng
    |
    | QProcess, bounded JSON-lines protocol
    v
latte-dock-ng-preview
    |
    | PlasmaQuick dialog and asynchronous QML capture items
    v
PipeWire / Plasma screencasting
```

The dock process owns task state and remains authoritative for window identity and activation. The preview process owns preview UI, capture objects, layout, and preview interaction.

Candidate implementation files:

- `plasmoid/plugin/previewprocess.cpp` and `.h`: dock-side process manager and protocol client.
- `plasmoid/preview/main.cpp`: isolated preview process entry point and protocol server.
- `plasmoid/preview/Preview.qml`: preview layout and interaction.
- `plasmoid/preview/Capture.qml`: asynchronous PipeWire capture item.
- `plasmoid/package/contents/ui/main.qml`: feature gating and lifecycle wiring.
- `plasmoid/package/contents/ui/task/TaskItem.qml`: delayed preview requests and fallback tooltip handling.
- `plasmoid/package/contents/ui/task/SubWindows.qml`: serializable window metadata and activation routing.

## Implementation phases

### 1. Audit the existing worktree

Run:

```bash
git status --short
git diff --check
git diff --stat
```

Verify that the old in-process preview path cannot be reached when isolated previews are disabled. Do not reset, clean, or overwrite unrelated worktree changes.

### 2. Define the protocol

Use one JSON object per line. Never send image data through the protocol.

Supported messages should include `show`, `hide`, `heartbeat`, `activate`, and `closed`.

Protocol requirements:

- Bound the maximum message size.
- Validate message type, generation, UUID, and numeric fields.
- Ignore stale generations.
- Reject malformed input without terminating the dock.
- Validate activation requests against the current task model before activating a window.

### 3. Implement process lifecycle and fail-closed behavior

The dock-side manager should start the helper lazily after the hover delay, keep all process I/O asynchronous, enforce startup and heartbeat timeouts of approximately 2.5 seconds, detect helper exit, avoid restart loops, and cancel stale requests.

The preview process should show a lightweight placeholder before capture becomes ready, send periodic heartbeats, exit after a bounded lease without valid requests, and close cleanly when the dock exits.

Failure sequence:

```
failure or timeout -> hide preview -> restore title tooltip -> no retry for this hover
```

### 4. Keep capture asynchronous

The helper must never wait synchronously for PipeWire or screencasting authorization. Each capture should load independently, and failure of one window must not take down the preview dialog.

Recommended behavior:

- Delay preview creation by roughly 150–250 ms.
- Limit simultaneous previews, for example to nine.
- Load capture components asynchronously.
- Keep placeholder cards for unavailable or minimized windows.
- Destroy capture items when the preview is hidden.

### 5. Preserve Wayland popup positioning

The helper must position the Plasma popup through `PlasmaQuick::Dialog::adjustGeometry()`.

Test all four dock edges, auto-hide mode, multiple monitors, and rapid movement between adjacent tasks. A stale `x()` value on Wayland is not evidence that the compositor accepted the requested position.

### 6. Prevent polish loops

When `IconItem` updates an SVG pixmap, synchronous repaint signals can cause a nested polish request. The resize and device-pixel-ratio update must remain protected from re-entrant signals with a narrowly scoped signal blocker.

## Build and test plan

Build both the application and helper with both compilers:

```bash
cmake --build build-gcc --target latte-dock-ng -j8
cmake --build build-gcc --target latte-dock-ng-preview -j8
cmake --build build-clang --target latte-dock-ng -j8
cmake --build build-clang --target latte-dock-ng-preview -j8
```

Run focused tests first:

```bash
ctest --test-dir build-gcc --output-on-failure -R 'sourcecontract|declarativecore'
ctest --test-dir build-clang --output-on-failure -R 'sourcecontract|declarativecore'
```

Then run the complete suites:

```bash
ctest --test-dir build-gcc --output-on-failure
ctest --test-dir build-clang --output-on-failure
```

A DBus-only failure caused by the test sandbox must be reproduced in an unrestricted test session and documented.

## Runtime verification

Install and launch the user-mode Debug build using
`docs/development-testing-guide.md`. Verify the default `None` hover action
keeps previews off, then select Preview Windows in the Dock task hover-action
setting and exercise the isolated helper without any environment override.

Test single-window tasks, grouped tasks, minimized windows, rapid hover transitions, window close while visible, PipeWire authorization failure, helper startup timeout, manual helper termination, and Dock shutdown while the helper is visible.

Inspect `/tmp/latte-ng.log` for warning, error, fatal, assert, polish, dialog, pipewire, and screencast entries.

## Acceptance criteria

The feature may be enabled only if:

- The dock never freezes while opening, updating, or closing a preview.
- The dock process never waits for capture or screencasting.
- Preview startup may be delayed, but dock input remains responsive.
- Capture failure closes the preview and restores the title tooltip.
- No polish loop or repeated empty-dialog warnings occur.
- Helper crashes and exits do not crash or disable the dock.
- CPU and memory usage settle after repeated hover transitions.
- GCC and Clang builds are warning-free.
- Focused and full tests pass, apart from separately documented environment-only failures.

If any criterion fails, keep `showPreviews` disabled and do not merge the PR.

## Handoff checklist

Before asking whether the PR is mergeable, report:

1. Changed files and the process boundary.
2. GCC and Clang build results.
3. Focused and full test results.
4. Normal dock runtime result.
5. Isolated preview runtime and performance result.
6. Remaining warnings, DBus limitations, or PipeWire failures.
7. Whether the default `None` hover action keeps previews disabled.

## Implementation status

Implemented and tracked on the experimental `preview` branch. Runtime findings
and their root causes are recorded in
`docs/isolated-window-preview-debug-notes.md`; read that file before changing
the helper surface or the protocol. The default hover action keeps previews
disabled; selecting a preview hover action enables the isolated helper.
