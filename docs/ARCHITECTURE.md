# How Solar Eyes works

This page follows the keychain from the solar panel to a lit pixel. The wiring and the build steps are in the [README](../README.md). Where to change things is in [CONTRIBUTING.md](../CONTRIBUTING.md).

## The whole keychain

Power flows down Board B, and data flows across Board A. The firmware blocks are the files in `firmware/SolarEyes/`.

```mermaid
flowchart TB
    subgraph B["Board B: power, outer face"]
        direction TB
        PANEL["Solar panel<br/>30 x 30 mm"]
        USBC_CHG["Micro-USB<br/>on the charger"]
        CN["CN3065 linear charger<br/>500 mA default charge"]
        CELL[("EEMB LP502030<br/>250 mAh LiPo")]
        REG["CANADUINO buck/boost<br/>EN tied to VIN"]
        SW{{"Slide switch<br/>in the 5 V line"}}
        PANEL -->|"solar in"| CN
        USBC_CHG -->|"5 V in"| CN
        CN <-->|"BAT +/-"| CELL
        CN -->|"SYS OUT, 3.0-4.2 V"| REG
        REG -->|"5.0 V"| SW
    end

    subgraph A["Board A: logic, front"]
        direction TB
        MCU["ESP32-C3 SuperMini<br/>5 V pin, on-board 3.3 V LDO"]
        USBC["USB-C<br/>flash and serial"]
        OLED["0.96 in SSD1306<br/>128 x 64 at 0x3C<br/>rows 0-15 yellow, 16-63 blue"]
        USBC -.->|"same 5 V rail;<br/>switch off first"| MCU
        MCU -->|"3V3, GND"| OLED
        MCU -->|"GPIO6 SDA, GPIO7 SCL<br/>I2C at 100 kHz"| OLED
    end

    SW -->|"5 V"| MCU
    REG -->|"GND"| MCU

    subgraph FW["Firmware on the C3"]
        direction LR
        INO["SolarEyes.ino<br/>setup, frame loop"]
        EYES["eyes.cpp<br/>expressions, easing"]
        RENDER["render.cpp<br/>U8g2 drawing in RAM"]
        DRV["oled.cpp<br/>SSD1306 init and push"]
        INO --> EYES
        INO --> RENDER
        RENDER --> DRV
    end

    MCU -.- FW
    DRV -.->|"1 KB frame"| OLED
```

## Firmware files

`eyes.cpp` holds all the decisions and no hardware. It is the part `make test` builds on a laptop.

```mermaid
flowchart LR
    CFG["config.h<br/>pins, address, I2C clock,<br/>geometry, timing"]

    subgraph Host["Builds on a laptop"]
        EYES["eyes.h / eyes.cpp<br/>Expr, Pose, Eyes, EyeBox<br/>eyesUpdate, eyesTarget,<br/>eyesStep, eyesBob, eyeBox"]
        TEST["test/test_eyes.cpp"]
    end

    subgraph Device["Needs the C3"]
        INO["SolarEyes.ino"]
        RENDER["render.h / render.cpp<br/>renderFrame"]
        OLED["oled.h / oled.cpp<br/>oledBegin, oledPush"]
        U8G2[["U8g2<br/>full frame buffer"]]
        WIRE[["Wire<br/>ESP32 I2C"]]
        RNG[["esp_random()<br/>hardware RNG"]]
    end

    CFG --> EYES
    CFG --> RENDER
    CFG --> OLED
    TEST --> EYES
    INO --> EYES
    INO --> RENDER
    INO --> OLED
    INO -->|"passed in as RandomFn"| RNG
    RENDER --> EYES
    RENDER --> U8G2
    RENDER --> OLED
    OLED --> WIRE
```

`eyesUpdate()` takes the random source as a function pointer. The sketch passes `esp_random`, and the tests pass a scripted list of numbers, so every branch of the rotation can be checked.

## Boot and one frame

```mermaid
sequenceDiagram
    autonumber
    participant Sketch as SolarEyes.ino
    participant Eyes as eyes.cpp
    participant Render as render.cpp
    participant U8g2 as U8g2 buffer
    participant Oled as oled.cpp
    participant Panel as SSD1306

    Note over Sketch: setup()
    Sketch->>Oled: oledBegin()
    Oled->>Panel: Wire.begin(6, 7), 100 kHz, 50 ms timeout
    Oled->>Panel: 25 init commands, display on
    Sketch->>Eyes: eyesInit(now)
    Note right of Eyes: Idle for 1600 ms,<br/>shown = REST_POSE
    Sketch->>Render: renderFrame(eyes, now)

    Note over Sketch: loop(), every pass
    Sketch->>Eyes: eyesUpdate(now, esp_random)
    alt current expression has run its time
        Eyes->>Eyes: roll % 100 picks a ROTATION row,<br/>set duration, glance side, wink eye
    end
    alt before nextFrame
        Sketch->>Sketch: delay(5) so USB serial stays up
    else frame due
        Sketch->>Sketch: nextFrame = now + 33 ms
        Sketch->>Eyes: eyesStep()
        Note right of Eyes: every Pose field moves 1 px<br/>toward eyesTarget(), heights 2 px
        Sketch->>Render: renderFrame(eyes, now)
        Render->>U8g2: clearBuffer()
        Render->>U8g2: two brows, drawLine x 4 each
        Render->>Eyes: eyeBox() for each eye
        Render->>U8g2: drawRBox() x 2
        Render->>U8g2: memset pages 0-1 to zero
        Render->>Oled: oledPush(buffer)
        loop 8 pages
            Oled->>Panel: 0xB0 | page, column low, column high
            Oled->>Panel: 8 writes of 0x40 + 16 bytes
        end
    end
```

## Expressions

Each expression runs for its time, then a roll from `esp_random() % 100` picks the next one. The same expression can come up twice in a row. `Happy` has a pose in `eyesTarget()` but is not in `ROTATION` yet, so it never appears.

```mermaid
stateDiagram-v2
    direction LR
    [*] --> Idle: power on, 1600 ms
    state pick <<choice>>

    Idle --> pick: time is up
    Blink --> pick: time is up
    LookSide --> pick: time is up
    LookUp --> pick: time is up
    LookDown --> pick: time is up
    Wink --> pick: time is up
    Sleepy --> pick: time is up

    pick --> Idle: 0-17, 18 pct, 420-699 ms
    pick --> Blink: 18-33, 16 pct, 170 ms
    pick --> LookSide: 34-57, 24 pct, 480-759 ms
    pick --> LookUp: 58-71, 14 pct, 420 ms
    pick --> LookDown: 72-83, 12 pct, 420 ms
    pick --> Wink: 84-92, 9 pct, 260 ms
    pick --> Sleepy: 93-99, 7 pct, 700 ms

    LookSide: LookSide, left or right on a coin flip
    Wink: Wink, left or right eye on a coin flip
    Happy: Happy, has a pose, never picked
```

What each expression asks for, relative to rest (each eye 26 x 34 px, brow arch 4):

| Expression | Eyes | Brows |
| --- | --- | --- |
| Idle | rest | rest |
| Blink | both 4 px tall | down 2, arch 2 |
| LookSide | near eye 28 x 36, far eye 23 x 31, both slide 2 and 8 px toward the side | near brow up 1, arch 5 / 3 |
| LookUp | up 2, 36 px tall | up 2, arch 5 |
| LookDown | down 3, 26 px tall | down 2, arch 2 |
| Wink | one eye 4 px tall, nudged 2 px inward | that brow down 1, arch 2 |
| Sleepy | down 2, 22 px tall | down 2, arch 2 |
| Happy | 28 x 32 | up 2, arch 5 |

## From pose to pixels

```mermaid
flowchart TD
    T["eyesTarget(eyes)<br/>Pose for the current Expr"] --> S["eyesStep()<br/>shown moves toward target:<br/>1 px per frame, heights 2 px"]
    S --> BOB["eyesBob(now), eyesBob(now + 80)<br/>-2 to 2 px, 1120 ms cycle,<br/>right eye 80 ms behind"]
    BOB --> BROW["Brow y = 18 + brow + bob,<br/>clamped to row 16"]
    BOB --> BOX["eyeBox(cx, cy, w, h)<br/>min 10 x 3, top clipped to row 16,<br/>radius 5 or less so w, h >= 2(r+1)"]
    BROW --> DRAW["U8g2 full buffer, 1024 bytes<br/>8 pages x 128 columns"]
    BOX --> DRAW
    DRAW --> BLANK["memset bytes 0-255<br/>pages 0-1 = yellow rows 0-15"]
    BLANK --> PUSH["oledPush()<br/>per page: 3 command writes,<br/>then 8 data writes of 16 bytes"]
    PUSH --> GLASS["SSD1306 GDDRAM<br/>only blue pixels light"]
```

U8g2's full buffer and the SSD1306 share one layout: byte `page * 128 + column` holds 8 pixels stacked vertically, with bit 0 on top. That match is what lets `oledPush()` send the buffer straight through.

## I2C budget per frame

At 100 kHz each byte on the wire costs 9 bit times, or 90 µs.

| Part | Transactions | Bytes on the wire, address included |
| --- | ---: | ---: |
| Page and column commands | 24 | 72 |
| Pixel data, 16 bytes at a time | 64 | 1,152 |
| **Total** | **88** | **1,224, about 110 ms** |

That is the floor before any ESP32 driver overhead, and it is more than the 33 ms `FRAME_MS`. The loop starts the next frame as soon as the push finishes, so the panel updates at about 9 frames a second, not 30. Set `DEBUG_FRAME_TIME` to `1` in `config.h` to print the real push time over serial.

The easing is counted in frames, so a 170 ms Blink gets one or two steps before the next expression starts. The eyes dip from 34 px to about 30 px instead of closing. Pages 0-1 are always blank, so skipping them in `oledPush()` after the first frame would save a quarter of the bus time.
