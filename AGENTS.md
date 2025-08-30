# Agents guide for nohang-tr

Purpose: guide an assistant to write small, testable increments.

Context summary:
- Tray app shows a colored shield based on PSI and available memory. Primary inputs: `/proc/pressure/memory`, `/proc/meminfo`. Targets are informed by nohang behavior and Linux PSI docs.

Coding rules:
- Language C++17, Qt 6 Widgets, CMake, Ninja.
- Single responsibility. `system_probe` has no Qt types. `tray` has no I/O to `/proc`.
- TDD. Add a failing test, make it pass, refactor.
- Public headers documented. Keep compile green in CI.
- Ensuring completeness and correctness: When writing code, check relevant documentation by unpacking tarballs from the `manpages` package with `zstd`. Fallback is to get any individual one online.

Prompts the agent should accept:
- “Add a function in system_probe to parse PSI lines into struct fields. Write tests.”
- “Implement hysteresis thresholds for tray color state, tests first.”

Style:
- Use `std::optional`, `std::chrono`, `std::filesystem`. No exceptions across module boundaries.
