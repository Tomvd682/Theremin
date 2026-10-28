# Theremin

Dit is mijn project voor HBO-ICT Microcontrollers bij Windesheim.
De afstand van je hand tot een sensor bepaalt de toonhoogte van een buzzer.
Met een potmeter regel je het volume. Twee knoppen stellen het mediaanfilter in.
Het 7-segmentdisplay op het Game Shield laat de filtergrootte zien.

Het 16x2 LCD toont de afstand en de berekende frequentie.
De basiswerking heb ik op 28 oktober 2026 met acht praktijktests gecontroleerd.
De software gebruikt bare-metal C op een Arduino Uno (ATmega328P), zonder Arduino-framework.

## Hardware en aansluitingen

| Onderdeel | Aansluiting | Functie |
| --- | --- | --- |
| HC-SR04 | D9/PB1: TRIG, D8/PB0: ECHO | Afstand meten |
| Potmeter | A0/ADC0/PC0 | Volume regelen |
| Buzzer | D3/PD3/OC2B | Toon afspelen |
| Plus- en minknop | D4/PD4 en D5/PD5 | Filtergrootte aanpassen |
| 7-segmentdisplay met PCF8574 | Op het Game Shield, SDA/A4/PC4 en SCL/A5/PC5 | Filtergrootte tonen |
| 16x2 LCD met I2C-adapter | SDA/A4 en SCL/A5; bij een 5V-adapter: VCC naar 5V en GND naar GND | Afstand en berekende frequentie |
| Ingebouwde LED | D13/PB5 | Geldige afstand aangeven |

Beide displays delen de I2C-bus. Controleer of de LCD-adapter geschikt is voor 5V voordat je hem aansluit.
Zorg voor een gezamenlijke GND en controleer de pull-ups op SDA en SCL.

## Hoe werkt het?

- Timer1 meet de lengte van de echopuls. De software rekent dit om naar centimeters.
- Het mediaanfilter kiest de middelste van de laatste geldige metingen.
  Zo geeft 10, 60, 11 cm als mediaan 11 cm. Een groter filter reageert langzamer.
- De gefilterde afstand bepaalt de toon: `1400 - 18 * afstand`.
  Timer0 schakelt de toon; Timer2 maakt het PWM-signaal voor het volume.
- De potmeter wordt door de ADC uitgelezen. De knoppen gebruiken pull-ups: ingedrukt is LOW.
  Na ongeveer 20 ms zonder verandering telt een druk één keer. Vasthouden herhaalt niet;
  beide knoppen tegelijk geven geen stap.
- Het filter start op 5 en ondersteunt 1, 3, 5, 7, 9, 11, 13 en 15 metingen.
  Het display toont dit als 1, 3, 5, 7, 9, b, d en F.
  Bij vergroten blijven de metingen bewaard; bij verkleinen blijven de nieuwste over.
- Afgeronde afstanden van 2 t/m 65 cm zijn geldig. Anders gaat de LED uit en wordt
  de laagste toon gekozen. Ongeldige metingen komen niet in het filter.

## LCD instellen en controleren

De adressen staan in `include/i2c.h`. Het zijn 7-bit adressen, zonder lees-/schrijfbit.

| Instelling | Ingesteld adres | Controle |
| --- | --- | --- |
| `DISPLAY7_I2C_ADDRESS` | `0x21` | Op hardware gemeten op 28 oktober 2026 |
| `LCD_I2C_ADDRESS` | `0x27` | Op hardware gemeten op 28 oktober 2026 |

De software gebruikt nu deze twee vaste adressen. Ze zijn verschillend, zodat de
displays apart aangesproken worden. Start de Uno opnieuw na het ombedraden.

Bij een andere adapter kun je opnieuw scannen met een los I2C-scanprogramma.
Laat het Game Shield zitten: scan eerst zonder de losse LCD-adapter en daarna met LCD.
Een nieuw adres hoort bij het LCD. Komt er geen adres bij, controleer dan voeding,
bedrading en een mogelijk adresconflict. Schakel de voeding uit voor het ombedraden.
Een PCF8574 gebruikt 0x20..0x27; een PCF8574A gebruikt 0x38..0x3F.
Pas zo nodig de adresjumpers volgens het adapterschema aan en scan opnieuw.
Het tijdelijke scannerproject is na de adrescontrole verwijderd.

Twee gelijke expliciet ingestelde adressen geven een compilerfout.

Met de huidige instellingen werkt het LCD in de praktijktests:
P0=RS, P1=R/W, P2=E, P3=verlichting en P4..P7=D4..D7, met verlichting actief HIGH.
Het exacte chiptype en de pinindeling zijn niet apart met een schema of meting gecontroleerd.
Controleer die bij een andere adapter en pas zo nodig de maskers in `include/lcd.h` aan.
De code gebruikt HD44780-aansturing. Stel het contrast zo af dat de tekst goed leesbaar is.

Het LCD toont de laatst gemeten afstand en de berekende frequentie.
Voor de frequentie wordt het mediaanfilter gebruikt; de getoonde afstand wordt niet gefilterd.
De afstand kan dus wat schommelen terwijl de toon stabieler blijft.
Een nulmeting of timeout toont `---`. Bij een ongeldige speelafstand wordt 230 Hz aangevraagd.
De getoonde frequentie is berekend, niet gemeten. De code schrijft alleen veranderde tekens
en probeert het LCD na een communicatiefout opnieuw in te stellen.
De uitlijning van `cm` en `Hz` werkt goed en er blijven bij wisselende getallen geen oude cijfers staan.

Meer achtergrond: [PCF8574](https://www.ti.com/lit/ds/symlink/pcf8574.pdf), [PCF8574A](https://www.ti.com/lit/ds/symlink/pcf8574a.pdf) en [HD44780](https://cdn-shop.adafruit.com/datasheets/HD44780.pdf).

## Bouwen, uploaden en testen

Ik gebruik PlatformIO om het project te bouwen, te uploaden en te testen.
Hiervoor kun je PlatformIO Core of de PlatformIO-extensie voor VS Code gebruiken.
Voor tests op de pc zijn ook GCC en G++ op PATH nodig, inclusief hun runtime-DLL's op Windows.
Voer de commando's uit vanuit de projectmap:

| Actie | Commando |
| --- | --- |
| Firmware bouwen | `pio run -e uno` |
| Naar de aangesloten Uno uploaden | `pio run -e uno -t upload` |
| Bestaande 35 tests uitvoeren | `pio test -e native -v` |
| 13 displaytests uitvoeren | `pio test -e native_display -v` |
| Statische codecontrole | `pio check -e uno --skip-packages --fail-on-defect low --fail-on-defect medium --fail-on-defect high` |

Voor uploaden kun je zo nodig `--upload-port COMx` toevoegen met jouw poortnummer.
Alleen het uploadcommando schrijft firmware naar de Uno. De tests draaien op de pc.
De displaytests bootsen de bus en klok na met de ingestelde adressen 0x21 en 0x27.
Ze controleren niet de echte adressen, bedrading of elektrische timing van jouw hardware.
Op Windows kan `$env:PYTHONUTF8 = '1'` helpen bij console-encodingproblemen.
De uitvoer staat in de terminal; aparte log- of XML-bestanden zijn niet nodig.
Buildproducten staan in `.pio/` en worden niet in Git opgenomen.

## Uitgevoerde en openstaande tests

| Controle | Datum | Resultaat |
| --- | --- | --- |
| AVR-build en Cppcheck | 28 oktober 2026 | PASS |
| Bestaande tests en displaytests op de pc | 28 oktober 2026 | 35/35 en 13/13 PASS |
| Zes praktijktests die ik zelf heb uitgevoerd met de versie zonder LCD | 25 oktober 2026 | PASS |
| Acht praktijktests van de huidige LCD-versie | 28 oktober 2026 | 8/8 PASS |

Ik moet nog testen of de Theremin zonder aangesloten LCD blijft werken en of het LCD na
een communicatiefout goed herstelt. Een echo-timeout met streepjes is nog niet apart getest.
Gespecialiseerde timingmetingen, langdurige geheugentests en foutinjectie op de Uno staan nog open.
Alle testgevallen, resultaten en technische onderbouwing staan in [docs/TESTRAPPORT.md](docs/TESTRAPPORT.md).

## Belangrijkste codeonderdelen

| Bestand of map | Inhoud |
| --- | --- |
| `src/main.c` | Sensor, meetinterrupts en hoofdloop |
| `src/buzzer.c`, `src/volume.c` | Toon en volume |
| `src/median_filter.c`, `src/theremin_math.c` | Filter en berekeningen |
| `src/filter_controls.c`, `src/button_debounce.c` | Knoppen en contactdender |
| `src/i2c.c`, `src/display7.c`, `src/lcd.c` | Gedeelde bus en displays |
| `include/`, `test/` | Functiedeclaraties en Unity-tests |

Interrupts voeren alleen korte taken uit. De meeste berekeningen en updates gebeuren in de hoofdloop.
Gedeelde variabelen worden veilig uitgelezen en aangepast.
De afstandsberekening voorkomt dat grote waarden overlopen op de Uno.
Het filter vraagt geheugen aan. Lukt een aanpassing niet, dan blijven de oude metingen bewaard.
Zonder geheugen gaan metingen ongefilterd door, toont het display 0 en kan plus het opnieuw proberen.
Dezelfde filter-, reken- en debouncecode wordt ook in de pc-tests gebruikt.

## AI-gebruik

Ik heb Codex gebruikt om de code te controleren en te verbeteren, de tests uit te werken en controleren
en reviews uit te voeren. Codex heeft ook geholpen met de testcode en documentatie
en heeft de beschreven automatische controles lokaal uitgevoerd.

De praktijktests van 25 en 28 oktober 2026 heb ik zelf uitgevoerd, eerst zonder en daarna met LCD. Ten slotte heb ik de tests door een derde laten uitvoeren aan de hand van de handleiding
