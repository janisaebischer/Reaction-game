# Reaction Game - ESP32 Project

Minimales ESP32-Projekt für ein Reaktionsspiel mit 15 Tastern, zufälligen Ziel-LEDs und vierstelliger Siebensegmentanzeige.

## Hardware
- ESP32 DevKit Board
- MCP23017-based I2C GPIO expander module for 15 buttons and 15 LEDs
- 4-digit TPIC6C596-based seven-segment display module
- 1x Ein/Aus-Schalter auf einem digitalen Eingang

## Quickstart

### Build
```bash
pio run -e reaction_game
```

### Upload

```bash
pio run -e reaction_game
```

## Spielablauf

Nach dem Einschalten zeigt die Anlage `00.00`.

1. Zwei Starttaster gedrückt halten für 3 Sekunden.
2. Danach blinkt die Anzeige.
3. Wenn beide Taster losgelassen werden, startet die Zeit und das Spiel.
4. Eine zufällige Anzahl Ziel-LEDs leuchtet.
5. Die gedrückten Ziele gehen aus, bis alle gefunden sind.
6. Danach bleibt die Endzeit stehen, bis ein neues Spiel gestartet oder die Anlage ausgeschaltet wird.
7. Beim Ausschalten gehen alle LEDs und die Anzeige aus.

## Verdrahtung

Die Standardbelegung ist in [include/settings.h](include/settings.h) konfiguriert.

- I2C SDA: GPIO 21
- I2C SCL: GPIO 22
- Display DATA: GPIO 18
- Display CLOCK: GPIO 19
- Display LATCH: GPIO 23
- Power switch: GPIO 27

Die 15 Taster hängen an der zweiten MCP23017-Hälfte, die 15 LEDs an der ersten.
Die Starttaster sind per Default Taster 0 und 1.

## Anpassungen

- **Button-Mapping**: in [include/settings.h](include/settings.h)
- **Anzeige-Reihenfolge**: in [include/settings.h](include/settings.h)
- **Zielanzahl / Blinkzeiten**: in [include/settings.h](include/settings.h)

## Struktur

```
src/
  main.cpp           - Reaction game controller
include/
  reaction_game.h    - Spielzustandsmaschine
  i2c_outputs.h      - MCP23017-Treiber
  seven_segment_display.h - 7-Segment-Treiber
```
