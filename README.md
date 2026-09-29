# ESP32 Adaptive Smart Lamp

An ESP32 smart lamp project with automatic and manual control. In AUTO mode, a photoresistor measures the surrounding light and the ESP32 adjusts the lamp output. In MANUAL mode, you control it from a browser page.

## Components

- ESP32 DevKit V1
- Photoresistor sensor module
- Yellow LED
- 220 Ω resistor
- Wokwi simulator

## Wiring

| Component pin | Connect to |
|---|---|
| Photoresistor VCC | ESP32 3V3 |
| Photoresistor GND | ESP32 GND |
| Photoresistor AO | ESP32 GPIO34 (D34) |
| ESP32 GPIO18 (D18) | 220 Ω resistor |
| Other resistor pin | LED anode (A) |
| LED cathode (C) | ESP32 GND |

The photoresistor’s **DO** pin is not used.

## Features

- AUTO and MANUAL modes
- Live photoresistor readings
- Browser-based lamp controls
- Serial output in the Wokwi terminal

## Run in VS Code

1. Open the project folder in VS Code.
2. Build the project with PlatformIO.
3. Start the Wokwi simulator.
4. Open `http://localhost:8181` for the control page.

The browser page and simulator require the project’s `wokwi.toml` port forwarding configuration.

## Project files

- `src/main.cpp` — ESP32 firmware and web server
- `diagram.json` — Wokwi components and wiring
- `platformio.ini` — PlatformIO board and framework settings
- `wokwi.toml` — Wokwi firmware and network settings
