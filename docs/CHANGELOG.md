# Changelog

## 2026-03-07
- Refactored `GameplaySession` into explicit state partitions: simulation, player combat, progression, presentation, debug/tool, and encounter runtime state.
- Updated runtime integration to use partitioned tool/tick ownership without changing replay determinism behavior.
- Added `gameplay_session_state_tests` to validate boundary wiring and getter/ownership consistency.
- Updated architecture documentation (`MasterSpec`, `DecisionLog`, `ImplementationPlan`, `HellEngine_Audit_Report`).
