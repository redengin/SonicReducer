Parametric Speaker System
================================================================================
```mermaid
zenuml
    Audio -> Modulator
    Modulator -> Amplifier
    Amplifier -> "Speaker(s)"
```
`Audio` signal in human hearing range.

`Modulator` encodes `audio` by varying a carrier wave's frequency in proportion to the message signal's amplitude, keeping amplitude constant but changing frequency to represent sound's pitch and loudness.

`Amplifier` powers the `Speaker(s)` per the modulated signal.

`Speaker(s)` are ultrasound transmitters, carrier wave > 20KHz.

Build of Materials
================================================================================
Speaker - ultrasound transmitter
--------------------------------------------------------------------------------
* [TCT40-16T/R](https://docs.sparkfun.com/SparkFun_Ultrasonic_Distance_Sensor-Qwiic/assets/component_documentation/TCT40-16-T-R.pdf)
    * Frequency - 40 KHz
    * Max Voltage - 80 V

Amplifier - modulated signal at higher voltage
--------------------------------------------------------------------------------
Higher voltage increases loudness - but too much voltage will destroy the speaker.

Amplifiers heat up as you transfer power through them.

* [L298N](https://www.google.com/url?sa=t&source=web&rct=j&opi=89978449&url=https://www.st.com/resource/en/datasheet/l298.pdf)
    * Max Voltage - 50 V
    * Current
        * max - 3 A
        * Typical - 2 A
    * Frequency
        * Typical - 20KHz
        * Max - 40KHz

Modulator and Audio
--------------------------------------------------------------------------------
This project leverages embedded hardware to perform this function
(see target docs for more details).
* Most platforms provide a voltage regulator that supports voltages up to 10 V.

Guidance
--------------------------------------------------------------------------------
If the voltage level of your amplifier vs SoC mismatch, you can use multiple
power sources coupled to a common ground.