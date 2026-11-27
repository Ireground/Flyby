# Flyby

A Doppler motion plugin (VST3). It moves a sound along a path around you and gives it the pitch, level, panning, air, ground reflection and reverb a real moving source would have. It does the same job as the old Waves Doppler, with tempo sync, height, MIDI triggers and more.

## How it works
Real physics: the sound goes into a delay line and is read back at the time sound takes to travel from the moving source to **each ear**. The pitch shift, timing and left/right ear differences come from that, not from a pitch shifter. At 100 % the pitch matches the textbook formula within 0.1 %.

## The field
- **You** are the red head in the middle. The floor is tilted, so **height goes straight up**. A raised point shows a stem down to the floor, and its path gets a dashed shadow on the ground.
- **Drag** a point to move it. **Shift+drag**, the **mouse wheel** or dragging the **diamond** changes height.
- **Double-click** the path or the floor to **add** a point (up to 12). Double-click a point to **remove** it.
- **Right-click** a point to delete it or change its height. Right-click the floor for **closed loop**, reverse, mirror, flatten and reset.
- The box at the top left shows live **speed (km/h), pitch (semitones), distance and time**.
- The **timeline** under the field shows the pitch (gold) and level (blue) of one pass. The white line is where you are now, and **CLOSEST** marks the closest point, which is the pitch drop.

## Motion
| Control | What it does |
|---|---|
| **Mode** | **Loop** repeats. **Ping-Pong** goes back and forth. **One-Shot** does one pass per trigger. **Manual** uses the **Position** knob, so you can automate it in REAPER |
| **Tempo sync** + length | Pass length from 1/16 bar to 32 bars. While REAPER plays, loops are locked to the song position |
| **Length** | Pass length in seconds when sync is off |
| **Center** | When the middle of the path is reached. Below 50 % = fast approach, slow leaving |
| **Phase** | Shifts a loop so the closest point lands on the drop |
| **Trigger** | **MIDI note** (any note on the track), **Audio level** (above Threshold) or **Button only**. The big button always triggers |
| **Reverse direction** | Runs the path backwards |
| **On time at closest point** | On: the sound is exactly in time when it passes you, so it stays on the beat. Off: real sound travel delay (far = late) |

## Reality knobs
**Distance** sets the size of the field (outer ring, 2–300 m). Bigger = faster movement = stronger effect.
**Gain, Pan, Pitch, Air Damp**: 100 % = real life, 0 % = off, 200 % = exaggerated. **Pitch** goes from −100 % (reverse Doppler, physics broken on purpose) to 300 %.
**Ground** is the reflection off the ground (the jet/car "whoosh" flanging). **Width**: 0 % = the sound is one point (realistic); higher keeps some stereo width.

## Space, spin, output
**Reverb / Rev Time / Brightness**: the reverb level stays constant while the direct sound fades, so far-away sounds get more reverb, like real life.
**Spin / Spin Rate**: spins the source like a Leslie horn (level, brightness and pitch wobble). Use it on its own or while the source moves.
**Mix, Output.**

## Presets
26 presets in three groups: realistic (car, highway, train, jet flyover, rocket, U-turn…), music (orbit, figure 8, drop swoosh, riser fly-in, orbiting arp, fly-over, side to side) and creative (Leslie fast/slow, Pannerizer, Jerky tape, Reverse Doppler, Wormhole, Bee around your head, UFO hover). They also show up in REAPER's preset list. The **icon** menu (car, jet, rocket, train, UFO, bee, note, dot) only changes the picture.

## Tips for music
- **Drop swoosh:** One-Shot + MIDI trigger. Put a MIDI note on the track where the swoosh should start.
- To land a pass exactly on a bar: Loop + tempo sync, then turn **Phase** until the timeline's CLOSEST line sits where you want it.
- Works on a mono or stereo track. The output is always stereo.

## Build
GitHub Actions builds the Windows VST3 on every push and runs **pluginval at strictness 10**. Go to **Actions → latest run → Flyby-VST3-Windows** and copy `Flyby.vst3` to `C:\Program Files\Common Files\VST3`.
