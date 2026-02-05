Parametric Speaker System
================================================================================
`Speakers` are ultrasound transmitters designed to transmit at 40 KHz (`carrier frequency`).

`Speaker` loudness is voltage dependent - the more voltage the louder the speaker.
To send the modulated signal to a speaker, you'll need voltage amplification.

Speaker Voltage Amplification
================================================================================

H-Bridge Based Design
--------------------------------------------------------------------------------
```mermaid
flowchart LR
    Audio["Modulated Audio"]    .->     H-Bridge
    H-Bridge                    .->     Speakers
```

The `H-Bridge` drives the speakers at a higher voltage (to increase loudness)
per two logic inputs (IN1, IN2) - driving the speaker at a VS(output voltage level)
polarity.

| IN1   | IN2   | Output to Speaker |
|-------|-------|-------------------|
| true  | false | (+) Positive      |
| false | true  | (-) Negative      |
| IN2   | IN1   | (ground) 0        |

Example Bill of Materials (BOM)
================================================================================
[Cheap Parts](demo_recipe.md) - focus on low cost
