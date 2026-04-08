# Remove `cJSON` Without Size or Performance Regression

## Summary
Refactor the repo to remove `cJSON` entirely while preserving current behavior and holding flash size, RAM use, and parse latency to a no-regression bar. The implementation should not introduce an owned intermediate DOM or copy-heavy abstraction; instead, it should use fkYAML as the only parser and expose a very thin local view layer over `fkyaml::node` so the code stays readable without paying for another data model.

## Key Changes
- Replace `cJSON` with a lightweight wrapper pair:
  - `FlxValueDocument` owns source text plus parsed `fkyaml::node`
  - `FlxValueView` is a tiny inline wrapper over referenced nodes
- Keep the wrapper intentionally minimal and allocation-free:
  - no custom tree conversion
  - no heap-owned variant/value graph
  - no string copies except when a caller explicitly asks for `std::string`
  - helpers should be thin inline adapters around `fkyaml`
- Parse both JSON and YAML through fkYAML:
  - JSON uses `fkyaml::node::deserialize()`
  - YAML uses `fkyaml::node::deserialize()`
  - path-based dispatch remains in one place
- Remove the FlxApp YAML bridge entirely:
  - delete `FlxAppYamlBridge`
  - stop converting YAML into `cJSON`
  - make `FlxApp`, manifest loading, renderer, action runner, and state read directly from `FlxValueView`
- Preserve current runtime data behavior exactly where possible:
  - supported state scalar types remain string, integer, boolean
  - `variables`, `ui`, actions, manifest arrays, and wallpaper preset fields keep their current semantics
  - button `on_click` continues to accept object or array
- Refactor wallpaper preset parsing to use the same fkYAML-backed view layer:
  - top-level field reads come from `FlxValueView`
  - `effects` is emitted back to compact JSON text using `FlxValueView::toJsonString()` so downstream behavior stays unchanged
- Remove all `cJSON` usage from headers and build files:
  - no `struct cJSON` forward declarations
  - remove `json` from `FlxApp/CMakeLists.txt`
  - remove `espressif/cjson` from `UI/idf_component.yml`

## Performance And Size Guardrails
- The wrapper layer must be near-zero-overhead by design:
  - `FlxValueView` stores only a node pointer plus shared root owner, no copied child lists
  - iteration stays on native fkYAML children
  - scalar conversion happens only at the read site
- Do not add any second representation of parsed content.
- Preserve parse lifetime by storing the original source text and parsed root inside `FlxValueDocument`; do not clone subtrees or materialize JSON strings except where already required by existing APIs.
- Update `FlxAppBenchmark` to compare old and new like-for-like before deleting the old path:
  - benchmark current `cJSON` JSON parse
  - benchmark new fkYAML JSON parse
  - benchmark YAML parse on the new direct path
- Treat regressions as blockers:
  - no flash growth attributable to the refactor
  - no additional persistent heap held after document load beyond the new single-parser path
  - no parse-latency regression for FlxApp JSON inputs on the existing benchmark workload
- If a wrapper helper causes measurable regression, the plan defaults to inlining or moving that access pattern directly to `fkyaml` rather than keeping the abstraction.

## Interface Changes
- `FlxAppState::loadFromJson(const cJSON*)` -> `loadFromValue(FlxValueView)`
- `FlxAppState::setFromJson(const std::string&, const cJSON*)` -> `setFromValue(const std::string&, FlxValueView)`
- `FlxAppActionRunner::run(const cJSON*)` -> `run(FlxValueView)`
- Renderer button bindings store `FlxValueView` instead of `const cJSON*`
- `FlxApp` stores `FlxValueDocument` and `FlxValueView m_uiRoot`
- No public/internal header exposes `cJSON`

## Test Plan
- Functional:
  - JSON FlxApp loads and behaves exactly as before
  - YAML FlxApp loads and behaves exactly as before
  - manifest parsing matches prior results for strings, ints, bools, and string arrays
  - action execution and binding refresh behavior remain unchanged
  - wallpaper presets still load valid configs and preserve `effects` JSON output
- Non-functional:
  - run `FlxAppBenchmark` before and after the refactor on the same fixture files
  - compare parse totals/averages for JSON and YAML
  - compare final component/binary size after removing `cJSON`
  - confirm repo search shows no remaining runtime/build references to `cJSON`
- Acceptance:
  - if functionality passes but size or performance regresses, the change is not accepted until the regression is removed

## Assumptions
- “Nothing should be compromised” means size, RAM, and parse performance are hard acceptance gates.
- A thin local wrapper over fkYAML is acceptable only if it remains essentially free at runtime; otherwise direct `fkyaml` calls should replace the costly parts.
- The refactor targets repo-wide `cJSON` removal, including wallpaper preset parsing, so the dependency can actually be dropped.
