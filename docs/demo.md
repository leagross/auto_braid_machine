# Demo

## Full Machine Demo

TODO: INSERT VIDEO LINK

> The owner has a real video of the working machine — link it here (and update the link in
> [README.md](../README.md#-demo)) once it's ready to publish. Access to the current copy is by
> request; see the link already in the README.

The video should demonstrate, in order:

1. **Machine startup** — boot-time carousel homing (`ST_BOOT_HOMING`), screen coming up at
   `ST_WAIT_CODE`.
2. **Touchscreen interaction** — entering a code on the keypad, selecting up to 3 extensions,
   confirming "insert hair" and "ready to braid".
3. **Sensor operation** — the ultrasonic distance check, and (if a "MyHair" extension was chosen)
   the color scan and its detected-color readout on screen.
4. **Motor operation** — the extension carousel turning to dispense each chosen extension, and the
   braid motor running.
5. **Rail movement** — the rail returning to its home position at the end of a session.
6. **Braiding mechanism** — the physical braiding action itself.
7. **Complete workflow** — start to finish: code entry → extension choice → hair insertion →
   sensing → dispensing → braiding → done screen → order saved.

Ideally also include, if available in a future recording: a normal successful run end-to-end, and a
short clip of the emergency-stop button being pressed mid-braid (see [safety.md](safety.md)) to
demonstrate the safety behavior, not just the happy path.

## Thumbnail

Recommend adding a static thumbnail image of the machine to the top of the main
[README.md](../README.md), above or alongside the demo video link, so the project reads visually
before anyone clicks through. See [images/README.md](images/README.md) for the list of recommended
photos — the "complete machine" shot is the natural thumbnail candidate.
