# Retro-Go Doom on the LilyGO T-Display-S3 LCD

This document records the working configuration for running Doom on the
classic LilyGO T-Display-S3 LCD board with Retro-Go/PrBoom.

It is not for the T-Display-S3 AMOLED board. The two boards share the ESP32-S3
family but use different displays and buses.

## Confirmed hardware

- Board: LilyGO T-Display-S3 classic LCD
- MCU: ESP32-S3R8
- Flash: 16 MB
- PSRAM: 8 MB
- Display: ST7789
- Panel size: 170 x 320
- Retro-Go logical canvas: 320 x 170
- Interface: 8-bit Intel 8080 parallel/I80
- Target in the build system: `t-display-s3-lcd`

## GPIO configuration

| Function | GPIO |
|---|---:|
| CS | 6 |
| DC | 7 |
| WR | 8 |
| RD | 9 |
| RESET | 5 |
| Backlight | 38 |
| D0-D3 | 39-42 |
| D4-D7 | 45-48 |
| BOOT button | 0 |
| START/other button | 14 |

The RD line is configured as an active-high output. GPIO 38 is backlight and
must be driven high.

## Display initialization

The I80 bus is configured at 16 MHz with 20 transfer queue entries and an
ESP-IDF ST7789 panel. The ST7789 initialization applies the command sequence
from the board reference implementation before enabling the panel.

The working geometry is:

```text
width:       320
height:      170
swap_xy:     true
mirror_x:    false
mirror_y:    true
gap:         x=0, y=35
inversion:   true
```

The display canvas is treated as one full frame in logical 320 x 170
coordinates. Partial-window byte commands are not mixed with ESP-LCD
transformations because that causes wrong offsets and fragmented output.

## Frame transfer

The renderer produces RGB565 Doom frames and sends the complete 320 x 170
frame with `esp_lcd_panel_draw_bitmap()`. The I80 implementation uses one
aligned DMA buffer and waits for the DMA callback before the buffer is reused.
This prevents stale pixels and transfer races.

Final color behavior uses:

- RGB element order in the ST7789 panel configuration.
- No extra I80 `swap_color_bytes` operation.
- The Doom palette already converted and byte-ordered before transfer.

Do not enable both palette byte swapping and ESP-LCD bus byte swapping. That
path duplicates the 16-bit endianness conversion and changes the colors.

## WAD and automatic Doom demo

PrBoom is configured to boot the `doom1.wad` from a dedicated flash partition
named `doomwad` and start the first demo automatically.

The Doom launcher arguments are:

```text
-save <save-path>
-iwad doom1.wad
-nomusic
-playdemo demo1
```

The WAD partition size is taken dynamically from
`esp_partition_t->size`. Do not use a hardcoded WAD size, because the WAD
and partition alignment can change.

The launcher switches directly to the `prboom-go` application after boot.

## Frame rate

The display loop runs at approximately 35 FPS. Retro-Go frameskip remains
enabled for the ESP32-S3 busy periods. The serial log should report:

```text
G_DoPlayDemo: playing demo with doom/doom2 v1.9 compatibility
FPS:35
```

The expected FPS values may vary by a few frames, but the demo must advance
and the screen must update continuously.

## Complete setup on Windows

Tools used for the verified setup:

- Git for Windows
- Python 3
- `pyserial`
- Espressif ESP-IDF 5.1 at `C:\Espressif\esp-idf`
- USB serial port `COM9`
- A valid `doom1.wad`

Install Python tooling:

```powershell
python -m pip install --upgrade pip
python -m pip install pyserial
```

Install Espressif's Windows ESP-IDF 5.1 environment first. Clone the fork and
enter the Doom branch:

```powershell
Set-Location C:\Users\$env:USERNAME\Documents
git clone https://github.com/jfcbxp/retro-go.git
Set-Location C:\Users\$env:USERNAME\Documents\retro-go
git checkout feature/t-display-s3-lcd-doom
```

Load ESP-IDF in the current PowerShell session:

```powershell
$env:IDF_PATH="C:\Espressif\esp-idf"
$env:IDF_TARGET="esp32s3"
. C:\Espressif\esp-idf\export.ps1
idf.py --version
```

If ESP-IDF is installed elsewhere, set `IDF_PATH` accordingly. Find the board
serial port:

```powershell
Get-PnpDevice -PresentOnly | Where-Object {
  $_.InstanceId -match 'VID_303A' -or $_.FriendlyName -match 'USB.*Serial'
}
```

### Erase and flash from scratch

This removes the Agent Needs You firmware and every retained flash partition.
Replace `COM9` with the port assigned by Windows.

```powershell
python C:\Espressif\esp-idf\components\esptool_py\esptool\esptool.py `
  --chip esp32s3 `
  --port COM9 `
  erase_flash
```

Clean, build, install the WAD partition and flash the launcher plus PrBoom:

```powershell
python .\rg_tool.py clean launcher prboom-go --target t-display-s3-lcd
python .\rg_tool.py install launcher prboom-go `
  --target t-display-s3-lcd `
  --port COM9 `
  --baud 1152000 `
  --wadfile C:\path\to\doom1.wad
```

Set COM9 to the serial port assigned by Windows if it differs.

## Serial diagnostics

A successful startup includes:

```text
rg_display_init: Display ready.
update_viewport_scaling: 320x170@1.882 => 320x170@1.882
IWAD found: doom1.wad
Playing demo demo1.lmp
G_DoPlayDemo: playing demo with doom/doom2 v1.9 compatibility
```

The Doom framebuffer is intentionally rendered without scaling:

```text
left:0 top:0 step_x:1.00 step_y:1.00
```

## Troubleshooting

### Purple or green artifacts

- The display was using SPI, AMOLED, or QSPI settings.
- The gap or mirror configuration was wrong.
- The frame buffer height or window height was wrong.
- ESP-LCD and raw ST7789 window commands were mixed.
- DMA buffers were reused before transfer completion.
- RGB565 bytes or the palette were being swapped twice.

### Wrong colors

Start with RGB element order, no extra I80 byte swapping, and verify the
palette conversion. Change only one color setting at a time.

### WAD not found

Use the dedicated `doomwad` partition, not a missing FAT/VFS image. The
partition table should contain:

```text
launcher
prboom-go
doomwad
```

The Doom entry point reads the data directly through an ESP-IDF partition
memory map.

### Menu only or fragmented images

Do not replace this driver's full-frame transfer with raw 0x2A/0x2B/0x2C
commands without also accounting for the ESP-LCD transform. Use
`esp_lcd_panel_draw_bitmap()` for each complete frame.
