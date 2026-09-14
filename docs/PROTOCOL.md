# WT1: één controller naar Raspberry Pi

115200 baud, 8N1, ASCII, één regel per bericht, maximaal 255 bytes inclusief LF.
Geen debugtekst op deze verbinding. De Pi heeft maximaal één verzoek uitstaan;
wacht op de ACK voordat je het volgende stuurt. IDs zijn willekeurige uint32
correlatiecodes; zij zijn geen authenticatie of bescherming tegen replay.

```text
WT1,C,123,STOP
WT1,C,124,RESET
WT1,C,125,MANUAL,20.000
WT1,A,125,1,1,20.000
```

Command: `WT1,C,id,STOP|RESET|MANUAL[,percent]`.
ACK: `WT1,A,id,accepted,state,output_percent`.
`accepted` is 0 of 1. Ongeldige syntax geeft `WT1,E,BAD_COMMAND` en vernieuwt
de watchdog niet. MANUAL 0 wordt behandeld als STOP. RESET vraagt herstelde
sensoren/interlocks en een geconfigureerde hardwarelaag.

Elke 100 ms verschijnt:

```text
WT1,S,100,1,0,20.000,1,1,1,4.2000,3.8000,-0.3000,0.1000,0,0.00
```

Velden in volgorde: prefix, S, sampled_ms, state, fault, output_percent, valid,
door_closed, estop_ok, air_before_mps, air_after_mps, lift_n, drag_n, rpm_valid,
fan_rpm. Tijd is uint32 milliseconden sinds boardstart, met rollover. Een RPM van
nul met rpm_valid=0 betekent geen geldige toerentalmeting. Gebruik ongeldige data
niet voor regeling of analyses.

De Pi herhaalt MANUAL iedere 200 ms; de controller stopt zelfstandig na 1000 ms
zonder geldig commando. Bij fouten probeert de Pi STOP; de controller blijft
zelf verantwoordelijk voor het stopgedrag als de verbinding is verdwenen.

Dit is een nieuwe verbinding voor één gecombineerde realtimecontroller.
De bestaande webapp in `Software/chat-version` verwacht een ander JSON-protocol
met twee seriële apparaten. Die webapp is dus nog niet aangesloten op deze basis.
Gebruik voor deze firmware de meegeleverde `raspberry_pi/windtunnel_link.py`;
integratie met de webapp vereist later een WT1-adapter.
