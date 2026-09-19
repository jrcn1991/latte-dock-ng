# Architecture and Tooling Modernization Plan

Status: proposed; implementation has not started.
Assessment date: 2026-09-19. Baseline revision: `78813c598`.

## Purpose and authority

Make existing quality rules enforceable and reduce the cost of changing runtime
behavior. Preserve C++20, Qt 6, KDE Frameworks 6, Plasma 6, and Wayland. This is
an incremental maintenance plan, not a rewrite or a release request.

Read [AGENTS.md](../AGENTS.md) first. Use the companion
[implementation guide](architecture-modernization-implementation.md) for each
work package. Runtime verification remains defined by the
[development testing guide](development-testing-guide.md).

## Verified baseline and limits

The assessment inspected source and configuration; it did not rebuild the
application, execute the full test suite, or establish current CI health.

| Area | Observed state | Consequence |
| --- | --- | --- |
| Language and dependencies | C++20, Qt >= 6.6, KF6 >= 6.0, Plasma >= 6.3; CMake >= 3.20 | Keep these compatibility floors unless a later decision explicitly changes them. |
| Application structure | `app/CMakeLists.txt` assembles most application sources into one executable; child directories append sources through `PARENT_SCOPE` | Directory boundaries do not enforce internal dependencies. |
| Runtime size | `layoutmanager.cpp`: 2,872 lines; `containmentinterface.cpp`: 2,536; `view.cpp`: 2,127; `AppletItem.qml`: 1,975; `storage.cpp`: 1,911 | These are investigation candidates, not mandatory line-count reduction targets. |
| Isolation | Preview helper uses an independent process, watchdog, serial validation, bounded protocol and fallback | Preserve this fault boundary and the authoritative hover-action setting. |
| CI | GCC/Clang builds and tests, QML lint, multi-distribution source/package installation checks | Extend existing workflows rather than replacing them. |
| Compiler warnings | Project explicitly adds `-Werror=format-security`; no comprehensive project warning-as-error gate was found in the reviewed CI | Reconfirm effective ECM/compiler flags, then enforce the zero-warning rule. |
| QML lint | Deep lint allows legacy diagnostics; promoted categories are empty | Introduce a measured baseline and prevent regressions. |
| QML backlog | Script references `docs/qmllint-backlog-plan.md`, absent at the baseline | Supply a scoped plan before claiming warning governance exists. |
| Tests | Unit, plugin, source-contract and isolated D-Bus tests exist; coverage estimate is file-level | Do not describe source contracts or file estimates as runtime/branch coverage. |
| Local build state | Existing GCC and Clang directories listed 41 and 42 tests respectively | Regenerate matching configurations; this observation does not establish a source defect. |
| Developer tools | clang-format and clangd configured; no tracked CMake Presets or clang-tidy configuration found | Establish repeatable configurations and introduce targeted analysis. |
| Nix | Flake has package/overlay/module outputs but no explicit `checks` or `devShells` outputs | A successful flake check alone does not prove application tests ran. |

The deep lint script's historical “7k+” comment is not a measured current count.
All counts and tool behavior must be remeasured at implementation time.

## Architectural direction

Keep the application as a cohesive desktop program with focused internal
components. Do not introduce services or public shared-library APIs merely to
make the directory layout appear modular.

- Application composition: `Corona` and `View` coordinate lifecycle and wire
  components together; avoid adding more unrelated policy to them.
- Testable policy: geometry, visibility decisions and storage transformations
  should accept explicit inputs and expose deterministic results where feasible.
- Platform adapters: Plasma, KWin, Wayland and compatibility behavior stay behind
  focused interfaces. Preserve version-specific fallbacks until consumers and
  supported distributions are verified.
- Presentation: QML owns visual composition; give it explicit, typed state
  instead of expanding implicit context and cross-component object discovery.
- Ownership: retain Qt parent ownership and `QPointer` where appropriate.
  Document the state authority, destruction order and stale-callback behavior;
  do not mechanically replace QObject pointers with smart pointers.

## Work packages and dependencies

Each package should be a reviewable change, with smaller slices when necessary.
Suggested commit subjects do not authorize commits or pushes.

| ID | Priority | Depends on | Deliverable | Suggested commit subject |
| --- | --- | --- | --- | --- |
| M0 | First | None | Reproducible baseline and evidence ledger | `docs: record modernization baseline` |
| M1 | High | M0 | Shared compiler/build/test presets | `build: add reproducible development presets` |
| M2 | High | M1 | Verified compiler warning gate | `ci: enforce compiler warning policy` |
| M3 | High | M0 | QML diagnostic baseline and regression gate | `ci: prevent new QML diagnostics` |
| M4 | Medium | M1, M2 | Focused static analysis and sanitizer validation | `test: add lifecycle analysis checks` |
| M5 | High | M0, M1 | Behavioral tests for one selected runtime boundary | `test: cover selected runtime behavior` |
| M6 | Medium | M2, M4, M5 | One tested internal component extraction | `refactor: isolate selected runtime policy` |
| M7 | Medium | M1 | Explicit Nix development/test contract | `build: define Nix development checks` |

M3 and M7 need not wait for the architecture extraction. Within M6, first add
characterization tests, then extract production logic, then simplify callers.
Do not combine unrelated warning cleanup, behavior changes and refactors.

## Completion criteria

- Matching GCC and Clang configurations build the application and all tests
  with zero compiler warnings/errors; CI demonstrably rejects an injected
  project warning in a disposable validation fixture.
- QML syntax and generated-module checks remain active. New unapproved
  diagnostics fail comparison against a reviewed baseline; no global warning
  suppression hides the backlog.
- Developers and CI use documented equivalent configurations, with separate
  build trees and explicit compiler selection.
- At least one high-risk behavior is tested through production code, with the
  relevant desktop retest recorded. Tests must not reproduce the production
  algorithm in the test body.
- At least one internal boundary has explicit dependencies and ownership;
  callers and tests use the same extracted production implementation.
- Nix checks state exactly whether they build, test, or verify installation.
- Each package has evidence, limitations and a handoff record. A package with
  required desktop/CI verification outstanding remains pending validation.

## Non-goals and safeguards

No language migration, whole-tree formatting, blanket QML import conversion,
public ABI redesign, minimum-version bump, feature removal, version bump,
release tag, or automatic commit/push is included. Do not weaken existing
regression contracts just to make a refactor pass. Treat build infrastructure
and compatibility changes as potentially affecting every supported package.

## Progress ledger

Update this table only with evidence from the implementation revision.

| Package | Status | Revision/evidence | Remaining work |
| --- | --- | --- | --- |
| M0 | Not started | Assessment only | Fresh measurements |
| M1 | Not started | None | Presets |
| M2 | Not started | None | Warning gate |
| M3 | Not started | None | QML baseline and gate |
| M4 | Not started | None | Analysis and sanitizers |
| M5 | Not started | None | Behavioral coverage |
| M6 | Not started | None | Component extraction |
| M7 | Not started | None | Nix checks |
