# Mechanical files

This folder is where AutoBraid's mechanical design files belong. No CAD/STL files exist in the
repository yet — these subfolders are placeholders, not populated with any files.

- **[stl/](stl/)** — put exported `.stl` files here, one per printable part (e.g.
  `braid_motor_mount.stl`, `carousel_hub.stl`, `sensor_bracket.stl`). These are what someone would
  download to reprint a part.
- **[source/](source/)** — put native/editable CAD source files here (`.step`/`.stp`, or your CAD
  tool's native format — Fusion 360 `.f3d`, SolidWorks `.sldprt`/`.sldasm`, FreeCAD `.FCStd`, etc.).
  This is what someone would open to *modify* a part, not just print it.

See [../../docs/mechanical_design.md](../../docs/mechanical_design.md) for a description of the
mechanisms these parts belong to (braiding, rail, extension carousel).
