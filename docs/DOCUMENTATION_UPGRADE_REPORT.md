# Documentation Upgrade Report

Summary of the documentation-only upgrade pass applied to this repository. No source code or
runtime behavior was changed — see [Code Changes](#code-changes) below.

## Files Created

- `docs/PROJECT_ANALYSIS.md` — repo audit: what exists, architecture, strengths, gaps
- `docs/hardware_architecture.md` — block diagram + per-board hardware responsibilities
- `docs/state_machines.md` — both boards' logic as Mermaid state diagrams/flowchart
- `docs/uart_protocol.md` — UART message table + sequence diagram
- `docs/mechanical_design.md` — braiding/rail/carousel mechanisms, 3D-printed parts status
- `docs/demo.md` — demo video documentation and shot list
- `docs/images/README.md` — recommended photo list (no images added)
- `docs/safety.md` — implemented safety behavior vs. future ideas, incl. an emergency-flow diagram
- `docs/system_flow.md` — end-to-end sequence diagram + Firebase data-flow diagram
- `docs/project_overview.md` — concise, interview-ready project summary
- `docs/DOCUMENTATION_UPGRADE_REPORT.md` — this file
- `hardware/BOM.md` — bill of materials in the requested table format
- `hardware/mechanical/README.md`, `hardware/mechanical/stl/`, `hardware/mechanical/source/` —
  placeholder structure for CAD/STL files (empty; no fake files created)

## Documentation Improved

- `README.md` — restructured around the requested sections (Demo, Key Engineering Features, System
  Architecture, Hardware, Embedded Architecture, Communication, Sensors, Motors and Actuators,
  Mechanical Design, Web Application, Safety, Repository Documentation, Future Improvements), with
  every technical claim (pins, UART commands, Firestore schema, setup steps) carried over unchanged
  from the previous version and cross-linked to the new `docs/` files instead of duplicated.
- `components.md` and `function_map.md` — left as-is; both were already accurate and detailed, and
  are now linked from the README and the new docs rather than superseded. `hardware/BOM.md` is the
  new canonical BOM location going forward; `components.md` is kept so existing links/history
  aren't broken.

## Diagrams Added

All in Mermaid, all verified against source code (pins, states, message names) before writing —
none describe behavior that isn't actually implemented:

1. Complete architecture — `README.md` (unchanged, already existed) and
   `docs/hardware_architecture.md` (new block diagram)
2. ESP32 Master/Slave architecture — `docs/hardware_architecture.md`
3. UART sequence — `docs/uart_protocol.md`
4. State machines — `docs/state_machines.md` (master: full `stateDiagram-v2`; slave: flowchart)
5. System workflow — `docs/system_flow.md` (sequence diagram)
6. Emergency flow — `docs/safety.md` (flowchart)
7. Web/Firebase data flow — `docs/system_flow.md` (flowchart)

## Missing Information

Everything below is called out inline (as `Not documented yet` in tables, or as an explicit TODO)
rather than invented:

- Exact development board part numbers/links for the two ESP32s (three candidates were named in the
  brief; the code doesn't pin down which is which beyond "the one with the screen")
- Motor/L298N power supply voltage and current specs
- Enclosure/mounting hardware details
- Any CAD/STL files for the 3D-printed parts
- Any project photos
- A public/unrestricted demo video link (current link requires access request)

## Owner TODOs

- Upload the demo video (or share a non-restricted link) and update it in both `README.md` and
  `docs/demo.md`
- Add machine photos to `docs/images/` per the shot list in `docs/images/README.md`, and link the
  "complete machine" shot as a README thumbnail
- Add STL files to `hardware/mechanical/stl/`
- Add STEP/CAD source files to `hardware/mechanical/source/`
- Confirm exact board part numbers and fill in the `Not documented yet` cells in `hardware/BOM.md`
  (power supply specs, enclosure/mounting hardware)
- Decide whether to keep `components.md` at the repo root long-term or fully consolidate into
  `hardware/BOM.md`

## Code Changes

**No source-code or runtime-behavior changes were performed as part of this documentation
upgrade.** All firmware (`first_esp32/`, `second_esp32/`), the React app (`react-app/`), build
configuration, and CI (`.github/workflows/build.yml`) are unchanged.
