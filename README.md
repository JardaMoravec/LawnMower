# LawnMower (IoT)

Firmware for the lawn mower project, built with **PlatformIO** and **Arduino** on the **LaskaKit ESP32-S3-DEVKit**.

## Hardware

| Item | Details |
|------|---------|
| Board | [LaskaKit ESP32-S3-DEVKit](https://github.com/LaskaKit/ESP32-S3-DEVKit) |
| Module | ESP32-S3-WROOM-1-N16R8 (16 MB flash, 8 MB OPI PSRAM) |
| Official board docs | [README (CZ)](https://github.com/LaskaKit/ESP32-S3-DEVKit/blob/main/README_CZ.md) · [Pinout](https://github.com/LaskaKit/ESP32-S3-DEVKit/blob/main/img/ESP32-S3-DEVKIT_pinout.png) |

The module datasheet in `datasheeds/` describes the **WROOM module**, not the dev board pinout or USB wiring. Use the LaskaKit repo for board-specific details.

## USB connection (important)

The dev kit has **two USB-C ports**. For this project, use only one of them:

| Port label on board | Chip / role | Use for this project |
|---------------------|-------------|-------------------|
| **UART** | CH9102F USB–UART bridge | **Yes** — upload, power, Serial monitor |
| Other USB-C | ESP32-S3 native USB | **No** — not used for normal dev workflow |

```
PC  ──USB-C cable──►  [ UART ]  LaskaKit ESP32-S3-DEVKit
                         │
                    CH9102F ──► ESP32-S3 UART0 (Serial)
```

- Plug the cable into the port **labeled UART** and leave it there.
- **Upload** and **Serial monitor** both use the same port (same COM port in Windows).
- You do **not** need to switch cables between upload and monitor.
- `Serial` in code goes through UART (same as [LaskaKit’s examples](https://github.com/LaskaKit/ESP32-S3-DEVKit/tree/main/SW)), not native USB CDC.

### Finding the COM port

```bash
pio device list
```

Look for **CH9102** (USB VID:PID `1A86:55D4` or `1A86:55D3`). PlatformIO auto-detects this port; you do not need to set `upload_port` manually.

## Software setup

1. Install [PlatformIO](https://platformio.org/) (VS Code / Cursor extension or CLI).
2. Clone this repo and open the project folder.
3. Connect the board via the **UART** USB-C port.

Default PlatformIO environment: `laskakit-esp32-s3-devkit` (see `platformio.ini`).

## Build & upload

```bash
# Build
pio run

# Upload firmware
pio run -t upload

# Serial monitor (115200 baud)
pio device monitor
```

In VS Code / Cursor you can use the PlatformIO sidebar: **Upload** and **Monitor**.

### Recommended workflow

1. Close any open Serial Monitor before uploading.
2. Run **Upload**.
3. Open **Monitor** after upload completes.

If upload fails with “port busy”, another program (or a stuck monitor) is holding the COM port. Close other serial terminals and try again.

## Project layout

```
src/
  main.cpp                 # Application entry
  modules/
    motorController.*      # Motor driver
    wifiConnect.*          # Wi-Fi setup
boards/
  laskakit-esp32-s3-devkit.json   # Board definition (flash, PSRAM, upload port)
platformio.ini           # PlatformIO config
```

## Troubleshooting

| Problem | What to try |
|---------|-------------|
| Upload fails, “port busy” | Close Serial Monitor; unplug USB, wait 5 s, replug into **UART** port |
| Wrong / no COM port | Confirm cable is in **UART**, not native USB; run `pio device list` |
| Monitor shows nothing | Upload first; wait 1–2 s after upload; baud rate is **115200** |
| Native USB port (COM6, `303A:1001`) | Ignore for this project — firmware uses UART Serial |

## References

- [LaskaKit ESP32-S3-DEVKit (GitHub)](https://github.com/LaskaKit/ESP32-S3-DEVKit)
- [LaskaKit product page](https://www.laskakit.cz/laskakit-esp32-s3-devkit/)
- [ESP32-S3-WROOM-1 module datasheet](https://www.espressif.com/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
