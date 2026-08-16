# Mechanical Design

AutoBraid is a physical machine, not just firmware — three stepper-driven mechanisms plus (per the
project's mechanical scope) 3D-printed structural parts. This document describes what's confirmed
from the firmware and README; it does not describe geometry, dimensions, or CAD details that aren't
present anywhere in the repository — those are owner TODOs (see below and
[DOCUMENTATION_UPGRADE_REPORT.md](DOCUMENTATION_UPGRADE_REPORT.md)).

## Braiding mechanism

Driven by the **braid motor** — a 28BYJ-48 stepper via a ULN2003 driver, on the second (master)
board (pins `27,33,32,3`). During `ST_BRAIDING`, `braidTick()` steps it in one direction
continuously for up to `BRAID_DURATION_MS` (60 seconds), or until an emergency stop. See
`second_esp32/Motors.ino` and [state_machines.md](state_machines.md).

## Rail mechanism

Driven by the **rail motor** — a 28BYJ-48 stepper wired/driven in **bipolar** mode through an
L298N dual H-bridge driver (pins `26,25,14,13`), a different phase table from the two unipolar
motors (`RAIL_PHASE_SEQ` vs. `PHASE_SEQ` in `Motors.ino` — not interchangeable). The rail is
designed to lower in segments (`RAIL_SEGMENT_STEPS`/`RAIL_MAX_SEGMENTS` in `Config.h`) and return
up by exactly the number of segments actually completed (`railStartUpHome()`), so a mid-braid
emergency stop returns accurately rather than overshooting. As documented in-code and in
[state_machines.md](state_machines.md), the segmented-lowering logic is not currently driven by
anything — this is a known, explicitly-flagged gap, not part of this document's scope to fix.

## Extension (carousel) dispenser mechanism

Driven by the **extension/dispenser motor** — a 28BYJ-48 stepper via a ULN2003 driver, on the
**first (slave)** board (pins `2,15,18,5`) — see [hardware_architecture.md](hardware_architecture.md)
for why this motor lives on the sensor board rather than the master. It's a carousel with (at
least) 4 positions corresponding to the 4 physical extension colors (`EXT_BLONDE`, `EXT_GREEN`,
`EXT_RED`, `EXT_BLACK`), plus a home/none position. `Dispenser.ino` moves it a computed number of
steps (typically a quarter turn, `DISP_QUARTER_STEPS = DISP_STEPS_PER_REV / 4`) to reach a
requested position, non-blockingly.

## Motor mounts

Not documented anywhere in the repository — physical mounting hardware/brackets for the three
28BYJ-48 motors are an owner TODO. If any of these are 3D-printed, STL/STEP files belong in
[hardware/mechanical/](../hardware/mechanical/) (see below).

## Sensor mounts

Not documented anywhere in the repository — physical mounting for the HC-SR04 ultrasonic sensor
and the TCS34725 color sensor (positioning matters for both: the ultrasonic sensor's valid range is
calibrated to 7.0–16.0 cm, and the color sensor's LED-lit raw-value calibration depends on a
consistent distance/angle to the hair). Owner TODO.

## Extension mechanism

See "Extension (carousel) dispenser mechanism" above — this is the same subsystem; the physical
carousel geometry (how many slots, spacing, load method) isn't described anywhere in the repo
outside of the 4-position code mapping.

## 3D-printed components

The project brief and portfolio goal call out 3D-printed parts as part of the mechanical scope, but
no STL/STEP files or descriptions of which parts are printed currently exist in the repository. This
is the single largest documentation gap identified in
[PROJECT_ANALYSIS.md](PROJECT_ANALYSIS.md) — see the folders below, ready for the owner to fill in.

## Where to put CAD files

```text
hardware/mechanical/
hardware/mechanical/stl/       ← STL files for 3D printing
hardware/mechanical/source/    ← STEP / native CAD source files (e.g. Fusion 360, SolidWorks, FreeCAD)
```

These folders exist in the repository (see [hardware/mechanical/README.md](../hardware/mechanical/README.md))
but are currently empty — no fake or placeholder model files were created. Add real STL/STEP files
there when available.
