# Latte Dock NG — Universal AI Instructions & Knowledge Base

This is the repository's single source of truth for AI coding rules, workflows,
architecture notes and diagnosis memory. It is tool-neutral and intended for
pi, Claude Code, OpenAI Codex, Cursor and every other AGENTS.md-aware assistant.
Keep project knowledge here; do not create tool-specific `CLAUDE.md` or
`CODEX.md` files.

Shared testing/release procedures live in `docs/`: `development-testing-guide.md`
documents the autotest suite and the Runtime Retest Workflow (clean-quit and
coredump A/B verification after runtime fixes).

The rules below are always in effect. Consult the later knowledge-base sections
when working on releases, compatibility problems or known runtime behavior.

## User Rules (always apply)

1. **No auto-commit / no auto-push** — Never commit or push git changes
   without explicit user approval. Commit and push are two SEPARATE approvals:
   after committing, ask "push?". "commit" alone never implies "push".
   Only use git read operations (diff, log, status) unless explicitly asked.
2. **English only** — All codebase content in English: commit messages,
   release notes/GitHub descriptions, code comments, documentation.
3. **No AI attribution** — Commit messages must NOT include `Co-Authored-By`,
   `Signed-off-by`, or similar AI attribution lines.
4. **Zero warnings on GCC and Clang** — Every build (debug, development,
   pre-commit, release) must compile with zero warnings and zero errors on
   BOTH compilers. Warning flags come from KDE's KDECompilerSettings module;
   when adding any warning suppression, document why.
5. **No regressions when removing dead code** — Trace all consumers before
   removal; keep working features intact even if they share code; verify
   compilation AND runtime behavior afterwards; check the debug log for new
   errors/warnings.
6. **Release requires autotest** — Before every release run
   `cd build && ctest --output-on-failure` (41 registered autotest targets
   incl. 170+ source-contract checks on GCC and Clang; fragile areas: digital
   clock, systray, volume, appmenu, clipboard, separator/spacer, middle-click
   close, auto-pin on drag, scroll minimize).
7. **Document non-obvious design constraints** — Add concise English code
   comments for runtime workarounds and subtle ownership, lifecycle, timing or
   cross-component state logic. Explain why the code exists, identify the
   authoritative state source and record the failure mode that would return if
   the constraint were removed. This context is required for both human and AI
   maintainers; do not narrate self-evident code.

## Development Debug & Retest Workflow

When testing changes to latte-dock-ng, follow this exact workflow:

1. **User-mode install modified code**
   ```bash
   cd /data/projects/latte-dock-ng && bash install.sh --user Debug >/tmp/latte-install-user.log 2>&1; tail -n 80 /tmp/latte-install-user.log
   ```

2. **Kill old latte-dock-ng process** (CAUTION: never use `pkill -f` in a
   command line that also contains "latte-dock-ng" elsewhere — it matches the
   shell's own command line and kills the shell; prefer `pkill -x latte-dock-ng`)
   ```bash
   pkill -x latte-dock-ng || true
   ```

3. **Remove old log file**
   ```bash
   rm -f /tmp/latte-ng.log
   ```

4. **Source user-mode environment variables**
   ```bash
   source ~/.config/latte-dock-ng/dev-env.sh
   ```

5. **Launch the USER-MODE Debug binary (~/.local/bin), NOT the system
   /usr/bin binary** (user explicitly corrected this; the dev-env.sh sourcing
   enables locally-built QML module overrides). Must survive shell timeout —
   use script + nohup in a separate session. The explicit `setsid` prevents
   command-runner session cleanup from terminating an otherwise nohup-protected
   process:
   ```bash
   cat > /tmp/launch-latte.sh << 'SCRIPT'
   #!/bin/bash
   source ~/.config/latte-dock-ng/dev-env.sh
   exec ~/.local/bin/latte-dock-ng --replace --debug > /tmp/latte-ng.log 2>&1
   SCRIPT
   chmod +x /tmp/launch-latte.sh
   setsid -f nohup /tmp/launch-latte.sh > /dev/null 2>&1
   sleep 5
   ps aux | grep latte-dock-ng | grep -v grep || echo "DOCK FAILED TO START"
   ```

   Note: `bash install.sh --user Debug` already installs all plasmoid QML to
   ~/.local/share/plasma/plasmoids/ — a manual `cp` overlay of QML files is
   usually not needed. (If DESTDIR causes prefix duplication like
   ~/.local/home/user/.local/..., install directly with
   `cmake --install build --prefix ~/.local`.)

6. **Wait for user retest feedback**, then automatically analyze debug log for warnings/errors

7. **Analyze debug log** (`/tmp/latte-ng.log`) for warning/error entries. If found, record them as issues that need fixing.

8. **Verify clean-quit scenarios (logout/shutdown/restart fixes)** — follow
   the Runtime Retest Workflow in `docs/development-testing-guide.md`: record a
   `coredumpctl` baseline, drive the quit scenario, then confirm no new
   latte-dock-ng core appears, the log shows the expected teardown markers and
   no Fatal/ASSERT, and run an A/B check against the pre-fix binary for crash
   fixes.

9. **Do NOT commit or push** unless the user explicitly confirms. No auto-commit/push allowed without user permission.

## Quick references

- **Debug logging**: latte discards ALL output unless launched with `-d`
  (`--debug`). In minimal or VM test environments, also pass
  `--log-file /tmp/latte-ng.log` plus `QT_LOGGING_RULES='latte*=true'` for
  latte's own qCDebug.
- **Runtime retest & clean-quit/coredump verification**: see
  `docs/development-testing-guide.md` (Runtime Retest Workflow) — canonical
  steps for crash fixes are kept there, not duplicated here.
- **GitHub proxy**: if git push/ls-remote hangs, retry through the local HTTP
  proxy configured in the shell environment (machine-local; exact address is
  not committed to the repo).
- **Wayland popup positioning (hard rule)**: never position a
  `LatteCore.Dialog` / plasma popup with a raw `QWindow::setPosition()`. For a
  Plasma shell surface the compositor ignores it and keeps the position the
  popup was first mapped at, while `x()` still reports the requested value.
  Always route through `PlasmaQuick::Dialog::adjustGeometry()`
  (`declarativeimports/core/dialog.cpp`). A stale `x()` in a diagnostic means
  the compositor ignored the request — not that the position math is wrong.

## Release Workflow (only on explicit request)

### latte-dock-ng repository

1. Bump `set(VERSION X.Y.Z)` in `CMakeLists.txt` and `version = "X.Y.Z"` in
   `default.nix`.
2. Run `nix flake check --print-build-logs`,
   `nix build .#default --no-link --print-build-logs`, and the required GCC and
   Clang autotests.
3. Commit `release: bump version to X.Y.Z`, including the pending changes and
   `CHANGELOG.md` section.
4. Create an annotated `vX.Y.Z` tag, then push the commit and tag only with
   separate explicit user authorization.
5. After CI creates the artifacts, curate English release notes with a
   `compare/vPREV...vX.Y.Z` changelog link. Debian trixie packages use the
   `+deb13u1` revision marker; testing/sid uses plain `-1`.

### Gentoo overlay

- Work in the local checkout of `ruizhi-lab/gentoo-overlay`, branch `main`, at
  `kde-misc/latte-dock-ng/`.
- Copy the previous ebuild, ensure `SRC_URI` uses `v${PV}`, remove the obsolete
  ebuild, and regenerate the Manifest with a temporary writable `DISTDIR`.
- Generate the Manifest only after the release tag is final. Moving a tag
  changes GitHub tarballs; delete the stale Manifest and regenerate it or
  emerge will report a filesize mismatch. Never use sudo for this workflow.

## Architecture & Compatibility Notes

- The application is one large executable assembled by `app/CMakeLists.txt`.
  Large runtime sources include `layoutmanager.cpp`, `containmentinterface.cpp`,
  `view.cpp`, `storage.cpp`, and `AppletItem.qml`.
- Use `-j8` for project builds unless a command has a specific resource limit.
- User configuration normally disables window previews and retains only title
  tooltips; treat preview rendering as inactive unless explicitly enabled for a
  regression test.
- The application icon is `latte-dock-ng`; never fall back to the legacy
  `latte-dock` name because third-party themes may supply old artwork for it.
- Debian Plasma 6.3 lacks a filesystem `org.kde.plasma.plasmoid` QML module.
  Register it lazily; an attached-type stub breaks Qt 6.8 Behavior resolution.
- On minimal Fedora Wayland systems, use `--log-file` and
  `QT_LOGGING_RULES='latte*=true'` to capture Latte's own logs.
- Keep detailed component invariants next to their implementation. In
  particular, `TaskItem.qml` documents the task-tooltip ownership, hover and
  enable-state contract; its source-contract autotests protect that design.
- Treat `AGENTS.md` as the high-level index and policy document. Put localized
  failure modes and workaround rationale in English code comments where future
  maintainers and AI tools will encounter the relevant logic.
