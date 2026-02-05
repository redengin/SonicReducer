Parametric Speaker System
================================================================================
`Speakers` are ultrasound transmitters designed to transmit at 40 KHz (`carrier frequency`).

`Speaker` loudness is voltage dependent - the more voltage the louder the speaker.
To send the modulated signal to a speaker, you'll need voltage amplification.

### Modulation for Ultrasonic Speaker
https://en.wikipedia.org/wiki/Sound_from_ultrasound#Modulation_scheme
> ... distorting effects may be better mitigated by using another modulation scheme that takes
> advantage of the differential squaring device nature of the nonlinear acoustic effect.
> Modulation of the second integral of the square root of the desired baseband audio signal,
> without adding a DC offset, results in convolution in frequency of the modulated square-root
> spectrum, half the bandwidth of the original signal, with itself due to the nonlinear channel
> effects. This convolution in frequency is a multiplication in time of the signal by itself, or
> a squaring. This again doubles the bandwidth of the spectrum, reproducing the second time
> integral of the input audio spectrum. The double integration corrects for the -ω² filtering
> characteristic associated with the nonlinear acoustic effect. This recovers the scaled
> original spectrum at baseband. 


Amplification
================================================================================

Amplifier Based Design
--------------------------------------------------------------------------------
```mermaid
flowchart LR
    Audio["Modulated Audio"]    .->     DAC
    DAC                         .->     Amplifier
    Amplifier                   .->     Speakers

```

`Amplifiers` drive the speakers at a higher voltage (to increase loudness) per the analog
voltage input.

The `DAC` produces the voltages to drive analog voltage per the `Modulated Audio`.

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
