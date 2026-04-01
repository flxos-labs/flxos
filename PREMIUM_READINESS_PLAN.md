# FlxOS Premium Readiness Master Plan

Status: Active
Version: 1.1
Last Updated: 2026-04-01
Program Horizon: 12 weeks (rolling)

## 1. Program Objective

Deliver a consistent, professional, and premium FlxOS platform across architecture, code quality, product polish, developer experience, and release reliability.

Premium in this plan means:
- Predictable architecture with low accidental coupling.
- Reliable quality gates with measurable debt burn-down.
- Complete and discoverable technical documentation.
- Stable runtime behavior across profiles and mode variants.
- Product-level finish in UI/UX behavior and performance.

## 2. Verified Initial Baseline (2026-03-31, before quick wins)

Baseline run from repository quality scripts:

| Gate | Result | Baseline Value |
|---|---|---|
| Format | Fail | 2 files need formatting |
| Naming | Fail | 80 issues |
| Documentation | Fail | 846 issues (236 header + 599 API docs + 11 README) |
| Includes | Fail | 674 include order issues |
| Complexity | Fail | 87 issues |

Overall initial baseline:
- Gates passed: 0/5
- Gates failed: 5/5

## 2.1 Current Checkpoint (2026-04-01)

Latest baseline run from scripts/run_premium_baseline.sh:

| Gate | Result | Current Value |
|---|---|---|
| Format | Pass | 0 files need formatting |
| Naming | Fail | 80 issues |
| Documentation | Fail | 835 issues (236 header + 599 API docs + 0 README) |
| Includes | Fail | 674 include order issues |
| Complexity | Fail | 87 issues |

Checkpoint summary:
- Gates passed: 1/5
- Gates failed: 4/5
- Net progress since baseline: formatting and README coverage sustained; naming/include/complexity unchanged.

## 2.2 P0-04 Include Tranche Progress (2026-04-01)

Targeted include-only reruns after tranche work show:
- Include order issues: 347 (down from 674)
- Net reduction: 327 issues
- Evidence: reports/premium_readiness/2026-04-01/includes_p0_04_tranche1.log, reports/premium_readiness/2026-04-01/includes_p0_04_tranche2.log, reports/premium_readiness/2026-04-01/includes_p0_04_tranche3.log, reports/premium_readiness/2026-04-01/includes_p0_04_tranche4.log, and reports/premium_readiness/2026-04-01/includes_p0_04_tranche5.log

Note:
- This reflects focused include checker reruns; a full 5-gate baseline rerun is still required for the next global checkpoint snapshot.

## 3. Success Metrics

### 3.1 Program-Level KPIs

| KPI | Baseline | Week 4 Target | Week 8 Target | Week 12 Target |
|---|---|---|---|---|
| Quality gates passing | 0/5 | 2/5 | 4/5 | 5/5 |
| Complexity issues | 87 | <= 65 | <= 35 | <= 10 |
| Include order issues | 674 | <= 350 | <= 120 | 0 |
| Naming issues | 80 | <= 40 | <= 10 | 0 |
| Documentation issues | 846 | <= 500 | <= 180 | <= 40 |
| Missing module READMEs | 11 | <= 5 | <= 1 | 0 |

### 3.2 Runtime and Product KPIs

| KPI | Baseline | Week 12 Target |
|---|---|---|
| Headless/GUI mode parity checks | Ad hoc | Automated matrix for at least 4 profiles |
| Boot and app-start regressions in CI | Partial | Standard smoke suite per profile class |
| UI consistency scorecard coverage | None | 100 percent for built-in apps |

## 4. Workstreams

### WS1: Gate Reliability and Fast Hygiene
Objective: Move from red baseline to stable, trusted quality gates.

Deliverables:
- Daily baseline report generation and trend tracking.
- Immediate formatting cleanup and low-risk naming cleanups.
- Issue categorization into true violations vs rule false positives.

Definition of done:
- Format gate green and stable for 7 consecutive days.
- Naming false positives documented and reduced.

### WS2: Complexity and Maintainability Debt Reduction
Objective: Reduce high-risk complexity hotspots in system-critical modules.

Priority hotspots:
- System/Source/services/FileSystemService.cpp
- Services/Source/ServiceRegistry.cpp
- Apps/Source/AppManager.cpp
- UI/Source/desktop/Desktop.cpp

Deliverables:
- Function decomposition plans per hotspot.
- Refactors preserving behavior with targeted tests/log evidence.

Definition of done:
- Complexity issues reduced by at least 75 percent from baseline.

### WS3: Architecture Hardening
Objective: Protect layer boundaries and remove upward dependencies.

Priority area:
- Connectivity layer dependence on System-level manager contracts.

Deliverables:
- Interface extraction at boundary points.
- Dependency inversion via contracts/events.

Definition of done:
- No direct upward layer dependencies in critical runtime paths.

### WS4: Documentation and Discoverability
Objective: Make codebase self-navigable and review-friendly.

Deliverables:
- Module README coverage for all core modules.
- Public API doc pass for highest-use headers.
- File-level header policy and templates.

Definition of done:
- Missing READMEs at zero.
- Undocumented public functions reduced to target.

### WS5: Include and Naming Governance
Objective: Achieve deterministic style consistency with low review friction.

Deliverables:
- Include order auto-fix strategy.
- Naming rules tune-up to avoid noisy false positives.

Definition of done:
- Include order issues at zero.
- Naming issues at zero or approved exceptions list.

### WS6: Runtime Validation Matrix
Objective: Convert profile/mode correctness into repeatable checks.

Deliverables:
- Profile matrix covering headless and graphical modes.
- Build, boot, and key-service smoke checks per profile class.

Definition of done:
- Matrix integrated into CI and reproducible locally.

### WS7: Product Polish and UX Consistency
Objective: Raise user-facing quality to premium level.

Deliverables:
- UI consistency rubric (spacing, typography, navigation, states).
- Built-in app polish sweep with acceptance screenshots.

Definition of done:
- Rubric pass for all built-in apps.

### WS8: Release and Operational Readiness
Objective: Ensure release confidence and traceability.

Deliverables:
- Release readiness checklist linked to quality/report artifacts.
- Known-risk ledger with mitigation state.

Definition of done:
- Release candidate sign-off package with evidence links.

## 5. Execution Cadence

Weekly rhythm:
- Monday: baseline run, target selection, risk review.
- Midweek: implementation batch and debt burn-down update.
- Friday: quality gate rerun, evidence update, tracker sync.

Governance:
- One owner per workstream.
- Tracker updates mandatory after every quality run.
- Re-baseline after major refactor merges.

## 6. Priority Backlog (Next 2 Weeks)

1. Close WS1 quick wins.
2. Start documentation tranche-1 to reduce file-header debt and public API doc gaps in highest-use modules.
3. Completed 2026-04-01: reduce include issues below week-4 target (674 -> 347) via deterministic ordering sweeps in low-risk files first.
4. Reduce top-4 complexity hotspots with behavior-preserving refactors and evidence logs.
5. Establish first build matrix for two headless and two graphical profiles.

## 7. Risks and Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Large legacy functions are risky to refactor | Regression risk | Stage refactor behind small commits and logs/tests per change |
| Strict static rules produce false positives | Team fatigue | Tune rules and maintain explicit exception policy |
| Profile-mode differences cause hidden breakages | Runtime instability | Enforce matrix runs on every release branch cut |
| Documentation backlog grows faster than edits | Drift | Add docs update requirement to PR checklist |

## 8. Program Launch Actions (Executed 2026-03-31)

1. Baseline quality run captured with logs and counts.
2. Premium execution tracker initialized.
3. Automated baseline script added for repeatable reporting.

## 9. Current Sprint Focus (2026-04-01 to 2026-04-14)

1. WS5 include-order tranches 1 and 2 complete: fixed 327 include-order issues in low-risk modules; week-4 target achieved (<= 350). Next is tranche-3 toward <= 120.
2. WS2 hotspot decomposition prep: create extraction plans for FileSystemService, ServiceRegistry, AppManager, and Desktop.
3. WS4 docs tranche-1: add file headers and API docs for top-impact headers to reduce docs issues by at least 120.
4. WS1 naming signal cleanup: classify true violations vs acceptable external/C-style names and prepare rule-tuning proposal.
5. WS6 matrix bootstrap: define 4-profile smoke command matrix and capture first reproducible local run evidence.
