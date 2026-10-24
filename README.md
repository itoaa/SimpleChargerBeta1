# Simple Charger Beta 1

ATmega328P firmware at 16 MHz. One main loop, no RTOS.

Build from the `SimpleChargerBeta1` directory:

```
make all
```

`avr-g++` and `avr-libc` must be on `PATH`. When they are not installed locally, GitHub Actions is the compile check.

TunerStudio signature, including the two trailing spaces:

`"MSII s-Charger B-1  "`

Command `F` replies `001`.

`SimpleChargerBeta1/tunerStudio.ini` is temporary. It will be replaced when the original ini is found. Do not change the realtime or page layout and this ini independently.
