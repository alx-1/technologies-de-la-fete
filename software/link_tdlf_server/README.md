# *E X P E R I M E N T A L* 

*Tested with esp-idf [v4.0-1](https://github.com/espressif/esp-idf/releases/tag/v4.0-rc)*

## ABLETON LINK -> MIDI

Sending Midi Clock and Start / Stop (uart - midi connector) based on Ableton Link session.
WIP

## Building and Running the Example

* Setup esp-idf as described in [the documentation](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html)
* Run `idf.py menuconfig` and setup WiFi credentials under 
`Example Connection Configuration` + Serial flasher config -> Flash size (4 MB) + Partition Table == Custom partition table CSV
```
idf.py build
idf.py -p ${ESP32_SERIAL_PORT} flash
