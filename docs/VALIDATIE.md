# Validatie van de basis

De tests werken zonder aangesloten machine:

- C11-hostbuild met MSVC, warningniveau /W4.
- ARM Cortex-M3 syntaxcontrole met arm-none-eabi-gcc, -Wall -Wextra -Werror:
  kern, protocol, FreeRTOS-taken en de placeholderport. De taakcontrole gebruikte
  de werkelijke FreeRTOS 10.4.6-headers uit het lokale deltarobotproject.
- C-gedragstests: opstart, STOP, expliciete reset, interlockfout, timeout,
  ongeldige/verouderde sensoren, uitgangslimiet, vertraagde cyclus, timerrollover,
  commandoparser en begrensde formattering.
- Simulatie: 1000 cycli met 10 ms tijdstappen, commands iedere 200 ms,
  illustratief eersteordemodel voor de ventilator.
- Python-unittests voor telemetry, ACK-correlatie, afgewezen commands,
  ongeldige waarden en ontbrekende antwoorden.

Uitgevoerd op 14 september 2026: C-hosttests en simulatie geslaagd, alle zeven
Python-tests geslaagd en ARM-syntaxcontrole geslaagd. Geen targetfirmware gelinkt
of geflasht en geen machine aangestuurd.

Een hosttest bewijst geen realtimegedrag. Nog te doen op het gekozen board:
volledig compileren/linken, driver- en seriële integratietest, meetkalibratie,
looptijd/jitter/stack meten, kabelverlies en reset testen, foutgedrag van de
aandrijving vaststellen en pas daarna regelaarontwerp en afstelling.

De testconfig in `tests/freertos_config` dient alleen om de taakcode tegen de
Cortex-M3 FreeRTOS-headers te controleren, niet als complete boardconfiguratie.
