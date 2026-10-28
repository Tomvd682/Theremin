# Testrapport Theremin

## 1. Inleiding

In dit rapport leg ik de testresultaten van mijn Theremin-project vast.
Het doel was controleren of de berekeningen, bediening en displays goed werken.
De praktijktests heb ik uitgevoerd op een Arduino Uno met Game Shield.
Op 28 oktober 2026 heb ik ook de versie met het 16x2 LCD getest.
De werking en aansluitingen staan in de [README](../README.md).

## 2. Automatische tests

De volgende controles zijn op 28 oktober 2026 uitgevoerd:

| Controle | Resultaat | Status |
| --- | --- | --- |
| Bestaande Unity-tests | 35 geslaagd, 0 mislukt, 0 genegeerd | PASS |
| Displaytests | 13 geslaagd, 0 mislukt, 0 genegeerd | PASS |
| Totaal automatische tests | 48 geslaagd, 0 mislukt, 0 genegeerd | PASS |
| AVR-build | Firmware bouwt zonder compilerfouten | PASS |
| Cppcheck | Geen meldingen met de ingestelde controles | PASS |
| `git diff --check` | Geen fouten in de witruimte van de wijzigingen | PASS |

De tests controleren vooral:

- Berekeningen voor afstand, frequentie en volume, inclusief grenswaarden
- Het mediaanfilter, de meetvolgorde en het aanpassen van de filtergrootte
- Geheugen aanvragen en opruimen, ook als een geheugenaanvraag mislukt
- Knoppen: contactdender, vasthouden en het wachten op een stabiele druk
- Displays: adreskeuze, initialisatie, tekstlengte, uitlijning en oude cijfers wissen

De tests gebruiken de echte filter-, reken- en displaycode.
Voor de displaytests worden de I2C-bus en klok nagebootst.
Ze gebruiken nu de vaste adressen uit `include/i2c.h`.
Dit controleert de software, maar bewijst niet hoe de echte elektronica reageert.
De afzonderlijke testgevallen staan in de [35 bestaande tests](../test/test_core/test_main.c)
en de [13 displaytests](../test/test_display/test_main.c).

### Opnieuw uitvoeren

Voer vanuit de hoofdmap van het project de volgende commando's uit:

```powershell
pio run -e uno
pio test -e native -v
pio test -e native_display -v
pio check -e uno --skip-packages --fail-on-defect low --fail-on-defect medium --fail-on-defect high
git diff --check
```

Voor de pc-tests zijn PlatformIO en GCC/G++ nodig. Een Arduino hoeft daarvoor
niet aangesloten te zijn. Deze commando's uploaden geen firmware.

## 3. Praktijktests

### 25 oktober 2026 — versie zonder LCD

Ik heb de volgende zes tests op de echte Arduino uitgevoerd:

| Test | Resultaat |
| --- | --- |
| Opstarten | PASS |
| Afstand en toonhoogte | PASS |
| Ongeldige afstand en herstel | PASS |
| Filterknoppen en 7-segmentdisplay | PASS |
| Volumeregeling | PASS |
| Korte stabiliteitstest | PASS |

### 28 oktober 2026 — versie met LCD

Na het uploaden van de LCD-versie heb ik deze acht tests uitgevoerd:

| Test | Resultaat |
| --- | --- |
| LCD toont afstand en frequentie en werkt beide waarden bij | PASS |
| Hand dichterbij geeft een hogere toon | PASS |
| Hand verder weg geeft een lagere toon | PASS |
| Potmeter verandert het volume | PASS |
| Plus- en minknop veranderen de filtergrootte | PASS |
| 7-segmentdisplay toont de juiste filtergrootte | PASS |
| Boven 65 cm gaat de LED uit en wordt 230 Hz gekozen | PASS |
| Bij veranderende getallen blijven geen oude cijfers op het LCD staan | PASS |

Ook de uitlijning van `cm` en `Hz` op het LCD werkt goed.
De 230 Hz is de gekozen softwarewaarde, geen nauwkeurig gemeten toonfrequentie.
Er zijn geen exacte tijdmetingen of een duur van de stabiliteitstest vastgelegd.

Op 28 oktober 2026 heb ik na de vereenvoudiging de firmware opnieuw naar de Uno geüpload.
De afstandssensor en toonhoogte, volumeregeling, filterknoppen en het 7-segmentdisplay
werken nog goed. Ook het LCD met afstand en frequentie, de tekstuitlijning en de
weergave en het gedrag bij een ongeldige afstand werken nog goed (PASS).

## 4. Testresultaten

Alle 48 automatische tests zijn geslaagd. Ook de zes praktijktests van 25 oktober
en de acht praktijktests van 28 oktober zijn geslaagd.

De I2C-adressen heb ik op 28 oktober 2026 op de hardware gecontroleerd:

| Apparaat | Gemeten adres | Instelling in de code |
| --- | --- | --- |
| 7-segmentdisplay op het Game Shield | `0x21` | `0x21` |
| 16x2 LCD | `0x27` | `0x27` |

De adressen zijn verschillend en komen overeen met [include/i2c.h](../include/i2c.h).

## 5. Conclusie

De uitgevoerde tests laten zien dat de basiswerking van de Theremin goed werkt.
De afstand bepaalt de toon, de potmeter regelt het volume en de knoppen veranderen
het filter. Beide displays tonen de verwachte informatie en het LCD houdt geen oude cijfers vast.

De automatische tests draaien op de pc; de praktijktests controleren het gedrag
op de echte Arduino. Samen ondersteunen ze deze conclusie, maar ze betekenen
niet dat alle mogelijke hardware- en foutsituaties zijn getest.
