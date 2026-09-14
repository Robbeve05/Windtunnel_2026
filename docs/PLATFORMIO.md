# VS Code + PlatformIO: Due en ESP32

Open de **repositoryroot** met `platformio.ini` in VS Code. Dit is
`05. Software/Windtunnel_2026`, dezelfde lokale checkout als in GitHub Desktop.
Maak geen nieuw leeg PlatformIO-project en kopieer de code niet naar een tweede map.

1. Installeer in VS Code de extensie **PlatformIO IDE**, uitgever PlatformIO.
2. Open deze repository via **File > Open Folder**.
3. Open PlatformIO (het mier-icoon), vervolgens **Project Tasks**.
4. Kies **due > General > Build** of **esp32 > General > Build**.
5. Alleen wanneer je het juiste board hebt aangesloten: kies onder diezelfde
   omgeving **Upload**. De Due-configuratie gebruikt de Programming USB-poort.

De eerste build downloadt automatisch platform, compiler en libraries.
PlatformIO beheert de gereedschappen; Microchip Studio is voor deze route niet nodig.

Vanuit een PlatformIO-terminal kan hetzelfde met:

```sh
pio run -e due
pio run -e esp32
```

`pio run` zonder `-e` bouwt beide. Upload is een afzonderlijke handeling:
`pio run -e due -t upload` of `pio run -e esp32 -t upload`.
Sluit een seriële monitor voordat je dezelfde poort vanuit de Pi-client gebruikt.

## Wat wordt gebouwd?

| Environment | Boardprofiel | Framework / FreeRTOS |
| --- | --- | --- |
| `due` | Arduino Due Programming Port | Arduino SAM + DueFreeRTOS 10.1.1, vastgezet op commit |
| `esp32` | Generieke klassieke ESP32 Dev Module | Arduino-ESP32 met ingebouwde FreeRTOS |

Het ESP32-profiel is een voorlopig **klassiek ESP32**-buildtarget. Controleer het
werkelijke board; een ESP32-S3/C3/C6 is niet hetzelfde target.

Beide builds gebruiken `arduino/platformio/main.cpp`, de drie gedeelde C-bestanden
uit `arduino/src` en precies één `board_port.c`. De ASF-main van de Due en de
ESP-IDF `app_main.c` worden door PlatformIO uitgesloten.

Op de Due start setup de scheduler expliciet. Op ESP32 draait deze al wanneer
setup wordt aangeroepen. `wt_rtos.h` kiest de juiste headerpaden.
De Due-library gebruikt Arduino-interrupthooks en een eigen FreeRTOS-configuratie;
de kernelversie verschilt van de 10.4.6 uit de deltarobot. De windtunnelkern blijft
hetzelfde. Linkerwrappers leiden de Due-failurehooks naar `board_emergency_off`.

Alle daadwerkelijke hardwarefuncties blijven uitgecommentarieerd:
`board_ready()` is false, de sensoren zijn ongeldig en de UART verstuurt nog niets.
Een geslaagde build maakt deze drivers niet automatisch werkzaam.

De uitgecommentarieerde Due-voorbeelden noemen nog ASF. De Arduino SAM-build
levert niet het volledige ASF-framework: implementeer deze helpers met Arduino
SAM-drivers/registers, of voeg de benodigde ASF-modules bewust toe.

## ESP-IDF als alternatief

Het eerdere standalone project onder `arduino/ports/esp32` blijft beschikbaar,
maar is niet de PlatformIO-route in deze repository. De beperking rond spaties
in ESP-IDF-projectpaden hoort bij die alternatieve build. Verplaats de repository
niet alleen vanwege die eerdere instructie als je deze Arduino/PlatformIO-route gebruikt.

## Bronnen

- [PlatformIO in VS Code](https://docs.platformio.org/en/latest/integration/ide/vscode.html)
- [Due-boardprofiel](https://docs.platformio.org/en/latest/boards/atmelsam/due.html)
- [Espressif32-platform](https://docs.platformio.org/en/latest/platforms/espressif32.html)
- [DueFreeRTOS-bron](https://github.com/bdmihai/DueFreeRTOS/tree/e6a3354cb0b337d8215ccf162e90a5fb9a50d241)
