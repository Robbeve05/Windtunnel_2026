# Windtunnel 2026

Realtime softwarebasis voor Arduino Due of ESP32 met een Raspberry Pi.
Taakstructuur geïnspireerd op het EMPD2026-deltarobotproject.

## Wat staat klaar?

- Boardonafhankelijke C-kern met STOPPED, MANUAL en FAULT.
- FreeRTOS-meet/controltaak op 100 Hz en aparte Pi-communicatietaak.
- Sensorbeeld voor twee luchtsnelheden, lift, drag en optioneel ventilator-RPM.
- Begrensde handmatige uitgang, communicatietime-out en vergrendelde fouten.
- Serieel WT1-protocol en Python-client voor de Raspberry Pi.
- C-tests, een eenvoudige ventilatorsimulatie en Python-protocoltests.

Dit is een geteste applicatiebasis, nog geen op hardware gevalideerde regeling.
Board, sensoren en aandrijving moeten nog definitief worden gekozen. De
meegeleverde hardwarelaag blijft uitgeschakeld. Een PI/PID-regelaar is nog niet
geïmplementeerd; MANUAL stuurt een uitgangspercentage, geen toerentalsetpoint.

## Bestanden

**Ontwikkelen in VS Code:** gebruik [PlatformIO voor Due en ESP32](docs/PLATFORMIO.md).
Open deze repositoryroot; `platformio.ini` bevat beide buildomgevingen.

| Map | Functie |
| --- | --- |
| `arduino/src` | Kern, protocol en FreeRTOS-taken voor beide boardopties |
| `platformio.ini` | Build- en uploadconfiguratie voor `due` en `esp32` |
| `arduino/platformio` | Gezamenlijk Arduino/FreeRTOS-entrypoint voor PlatformIO |
| `arduino/include/board_port.h` | Interface voor de echte hardwaredrivers |
| `arduino/ports` | Due- en ESP32-hardwarelagen met uitgecommentarieerde voorbeelden, startup en algemene fallback |
| `raspberry_pi` | Seriële Python-client |
| `tests` | Gedragstests en compilatieconfiguratie |
| `docs` | Architectuur, integratiestappen en protocol |

## Testen zonder hardware

Met CMake en een C11-compiler:

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
python -m unittest discover -s tests -p "test_*.py" -v
```

De C-test simuleert tien seconden aan 100 Hz met een eenvoudig ventilatormodel.
Dit controleert softwaregedrag, geen werkelijke ventilatordynamica of RTOS-jitter.

## Raspberry Pi, na implementatie van de boardport

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install -r raspberry_pi/requirements.txt
python raspberry_pi/windtunnel_link.py --port /dev/serial/by-id/JOUW_CONTROLLER
```

Standaard wordt STOP gestuurd en daarna alleen gemonitord. Een handmatige
uitgangstest kan met `--manual 10`; Ctrl+C probeert STOP. Na een herstelde fout
kun je expliciet `--reset` meegeven. De controller moet lokaal vrijgegeven zijn.

Lees [architectuur en boardintegratie](docs/ARCHITECTUUR.md),
[protocol](docs/PROTOCOL.md) en [validatie](docs/VALIDATIE.md).

Deze bestanden staan in **05. Software/Windtunnel_2026**. De gelijknamige kopie
onder **Software/Windtunnel_2026** is een andere lokale checkout.
