# Realtime basis windtunnel

## Uitgangspunt

Arduino Due **of** ESP32 + Raspberry Pi. De boardkeuze is nog open.
De besturingstaak en communicatie zijn in C geschreven bovenop FreeRTOS.
`arduino/` is de historische mapnaam; de kern hangt niet af van de Arduino IDE.
De primaire ontwikkelroute is nu [PlatformIO in VS Code](PLATFORMIO.md), met
Arduino als boardframework en dezelfde C/FreeRTOS-applicatiekern.

De deltarobot in `EMPD2026-DeltaRobot` is als architectuurreferentie gelezen:
`main.c` initialiseert hardware en scheduler, `ApplicationTasks.c` maakt taken
en queues en `ControlTask.c` beheert de toestand. Dit project gebruikt die
taakverdeling, maar heeft eigen windtunnelcode. Robotkinematica, motorpinnen,
homing en regelparameters zijn niet overgenomen. De referentie gebruikt een
ATSAM3X8E, ASF en FreeRTOS 10.4.6 met een tick van 1 ms.

## Datastroom

```text
Sensoren -> board_sample() -> ControlTask (100 Hz) -> board_fan_write() -> aandrijving
                                  |        ^
                           snapshotqueue  commandqueue
                                  v        |
                           CommunicationTask
                                  |        ^
                             UART / USB serial
                                  |        ^
                              Raspberry Pi
```

De ControlTask heeft prioriteit 3 en is de enige eigenaar van de regeltoestand
en normale ventilatoruitgang. Elke 10 ms: coherent sensorbeeld ophalen,
fouten/time-outs controleren, maximaal één commando verwerken, uitgang schrijven,
snapshot publiceren en watchdog voeden. `vTaskDelayUntil` houdt de periode vast.
Een trage sensor moet asynchroon in de hardwarelaag worden uitgelezen; de
regelcyclus mag daar niet op wachten. De sampletimestamp is de meettijd en mag
bij het teruggeven van een oude meting niet worden vernieuwd.

De CommunicationTask heeft prioriteit 1. Zij verstuurt iedere 100 ms een snapshot,
verwerkt begrensde invoer en bevestigt een commando pas nadat de ControlTask het
heeft verwerkt en de driver de uitgang heeft geaccepteerd. Queues dragen kopieën
over, zodat de Pi een coherent beeld krijgt. Eén commando tegelijk is toegestaan.
De ACK bevestigt een uitgangsopdracht, geen gemeten omwentelingssnelheid.

100 Hz is een instelbare startwaarde, nog niet onderbouwd met sensordatasheets of
een dynamisch model. Meet op het uiteindelijke board de looptijd, jitter,
stackruimte, sensorvertraging en seriële doorvoer onder belasting.

## Toestanden en fouten

`STOPPED=0`, `MANUAL=1`, `FAULT=2`. Opstart is STOPPED met 0%.
MANUAL is een directe uitgangsopdracht in procenten, **geen snelheidsregelaar**.
De voorlopige bovengrens is 30%; dit is geen gekalibreerde veilige bedrijfslimiet.

Bij ongeldige/verouderde metingen (>100 ms), onderbroken interlocks, ontbrekende
hardwarevrijgave, een controlestap na meer dan 20 ms, een driverfout of >=1000 ms
zonder geldig MANUAL-commando gaat de uitgang naar nul. FAULT blijft vergrendeld.
STOP werkt ook in FAULT en wist de fout niet. RESET werkt alleen bij herstelde
voorwaarden en geeft nooit vanzelf een niet-nul uitgang.

Foutcodes: 0 OK, 1 NOT_READY, 2 INTERLOCK, 3 SENSOR, 4 TIMEOUT,
5 TIMING, 6 OUTPUT. Sensor- en interlockvoorwaarden worden bij stilstand opnieuw
beoordeeld wanneer een start/reset wordt gevraagd.

De software kan een vastgelopen CPU of geblokkeerde driver niet zelfstandig
uitschakelen. Implementeer daarvoor `board_emergency_off`, een hardwarewatchdog
en de echte enable/interlockbedrading. Deze onderdelen zijn nog niet ingevuld.

## Hardwarelaag invullen

`arduino/include/board_port.h` is het contract. Er staan nu aparte voorbereide
implementaties in `arduino/ports/due/board_port.c` en
`arduino/ports/esp32/board_port.c`. Iedere functie bevat uitgecommentarieerde
hardwarestappen voor dat board. De genoemde `due_*` en `esp_*` helpers zijn
ontwerpvoorbeelden die nog moeten worden geïmplementeerd, geen SDK-functies.
Alle actieve implementaties gebruiken geen pinnen, melden ongeldige sensoren en
`ready=false`. Ook de UART is nog uitgeschakeld: de Pi kan nog niet verbinden.
`unconfigured` blijft beschikbaar als algemene fallback.

Nog nodig:

- Definitieve Due/ESP32-keuze en ontwikkelomgeving.
- Sensortypes, interfaces, samplefrequenties en kalibratie naar m/s en N.
- Tachometer/toerentalmeting als je werkelijk RPM wilt regelen; luchtstroomsnelheid
  regelen is een andere regelgrootheid. `fan_rpm` is optioneel met `rpm_valid`.
- Ventilator/frequentieregelaar en zijn interface (bijvoorbeeld PWM, 0–10 V of
  Modbus), enable-signaal en betekenis van 0%.
- Werkelijke deur/noodstopingangen, watchdog en elektrisch stopgedrag.

Voeg later een afzonderlijke `Regelaar.c/.h` toe die elke controlestap een
uitgang berekent uit setpoint, gemeten RPM (of gekozen luchtsnelheid) en vaste dt.
Breid daarna de toestand en het protocol uit voor closed-loop setpoints.
Identificeer eerst de ventilatordynamica; neem de deltarobotgains niet over.

## Integratie op de boards

**PlatformIO:** gebruik `platformio.ini` in de repositoryroot. Er zijn afzonderlijke
omgevingen voor Due en klassieke ESP32. Deze gebruiken een Arduino-entrypoint en
hebben Microchip Studio niet nodig. Onderstaande ASF/ESP-IDF-instructies beschrijven
de alternatieve, eerdere integratieroutes.

ESP32: er is een ESP-IDF-project in `arduino/ports/esp32`. Dit selecteert al de
eigen `esp32/board_port.c` met uitgecommentarieerde hardware. Implementeer de
helpers, vul de pinmapping in en selecteer de juiste chip voordat je de hardware
activeert. In een ingerichte ESP-IDF-shell: `idf.py build`. De scheduler draait
al wanneer `app_main` wordt aangeroepen. Dit target is nog niet met ESP-IDF gebouwd.

Due: `arduino/ports/due/main.c` is een ASF/FreeRTOS-entrypoint. Gebruik een
Due-project met dezelfde ASF/CMSIS/startup/linkerscript en FreeRTOS-kernel als de
deltarobotreferentie. Voeg de drie bestanden uit `arduino/src` en
`arduino/ports/due/board_port.c` toe, plus `arduino/include` als include-pad.
Compileer altijd precies één boardport om dubbele functies te voorkomen.
Gebruik de nieuwe main in plaats van
de robotmain; neem de robottaken en motorinitialisatie niet op. Configureer 1 ms
ticks, minstens vier prioriteiten, dynamic allocation en delay/delete-API's.
Schakel de ongebruikte runtime-statistieken uit of lever de bijbehorende timerhooks.
Printf moet floats ondersteunen (de robotprojectoptie `iprintf` is hiervoor
ongeschikt). Er is nog geen zelfstandig Due `.cproj` of gelinkte firmware-image.

De hardwaredrivers en een echte targetbuild volgen op de definitieve boardkeuze.
De hostbuild hieronder test al wel dezelfde C-kern die op beide boards draait.
