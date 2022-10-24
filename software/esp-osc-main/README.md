# esp-osc

[![Test](https://github.com/256dpi/esp-osc/actions/workflows/test.yml/badge.svg)](https://github.com/256dpi/esp-osc/actions/workflows/test.yml)
[![Release](https://img.shields.io/github/release/256dpi/esp-osc.svg)](https://github.com/256dpi/esp-osc/releases)

**OSC component for esp-idf projects based on the [tinyosc](https://github.com/mhroth/tinyosc) library**

This component bundles the tinyosc library and provides a simple API for sending OSC messages.

## Installation

You can install the component by adding it as a git submodule:

```bash
git submodule add https://github.com/256dpi/esp-osc.git components/esp-osc
git submodule update --init --recursive
```

## Example

An example can be found here: https://github.com/256dpi/esp-osc/blob/master/test/main/main.c.
