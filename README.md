# Theremin

Dit project maakt geluid op basis van de afstand van je hand tot een sensor.
Een hand dichter bij de sensor geeft een hogere toon. Met een potmeter regel je
het volume. Twee knoppen veranderen hoeveel metingen het mediaanfilter gebruikt;
het 7-segmentdisplay laat die filtergrootte zien.

Het is een project voor HBO-ICT Microcontrollers bij Windesheim. De software is
geschreven in bare-metal C voor de ATmega328P op een Arduino Uno: registers worden
rechtstreeks ingesteld, zonder Arduino-framework. PlatformIO verzorgt het bouwen.

## Hardware en bediening

De Uno wordt gebruikt met een Game Shield en de onderstaande onderdelen.

| Onderdeel | Aansluiting | Functie |
| --- | --- | --- |
| HC-SR04 | D9/PB1: TRIG, D8/PB0: ECHO/ICP1 | Afstand meten |
| Potmeter | ADC0/PC0 | Volume regelen |
| Buzzer | D3/PD3/OC2B | Toon afspelen |
| Plusknop | D4/PD4 | Filter twee metingen groter maken |
| Minknop | D5/PD5 | Filter twee metingen kleiner maken |
| PCF8574 met 7-segmentdisplay | SDA/PC4 en SCL/PC5 | Filtergrootte tonen via I2C |
| Ingebouwde LED | D13/PB5 | Aangeven dat de afstand geldig is |

De knoppen gebruiken interne pull-ups: los is HIGH en ingedrukt is LOW.
Een stabiele druk geeft één stap; vasthouden herhaalt niet. Tegelijk indrukken
geeft geen stap.

Het filter start op 5 en ondersteunt 1, 3, 5, 7, 9, 11, 13 en 15 metingen.
Het display toont dit hexadecimaal: 1, 3, 5, 7, 9, b, d en F.
De software zoekt de PCF8574 bij het opstarten op adressen 0x20 tot en met 0x27.

## Hoe de onderdelen samenwerken

1. Timer1 maakt de triggerpuls en legt de tijdstippen van beide ECHO-flanken vast.
   Het verschil is de lengte van de echopuls. Daaruit volgt de afstand in cm.
2. De hoofdloop accepteert afgeronde afstanden van 2 tot en met 65 cm. Het
   mediaanfilter neemt de middelste waarde van de laatste geldige metingen.
   Zo wordt bijvoorbeeld de reeks 10, 60, 11 cm gefilterd naar 11 cm.
3. De gefilterde afstand bepaalt de gewenste frequentie: `1400 - 18 * afstand`.
   Timer0 schakelt de toon iedere halve periode aan of uit. Timer2 maakt het
   PWM-signaal op D3; de potmeter bepaalt de pulsbreedte en daarmee het volume.
4. Een pin change interrupt meldt dat een knop is veranderd. De hoofdloop wacht
   ongeveer 20 ms zonder nieuwe verandering voordat de druk wordt verwerkt.
   Daarna worden de filtergrootte en het display bijgewerkt.

Bij een ongeldige afstand of een echo-timeout gaat de LED uit en klinkt de
laagste toon. De ongeldige meting wordt niet aan het filter toegevoegd.

## Bouwen en testen

Installeer PlatformIO Core of de PlatformIO-extensie voor VS Code. Voor tests op
de pc zijn GCC en G++ op `PATH` nodig, met hun runtime-DLL's op Windows.
PlatformIO haalt de AVR-toolchain en Unity op. Voer vanuit de projectmap uit:

```text
pio run -e uno
pio test -e native -v
```

Het eerste commando bouwt de firmware. Het tweede voert 35 Unity-tests op de pc
uit, zonder aangesloten Arduino. Deze commando's uploaden geen firmware.
Alleen `pio run` gebruiken bouwt standaard de Uno-omgeving.

Een extra statische controle op mogelijke codeproblemen is beschikbaar via:

```text
pio check -e uno --skip-packages --fail-on-defect low --fail-on-defect medium --fail-on-defect high
```

Op Windows kan `$env:PYTHONUTF8 = '1'` nodig zijn bij console-encodingproblemen.
De laatst uitgevoerde controle gaf 35 geslaagde tests en een geslaagde AVR-build.
De details en beperkingen staan in het [testrapport](docs/TESTRAPPORT.md).

## Belangrijke keuzes in de code

- **Korte ISR's:** interrupts leggen meetwaarden vast of zetten een vlag. De
  hoofdloop doet het filteren, geheugen reserveren en bijwerken van het display.
  `volatile` zorgt dat de compiler de gedeelde variabele telkens echt leest of
  schrijft. Een atomic block voorkomt onderbreking tijdens een overdracht.
- **Afstand in 32 bits berekenen:** op de AVR is `int` 16 bits. De optelling bij
  het afronden wordt daarom eerst verbreed, zodat een grote tickwaarde niet omloopt.
- **Timer0 op een vast moment aanpassen:** de nieuwe frequentie wordt pas bij een
  compare match ingesteld. Midden in een periode een lagere eindwaarde schrijven
  kan anders een compare match overslaan.
- **Mediaanfilter met dynamisch geheugen:** `malloc()` reserveert de buffer.
  Bij vergroten blijven bestaande metingen bewaard; bij verkleinen de nieuwste.
  Eerst wordt de nieuwe buffer aangevraagd, daarna pas de oude vrijgegeven. Bij
  `NULL` blijft de oude buffer bruikbaar. Zonder buffer geeft het filter de
  invoer terug, toont het display 0 en kan plus opnieuw grootte 1 proberen.
- **Dezelfde logica in firmware en tests:** de native tests bouwen de echte
  filter-, reken- en debouncecode. Alleen `malloc()` en `free()` van het filter
  krijgen in de tests extra controles en een mogelijkheid om een fout na te bootsen.

## Wegwijs in de bestanden

| Bestand/map | Inhoud |
| --- | --- |
| `src/main.c` | Sensor, bijbehorende ISR's en hoofdloop |
| `src/buzzer.c`, `src/volume.c`, `src/display7.c` | Hardware aansturen |
| `src/filter_controls.c`, `src/button_debounce.c` | Knoppen uitlezen en dender onderdrukken |
| `src/median_filter.c`, `src/theremin_math.c` | Filter en berekeningen |
| `include/` | Functiedeclaraties en afspraken voor het gebruik |
| `test/` | Unity-tests en geheugencontrole voor die tests |
| `docs/TESTRAPPORT.md` | Automatische tests, praktijktests, technische onderbouwing en openstaande controles |

## Beperkingen en openstaande controles

Tests op de pc controleren geen elektrische signalen of echt interruptgedrag.
De student heeft de gewijzigde firmware op 25 oktober 2026 geüpload en zes
praktijktests als PASS gemeld. De resultaten en nog openstaande gespecialiseerde
controles staan in het [testrapport](docs/TESTRAPPORT.md#handmatige-praktijktests).
De sensor en afronding beperken de afstandsnauwkeurigheid. De timer werkt met
hele ticks; de werkelijke tonen lopen daardoor ongeveer van 231 tot 1359 Hz.
Een groter filter onderdrukt uitschieters beter, maar reageert ook langzamer.
Tijdens het vullen gebruikt een even aantal metingen de onderste middelste waarde.

Een display dat bij opstarten ontbreekt, wordt later niet opnieuw gezocht.
I2C-wachten heeft een timeout. Langdurig gebruik, geheugenfragmentatie en de
stackmarge op de Uno zijn nog niet fysiek onderzocht.

## Werkwijze

Bij de code, tests en documentatie in deze review is AI-assistentie (Codex)
gebruikt. Builds en automatische tests zijn tijdens de eerdere reviews lokaal
uitgevoerd door de assistent. De handmatige praktijktests zijn door de student
uitgevoerd en gemeld. Er zijn geen schoolspecifieke regels
over AI-gebruik meegeleverd. Controleer daarom bij de opleiding welke vermelding
nodig is en welke hulp is toegestaan. De code kunnen uitleggen blijft onderdeel
van de beoordeling.

Het [testrapport](docs/TESTRAPPORT.md) is het centrale testdocument. De
testcommando's geven uitvoer in de terminal; aparte log- of XML-bestanden zijn
niet nodig. PlatformIO bewaart buildproducten en testprogramma's onder `.pio/`,
dat buiten Git blijft.
