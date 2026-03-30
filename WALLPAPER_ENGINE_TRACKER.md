# Wallpaper Engine Tracker

Source Plan: `WALLPAPER_ENGINE_PLAN.md`
Plan Version: 1.4
Tracker Status: Active
Last Synced: 2026-03-29

## Objective
This tracker turns the wallpaper plan into an execution board with explicit status, owners, next actions, and evidence links.

## Status Legend
- Not Started: No implementation work started.
- In Progress: Implementation underway.
- Blocked: Waiting on a decision, dependency, or evidence.
- Complete: Implemented and validated against acceptance criteria.
- Evidence Pending: Implementation exists, but required artifacts are not yet attached.

## Phase Tracker
| Track | Window | Current Status | Owner | Evidence State | Next Action |
|---|---|---|---|---|---|
| Cross-Phase: Wallpaper Engine App | Weeks 2-10 | Complete | App/UI | Evidence Pending | Attach screenshots/logs for fallback banner and page flows. |
| Phase 1: Foundation | Weeks 1-2 | Complete | Runtime | Complete | None. Keep regression checks in CI. |
| Phase 2: Animation Support | Weeks 3-4 | In Progress | Runtime | Evidence Pending | Run device benchmarks and attach GIF/Lottie artifacts. |
| Phase 3: Effect System | Weeks 5-6 | Complete | Runtime + UI | Evidence Pending | Confirm no remaining UI gaps and collect quick validation notes. |
| Phase 4: Dynamic Wallpapers | Weeks 7-8 | Complete | Runtime + UI | Evidence Pending | Capture performance evidence on target boards. |
| Phase 5: Preset Library | Weeks 9-10 | In Progress | Runtime + Design + QA | Evidence Pending | Expand from 3 presets to production target and finalize ownership throughput. |
| Phase 6: Adaptive Wallpapers | Weeks 11-12 | Not Started | Runtime | Not Started | Define AdaptiveProvider design and service integration contract. |
| Phase 7: Polish & Optimization | Weeks 13+ | Not Started | Runtime + QA | Not Started | Start after P0 and Phase 5 evidence gates pass. |

## P0 Ticket Tracker
| Ticket | Status | Owner | Acceptance State | Next Action | Required Artifacts |
|---|---|---|---|---|---|
| WPE-P0-01 GIF Playback Benchmark Harness | In Progress | Runtime | Evidence Pending | Capture 60s runs on `generic-esp32` and `generic-esp32s3`. | `artifacts/wallpaper/gif_benchmark_<profile>.csv`, `artifacts/wallpaper/gif_benchmark_summary.md` |
| WPE-P0-02 Lottie Complexity Gate + Downgrade | In Progress | Runtime | Evidence Pending | Produce hardware matrix with complexity gate outcomes and perf numbers. | `artifacts/wallpaper/lottie_gate_matrix.md` + reason-coded runtime logs |
| WPE-P0-03 Fallback/Error UX in App | In Progress | App/UI | Evidence Pending | Capture screenshots and logs showing banner appearance, retry, and auto-dismiss. | Screenshot set + runtime logs |
| WPE-P0-04 Provider Switching and Persistence Validation | In Progress | Runtime + QA | Evidence Pending | Execute soak/reboot scenarios and publish report bundle. | `artifacts/wallpaper/` run logs and summary markdown |

## Acceptance Evidence Matrix
| Row | Artifact Target | Current State | Notes |
|---|---|---|---|
| GIF playback gate | `artifacts/wallpaper/gif_benchmark_generic-esp32.md`, `artifacts/wallpaper/gif_benchmark_generic-esp32s3.md` | Pending | Waiting for device captures. |
| Lottie gate | `artifacts/wallpaper/lottie_benchmark_generic-esp32.md`, `artifacts/wallpaper/lottie_benchmark_generic-esp32s3.md` | Pending | Waiting for device captures. |
| Fallback UX | screenshots + runtime logs | Pending | Banner path implemented; evidence not attached. |
| Provider switch soak | `artifacts/wallpaper/` soak report | Pending | Need 1000-switch run and heap trend summary. |
| Persistence restore | reboot validation report in `artifacts/wallpaper/` | Pending | Need checklist execution across target profile(s). |

## Open Decisions
| Decision | Options | Current State | Owner |
|---|---|---|---|
| GIF decode strategy | Predecode vs streaming decode | Open | Runtime |
| Lottie low-end policy | Hard reject vs auto-poster fallback | Open | Runtime |
| Default quality profile by board class | Conservative ESP32 vs medium ESP32-S3 defaults | Open | Runtime |

## This Week Focus (Immediate)
- Close Phase 2 validation gaps with real hardware evidence.
- Publish P0 acceptance artifacts under `artifacts/wallpaper/`.
- Capture fallback UX screenshots/logs.
- Execute provider-switch and reboot persistence validation.

## Update Log
- 2026-03-29: Tracker created from `WALLPAPER_ENGINE_PLAN.md` v1.4.

## Update Template
Use this snippet for each status update:

```md
### YYYY-MM-DD
- Scope:
- Changes:
- Evidence Added:
- Risks/Blockers:
- Next Action:
```
