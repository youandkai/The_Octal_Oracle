# The Octal Oracle

> *"From the void, the Eye observes. When the shadow stirs, the fires awaken to tell the stories of the old world and the new."*

A motion-activated 360° light installation built for a dance studio. Eight 12V LED channels arranged as a compass respond to presence — playing one of 16 narrative light animations when someone enters, then fading out when the room goes quiet. Intelligent occupancy detection keeps the lights on during still periods like meditation or stretching, so a dancer holding a pose doesn't suddenly go dark.

---

## How It Works

The Oracle sits in an idle state with all lights off. When the PIR sensor detects motion, it selects one of 16 animations and plays it as a "wake-up" sequence, then holds all lights on. When the room has been quiet long enough, a reverse-chase shutdown animation plays and the lights go off.

The system is smart about *why* the room is quiet. A single person walking through gets a short timeout. An active room with lots of movement builds up a confidence score that extends the timeout. When a class arrives (6+ motion events in 2 minutes), **Class Lock** engages — the lights won't turn off until there's been 12 full minutes of silence, protecting still periods mid-class.

```
Power on → PIR warmup (10s) → IDLE
                                 ↓ motion detected
                            ANIMATING (wake-up sequence)
                                 ↓ animation complete
                            LIGHTS ON (monitoring motion)
                                 ↓ quiet long enough
                           SHUTTING DOWN (reverse chase)
                                 ↓ complete
                              IDLE ←────────────────────
```

---

## Hardware

| Component | Details |
|---|---|
| Microcontroller | Arduino (Uno or compatible) |
| Motion sensor | Parallax PIR sensor |
| LED channels | 8× 12V LED strips or fixtures |
| Driver | Octal N-channel MOSFET array |
| Power | 12V supply for LEDs, 5V for Arduino |

### Pin Map

| Pin | Position | Direction |
|---|---|---|
| 2 | Up (North) | 12 o'clock |
| 3 | Upper Right | 1:30 |
| 4 | Right | 3 o'clock |
| 5 | Lower Right | 4:30 |
| 6 | Down (South) | 6 o'clock |
| 7 | Lower Left | 7:30 |
| 8 | Left | 9 o'clock |
| 9 | Upper Left | 10:30 |
| 10 | PIR sensor input | — |

---

## Animations

16 narrative animations, each with its own character and timing. One is selected on each motion trigger using a counter + random offset, guaranteeing variety even if the random number generator repeats.

**Original 8**
| Name | Duration | Narrative |
|---|---|---|
| Rise | 2.4s | Sun climbing from horizon to zenith |
| Whirlpool | 2.4s | Water spinning into a vortex |
| Heartbeat | 3s | Double pulse, then full bloom |
| Pinball | ~2s | Ball ricocheting around the compass |
| Pincer | ~2s | Two arms closing from opposite sides |
| Radar | ~2s | Sweep rotating around the dial |
| Spark | ~1.5s | Electric crackle outward from center |
| Compass | ~2s | Cardinal points lighting in sequence |

**New in v3.0**
| Name | Duration | Narrative |
|---|---|---|
| Lightning | ~1.5s | Jagged strike from sky to ground |
| Tide | ~3s | Ocean waves rolling in and retreating |
| Eclipse | ~2.5s | Shadow passing across the light |
| Firefly | ~4s | Random twinkling scatter, then bloom |
| Avalanche | ~2s | Cascading tumble from peak to valley |
| Pendulum | ~3.5s | Swinging side to side, accelerating |
| Serpent | ~3.5s | Slithering clockwise, then back |
| Nova | ~2.5s | Stellar collapse then full explosion |

---

## Occupancy Detection

The system builds a **motion confidence score** (0–100) and uses it to decide how long to stay on after motion stops.

| Score | Timeout | Scenario |
|---|---|---|
| 0–24 | 15 seconds | Likely false trigger, brief pass-through |
| 25–59 | 30 seconds | Someone in the room, moderate activity |
| 60–100 | 2 minutes | Active room, high confidence |
| Class Lock | 12 minutes | Class in session |

Each rising edge from the PIR adds 18 points to the score. The score decays by 1 point per second when the room is quiet.

### Class Lock

When 6 or more motion events occur within any 2-minute window, Class Lock engages. This is the signature of a group arriving together. While locked:

- Timeout extends to 12 minutes of silence
- Failsafe extends from 15 minutes to 2 hours
- A single still dancer won't trigger shutdown

Class Lock releases automatically after 12 consecutive minutes with no motion.

---

## Configuration

All tunable values are defined as constants at the top of the sketch.

```cpp
// Timeout thresholds
const unsigned long OFF_SHORT = 15000;       // 15s — low confidence
const unsigned long OFF_30S   = 30000;       // 30s — medium confidence
const unsigned long OFF_2MIN  = 120000;      // 2 min — high confidence

// Class lock
const int LOCK_EDGES_MIN = 6;               // edges needed to engage lock
const unsigned long LOCK_WINDOW_MS = 120000; // within this window (2 min)
const unsigned long CLASS_LOCK_QUIET_OFF = 12UL * 60UL * 1000UL; // 12 min to release

// Failsafes
const unsigned long MAX_ON_TIME = 15UL * 60UL * 1000UL;          // 15 min normal
const unsigned long CLASS_FAILSAFE_ON = 2UL * 60UL * 60UL * 1000UL; // 2 hrs class

// Motion score
const int SCORE_HIT   = 18;   // points added per motion edge
const int SCORE_DECAY = 1;    // points lost per second
```

---

## Debugging

Set `DEBUG_MODE = true` in the sketch to enable Serial output at 9600 baud. The monitor prints on PIR state changes and every 5 seconds:

```
Time: 42s | PIR: HIGH | State: LIGHTS_ON   | Score: 54 | Class: no     | LastMotion: 0s | Timeout: 28s (30s)
Time: 73s | PIR: LOW  | State: LIGHTS_ON   | Score: 21 | Class: no     | LastMotion: 31s | Timeout: NOW!
Time: 74s | PIR: LOW  | State: SHUTDOWN    | Score: 20 | Class: no     | LastMotion: 32s
```

Class Lock events are also printed:
```
*** CLASS LOCK ENGAGED *** (7 edges in 2 min)
*** CLASS LOCK RELEASED *** (12 min silence)
```

---

## Version History

| Version | Changes |
|---|---|
| v3.1 | Adaptive occupancy: rising-edge detection, confidence scoring, Class Lock |
| v3.0 | 8 new animations (Lightning, Tide, Eclipse, Firefly, Avalanche, Pendulum, Serpent, Nova) |
| v2.2.1 | Fixed RISE animation — bottom light (pDn) now included |
| v2.2 | Fixed animation variety using counter + random to guarantee cycling |
| v2.1 | Fixed scope bug in LIGHTS_ON preventing shutdown; reduced PIR warmup 60s → 10s |
| v2.0 | Non-blocking animations with millis(), PIR debouncing, failsafe, startup test |
