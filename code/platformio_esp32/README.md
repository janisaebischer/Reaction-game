# Reaction Game - ESP32 Project

Minimales ESP32-Projekt für ein Reaktionsspiel. Der ESP32 läuft als Access Point und serviert eine einfache Webseite.

## Hardware
- ESP32 DevKit Board

## Quickstart

### Build
```bash
pio run -e reaction_game
```

### Upload (Automatische Auswahl)
Der einfachste Weg: **Terminal → Run Task…** → **PlatformIO: Upload (choose method)**

Das Script fragt dich, ob du via Serial (USB) oder OTA (WiFi) hochladen möchtest:
```
1) Serial (USB) - Upload Firmware + Files
2) OTA (WiFi)   - Upload Firmware only
```

### Upload manuell (Terminal)

**Serial (USB) - Firmware + Dateien:**
```bash
pio run -e reaction_game -t uploadfs && pio run -e reaction_game -t upload
```

**OTA (WiFi) - nur Firmware:**
```bash
pio run -e ota -t upload --upload-port=ReactionGame.local
```

### VS Code Tasks
Alternative: **Terminal → Run Task…**
- **PlatformIO: Upload (choose method)** ← Empfohlen!
- PlatformIO: Upload Firmware
- PlatformIO: Upload Files (SPIFFS)
- PlatformIO: Upload All (Files + Firmware)
- PlatformIO: Upload via OTA (WiFi)

## Verbindung

Nach dem Upload:
1. WiFi-Netzwerk suchen: `ReactionGame-AP`
2. Im Browser öffnen: `http://192.168.4.1`
3. Seite sollte den Titel "Reaktionsspiel" anzeigen

## OTA (Over-The-Air Updates)

Der ESP32 lauscht auf OTA-Requests auf **Port 3232**. Nach dem ersten seriellen Upload:
1. Mit `ReactionGame-AP` verbinden (oder `ReactionGame.local` über mDNS wenn möglich)
2. Task **PlatformIO: Upload (choose method)** ausführen und **Option 2** wählen
   
   Oder manuell im Terminal:
   ```bash
   pio run -e ota -t upload --upload-port=ReactionGame.local
   pio run -e ota -t upload --upload-port=192.168.4.1
   ```

## Struktur

```
src/
  main.cpp           - ESP32 Access Point + Webserver + OTA
data/
  index.html         - Testseite
scripts/
  select_upload.ps1  - Interaktives Upload-Auswahlmenü
```

## Anpassungen

- **AP-SSID**: in `src/main.cpp` ändern (Zeile 8)
- **Webseite**: `data/index.html` bearbeiten
