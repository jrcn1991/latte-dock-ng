# Architecture Modernization Implementation Guide

Status: execution instructions for proposed work, not an implementation report.
Read the [plan](architecture-modernization-plan.md), [AGENTS.md](../AGENTS.md)
and [testing guide](development-testing-guide.md) before making changes.

## Start or resume a package

1. Read the progress ledger and the previous handoff. Inspect the working tree;
   preserve unrelated edits and do not assume local build artifacts are current.
2. Select the first authorized incomplete package whose dependencies are met.
   A request to implement one package does not authorize all later packages.
3. If `.codegraph/` exists, use CodeGraph before locating or understanding code.
   Trace consumers before moving sources or changing interfaces. Read raw files
   for configuration, documentation or details the index did not return.
4. Record the current revision, tool versions, selected files and expected
   behavior. Recheck observations from the original assessment.
5. Implement one cohesive slice, run its acceptance checks, and update the
   ledger and handoff. Follow the branch policy in AGENTS.md: obtain separate
   commit and push approvals on `main`; on another branch, the requested and
   validated implementation may be committed and pushed automatically. Do not
   silently switch branches or rewrite published history.

## M0: Establish the baseline

Inspect `CMakeLists.txt`, `autotests/CMakeLists.txt`, `.github/workflows/build.yml`,
`scripts/qmllint*.sh`, `default.nix`, `flake.nix` and the testing guide.

- Record GCC, Clang, CMake, ECM, Qt, Plasma and qmllint versions.
- Configure fresh, separately named GCC and Clang build directories. Explicitly
  select both C and C++ compilers on first configure. Never switch compilers in
  an existing cache or delete unrelated build trees.
- Build the application and `latte-autotests`, then run CTest. Capture full logs
  with command exit codes; use `pipefail` when piping through `tee`.
- Compare registered test names with `ctest --show-only=json-v1`. Explain
  differences using configuration and optional dependencies, not historical
  hard-coded counts. Reject missing executables and unexpected skipped tests.
- Run syntax and deep QML lint against the fresh modules. Record diagnostics by
  category, file and tool version, including the import environment.
- Inspect effective compiler commands, not just top-level CMake settings, to
  distinguish inherited ECM flags from project policy.

Acceptance: a baseline record in this plan's ledger or a linked English report,
with successful checks and pre-existing failures distinguished. Stop dependent
refactoring when unexplained baseline failures prevent regression comparison.

## M1: Reproducible build configurations

Primary files: new root `CMakePresets.json`, `.gitignore`, `.clangd`, the testing
guide and `.github/workflows/build.yml`.

- Use a preset schema supported by the retained CMake 3.20 minimum. Do not copy
  newer workflow-preset syntax without an explicit compatibility decision.
- Define separate GCC/Clang Debug and Release configurations, explicit compiler
  paths/names, `BUILD_TESTING=ON`, and compilation database export.
- Give every configuration a separate binary directory. Configure build presets
  with eight jobs and explicit application/test targets; a normal build does
  not build the `EXCLUDE_FROM_ALL` autotest targets.
- Provide test presets with failure output. Keep developer-specific paths in
  ignored `CMakeUserPresets.json`; add no machine-specific absolute paths.
- Resolve clangd's current fixed `build` database path through a documented
  default preset or local selection. Avoid maintaining competing databases.
- Keep `install.sh --user Debug` working and document how it relates to the
  presets. Do not silently replace the established runtime install workflow.

Acceptance: list, configure, build and test the presets with the supported CMake
version and both compilers. Verify CI and local settings agree. If old CI used
`nproc`, update the touched project build steps to the documented eight-job rule.

## M2: Compiler warning enforcement

Primary files: root CMake configuration, presets and build workflow.

- Preserve KDECompilerSettings as the warning-policy source. Add a deliberate
  strict-build option for first-party targets and enable it in CI/development
  configurations; retain compatibility for downstream packaging environments.
- `CMAKE_COMPILE_WARNING_AS_ERROR` requires CMake 3.24. With the current floor,
  use an appropriate compiler-specific fallback or version-guarded property.
  Ensure options reach targets created earlier, including logging and plugins.
- Verify both Debug and Release: warnings may depend on optimization. Include
  the preview helper, other executables, plugins and test targets.
- Fix warnings at their cause. Any narrowly scoped suppression must explain the
  upstream/compiler issue and removal condition. Do not globally disable useful
  diagnostics or silently exclude project targets.
- Retain full CI logs on failure. Configure/link/QML diagnostics are separate
  from compiler warnings and must not be mislabeled as covered by `-Werror`.

Acceptance: clean application/test builds on GCC and Clang, plus a disposable
warning fixture proving both gates fail. Remove the fixture before final diff
review; check all compile targets receive the intended policy.

## M3: QML warning governance

Primary files: `scripts/qmllint.sh`, `scripts/qmllint-deep.sh`, build workflow;
new `docs/qmllint-backlog-plan.md` and a machine-readable diagnostic baseline.

- Create the referenced backlog document with measured categories, priorities,
  environment, exemptions and evidence. Do not copy “7k+” as a current count.
- Preserve syntax errors, missing generated modules and failed project imports
  as unconditional failures. System-installed modules must not mask missing
  build outputs, including generated type metadata required by the lint path.
- Compare diagnostics in a controlled Qt/qmllint/import environment. Pin or
  explicitly version the lint environment; do not compare different tool
  releases as though their diagnostics were interchangeable.
- Prefer diagnostic identities using repository-relative file, category and
  normalized message, retaining duplicate counts. Ignore unstable absolute
  paths and line shifts. Category totals alone can hide a new warning when an
  unrelated warning is removed.
- Baseline updates must show additions/removals for review, never automatically
  bless current output. Explain specific unavoidable dynamic Plasma imports;
  do not exempt whole files or all missing-property warnings.
- Clean one category at a time. After two consecutive comparable zero-warning
  measurements with green CI, promote it through
  `PROMOTED_ERROR_CATEGORIES`. Keep the script and backlog document in sync.
- Qualify QML access and declare types only after checking bindings, implicit
  context, plugin registration and supported Plasma versions.

Acceptance: fixture checks prove added diagnostics fail, removals pass, path/line
shifts do not create false regressions, and malformed/incomplete lint output
fails closed. Verify syntax/module failures still fail and retain raw logs.

## M4: Static analysis and sanitizer validation

Primary files: CMake/presets, build workflow, scoped analysis configuration and
the testing guide. Introduce analysis and sanitizers in separate slices.

- Start clang-tidy or Qt-aware clazy with a small documented set of high-value
  checks on changed first-party code. Use the matching compilation database and
  exclude generated sources without hiding project diagnostics.
- Add an opt-in ASan/UBSan configuration with matching compile/link options on
  participating executables and libraries. Never ship instrumented binaries.
- Start with existing offscreen/unit tests; extend to lifecycle integration
  tests once the environment is reliable. Confirm child helper processes and
  dynamically loaded project plugins use compatible instrumentation.
- Record each third-party suppression with a reproduction and scope. Do not
  suppress all leaks or all Qt frames to obtain a green result.

Acceptance: a disposable fixture demonstrates that the enabled analysis and
sanitizer checks detect their intended failures; targeted production tests pass
without unexplained findings. Document anything the selected tools do not cover.

## M5: Test a runtime boundary through production code

Select one risk based on M0 evidence: popup positioning/focus, tooltip ownership,
layout transition, or clean teardown. Inspect existing autotests first.

- Record observable behavior and failure reproduction before adding tests.
- Prefer a real `QQmlComponent`, production helper, or isolated D-Bus service
  over source-text assertions where practical. Keep valuable existing source
  contracts until replacement coverage proves equivalent protection.
- Test normal, stale/invalid input, fallback and teardown paths relevant to the
  selected boundary. Use event-driven bounded waits, not arbitrary long sleeps.
- Keep tests away from the real session bus and user configuration. Full KWin
  scenarios belong in an isolated desktop/VM or an explicitly coordinated user
  retest, not an uncontrolled CI interaction with the active desktop.
- For GUI/runtime modifications follow the canonical user-mode Debug install,
  detached launch, user feedback and log review procedure. Crash fixes also need
  coredump baseline and pre-fix/fixed A/B evidence.

Acceptance: the behavior test fails for the reproduced defect or a controlled
fault and passes with production behavior restored. Record real desktop retest
results separately from offscreen results. Mark unavailable retests pending.

## M6: Extract one internal component

Candidate areas: `app/view/`, `app/layouts/`, `containment/plugin/`. Select the
smallest useful boundary supported by M5, not the largest file by line count.

1. Trace callers, QML property consumers, signals, configuration keys and
   ownership. Write down inputs, outputs, state authority and lifetime.
2. Add characterization tests around the behavior to preserve.
3. Extract a deterministic calculation or focused state component. Keep Qt,
   Plasma and UI adapters only where needed; avoid a new generic framework.
4. Create a private static/object CMake target where it enforces a useful
   dependency boundary. Use target-scoped include paths and dependencies.
5. Link application and tests to the same implementation. Remove duplicate
   compilation of the extracted sources; check PIC for shared-plugin consumers,
   AUTOMOC, generated headers, logging and exported include requirements.
6. Preserve public QML properties, registration names, D-Bus behavior, storage
   format, plugin paths and compatibility fallbacks. Move subtle rationale with
   the code and explain any new ownership constraint next to implementation.

Acceptance: GCC/Clang application and tests pass; touched QML has no diagnostic
regression; applicable runtime and distribution installation checks pass.
Document the actual dependency reduction, not merely the line-count change.
Do not split another component until this slice has completed validation.

## M7: Nix development and checks

Primary files: `flake.nix`, `default.nix` and testing documentation.

- Add a development shell consistent with the package's dependencies and tools.
- Define an explicit check derivation that enables testing, builds
  `latte-autotests`, and executes CTest in the sandbox with isolated D-Bus and
  offscreen settings where appropriate. Avoid an accidental dependency cycle
  or changing release package contents just to add test tools.
- Preserve supported systems and the lock file unless a dependency update is
  explicitly in scope. Separate any lock update from architecture changes.
- Document which runtime scenarios still require a Plasma session.

Acceptance: run `nix flake check --print-build-logs` and
`nix build .#default --no-link --print-build-logs`; verify test execution appears
in the check logs, not just evaluation/build success. Verify the development
shell can configure the documented build.

## Validation and rollback rules

Documentation-only changes require link/path, accuracy and whitespace review;
they do not require desktop restart or a full build. CMake/CI changes require
actual affected configurations, not just YAML/JSON parsing. Runtime changes
require the canonical runtime workflow in addition to compilation/tests.

Use eight build jobs unless a specific resource limit requires fewer. Do not
claim remote CI or packaging passed based only on a local unit-test run.

If a slice regresses behavior, stop further extraction and restore only that
slice's changes without overwriting unrelated edits. Preserve the failure log
and a regression test. Do not rewrite published history or automatically revert
commits without authorization. Fix or explicitly record failing gates rather
than making tests optional to finish the package.

## Required handoff record

Append evidence to the plan's ledger and link a separate report when needed:

```text
Package / slice:
Status: not started | in progress | pending validation | complete
Base revision / implementation revision (or uncommitted diff):
Files and behavior changed:
Authoritative state / ownership constraints:
Toolchain and environment:
Commands executed and exit status:
Compiler warning results:
Registered / passed / skipped / failed tests:
QML diagnostic delta:
Runtime feedback, logs and coredump/A-B evidence (if applicable):
CI / distribution / Nix evidence (if applicable):
Known failures and unavailable checks:
Next exact action and remaining dependencies:
Commit approval / push approval status:
```

Use durable repository or CI links for essential evidence; `/tmp` logs alone
are not a handoff. Never mark a package complete while its required checks are
pending. Future assistants should resume from evidence, not from an optimistic
summary of the previous conversation.
