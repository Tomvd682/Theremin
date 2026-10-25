# Testrapport Theremin — DP-MC2-4

Dit is het centrale testdocument voor het HBO-ICT Microcontrollers-project.
Het beschrijft de automatische controles uit de eerdere Codex-reviews en de
handmatige praktijktests die de student op 25 oktober 2026 heeft gemeld.
De firmware gebruikt een Arduino Uno met Game Shield, zonder Arduino-framework.
De aansluitingen en bediening staan in de [README](../README.md).

## Resultaten en herkomst

| Controle | Werkelijk resultaat | Status | Herkomst |
| --- | --- | --- | --- |
| AVR-build | Geslaagd; 3894/32256 bytes flash en 53/2048 bytes statisch RAM | PASS | Laatste code-review van 25 oktober 2026, uitgevoerd door Codex |
| Native Unity-tests | 35 geslaagd, 0 mislukt, 0 genegeerd | PASS | Dezelfde eerdere review |
| Cppcheck | Geen meldingen binnen de ingestelde controles | PASS | Dezelfde eerdere review |
| Handmatige praktijktests | Zes tests geslaagd; hieronder afzonderlijk vermeld | PASS | Door de student uitgevoerd en gemeld, 25 oktober 2026 |
| Gespecialiseerde timing-, geheugen- en foutinjectietests op de Uno | Niet uitgevoerd | NIET UITGEVOERD | Nog openstaande controles |

Bij deze documentatieopruiming zijn de firmware en testcode niet gewijzigd en
zijn builds, Unity en Cppcheck niet opnieuw uitgevoerd. De automatische uitslagen
hierboven zijn overgenomen uit de vorige review; de toenmalige uitvoer is vóór
verwijdering nagekeken. De 53 bytes RAM omvatten alleen vaste variabelen, geen
heap of stack.

De losse consolelogs en XML-exports zijn bij het opruimen verwijderd. Ze bevatten
herhaalde uitvoer, waarvoor geen afzonderlijke inlevereis is aangeleverd. De
uitslagen, alle testgevallen, technische onderbouwing en herhaalcommando's blijven
hier staan. De testcode blijft beschikbaar om de resultaten opnieuw te controleren.

## Handmatige praktijktests

Volgens de student is de gewijzigde firmware op **25 oktober 2026** geüpload en
zijn de volgende zes tests op de echte hardware uitgevoerd. De resultaten zijn
als handmatige praktijktests vastgelegd, niet als door Codex uitgevoerde metingen.

| Test | Resultaat | Datum |
| --- | --- | --- |
| Opstarten | PASS | 25 oktober 2026 |
| Afstand en toonhoogte | PASS | 25 oktober 2026 |
| Ongeldige afstand en herstel | PASS | 25 oktober 2026 |
| Filterknoppen en 7-segmentdisplay | PASS | 25 oktober 2026 |
| Volumeregeling | PASS | 25 oktober 2026 |
| Korte stabiliteitstest | PASS | 25 oktober 2026 |

Er zijn geen exacte meetwaarden of testduur opgegeven. Er zijn daarom geen
centimeters, frequentiemetingen of tijdsduren aan deze praktijktests toegevoegd.
De korte stabiliteitstest is geen langdurige geheugen- of duurtest. Ook bewijzen
deze zes tests geen nauwkeurige ISR-timing of herstel na een geforceerde malloc-fout.

## Automatische controles herhalen

Voer vanuit de projectmap uit:

```text
pio run -e uno
pio test -e native -v
pio check -e uno --skip-packages --fail-on-defect low --fail-on-defect medium --fail-on-defect high
```

Hiervoor zijn PlatformIO en een native GCC/G++-toolchain op PATH nodig. Voor de
pc-tests hoeft geen Arduino aangesloten te zijn. Deze commando's uploaden niets
naar het bord en hebben geen uitvoeroptie voor log- of XML-bestanden nodig.
Buildproducten en testprogramma's staan onder de genegeerde map `.pio/`.
Op Windows kan `$env:PYTHONUTF8 = '1'` console-encodingproblemen voorkomen.

Versies bij de vorige review: PlatformIO Core 6.2.0, Atmel AVR 5.3.0,
AVR-GCC 7.3.0 (toolchain 1.70300.191015), Native 1.2.1, MSYS2 UCRT64 GCC 16.1.0,
Unity 2.6.1 en Cppcheck 2.11 op Windows. Beide omgevingen bouwen met C11 en
`-Wall -Wextra -Werror`.

De native omgeving bouwt de echte `median_filter.c`, `theremin_math.c` en
`button_debounce.c`. De AVR-registerdrivers en firmware-main worden uitgesloten.
Alleen de malloc/free-aanroepen van het filter krijgen in de tests een wrapper:
die gebruikt echte heapallocatie, telt actieve buffers, controleert pointers en
controlebytes en kan een allocatiefout nabootsen. De Uno gebruikt gewone AVR-libc.
Dit houdt de productielogica in beide omgevingen gelijk.

## Onderbouwing per rubriccriterium

| Criterium | Aanpak en gecontroleerd gedrag |
| --- | --- |
| ISR's | ISR's doen kort werk: meetwaarden vastleggen, een vlag zetten of PWM schakelen. Filteren, malloc/free, sorteren en I2C gebeuren in main. Gedeelde waarden gebruiken waar nodig volatile. Het 16-bit meetresultaat en de gereedvlag worden samen atomisch opgehaald; ook de 32-bit debounceklok wordt atomisch gelezen. |
| Pointers | De filtermodule beheert één bufferpointer. NULL wordt vóór gebruik gecontroleerd. qsort leest de argumenten als const-pointers; vergelijken gebeurt zonder aftrekking die kan overlopen. Een sorteerkopie bewaart de meetvolgorde. De debouncepointer wijst naar een geldig object. |
| Dynamisch geheugen | Bij resize wordt eerst nieuw geheugen aangevraagd en gevuld, daarna de oude buffer vrijgegeven. Een allocatiefout behoudt de oude toestand. `median_filter_free()` zet na free de pointer zelf op NULL en grootte/count op 0. De tests controleren ook herhaald opruimen en allocatiefouten. |
| Testrapportage | Dit document onderscheidt eerder uitgevoerde automatische tests, door de student gemelde praktijktests en nog niet uitgevoerde controles. Verwachtingen, werkelijke uitslagen, beperkingen en herhaalcommando's zijn vastgelegd. |

### Belangrijke technische keuzes en oplossingen

- **Afstand:** `(ticks + 58) / 116` kon op AVR bij ticks vanaf 65478 overlopen,
  omdat int daar 16 bits is. De productiecode verbreedt nu vóór de optelling
  naar uint32_t. Maximaal is dat `65535 + 58 = 65593`; de uitkomst is 0..565 cm.
  Afgeronde afstanden 2..65 cm worden geaccepteerd (174..7597 ticks).
- **Echo meten:** de twee hardwaretijdstippen uit ICR1 worden afgetrokken. De
  timer bij de stijgende flank in de ISR resetten zou ISR-latentie van de puls
  aftrekken. Timer1 bewaakt ook de timeout en de pauze tot de volgende meting.
  Een oude overflowflag wordt bij het resetten voor die pauze gewist.
- **Timer0:** main vraagt een nieuwe TOP aan via een atomische 8-bit variabele.
  De ISR telt eerst de oude periode bij de klok op en neemt daarna de nieuwe
  TOP over. Een lagere TOP midden in een periode zou een compare match kunnen
  missen. De volume-instelling en OCR2B worden samen atomisch bijgewerkt.
- **Knoppen:** de ISR zet alleen een wijzigingsvlag. Main leest vlag, ingang en
  klok samen atomisch en controleert ook een nog wachtende PCINT-hardwareflag.
  Fysieke flanken kunnen immers optreden terwijl interrupts uitstaan. Unsigned
  tijdverschillen houden de wachttijd correct over een klokomloop. Alleen plus
  of alleen min geeft een stap; beide tegelijk niet.
- **Mediaanfilter:** groottes zijn 1..15 oneven. Een resize bewaart de nieuwste
  metingen; een gelijke grootte vraagt geen nieuw geheugen aan. Leeftijden zijn
  maximaal 15 vóór vervanging en worden na resize opnieuw genummerd. Een even
  aantal samples tijdens het vullen geeft de onderste middelste waarde.
- **Allocatiefouten:** zonder buffer geeft het filter de meting ongewijzigd
  terug en toont het display 0. Plus kan dan opnieuw grootte 1 proberen. Bij
  een mislukte latere resize blijven de oude grootte en metingen intact.
- **Rekenen:** `1400 - 18 * cm` geeft voor geldige invoer een aanvraag van
  1364..230 Hz; ongeldige invoer kiest 230 Hz. De Timer0-berekening gebruikt
  32 bits en een nooit-nul noemer, met TOP 22..134. ADC-percentages ronden af;
  PWM gebruikt gehele deling. Maxima 25627 en 25500 passen in 16 bits.

Deze getallen beschrijven berekeningen en instellingen, geen gemeten waarden
uit de handmatige praktijktests.

### Timers en statische ISR-controle

| Onderdeel | Instelling en rol |
| --- | --- |
| Timer0 | CTC, prescaler 256; toonmodulatie en debounceklok, ook bij volume 0 |
| Timer1 | Normal mode, prescaler 8; trigger, Input Capture, timeout en cooldown |
| Timer2 | Fast PWM, prescaler 1; PWM op D3, zonder eigen ISR |
| ADC | Free-running, prescaler 128; ISR kopieert alleen ADCH |
| PCINT2 | D4/D5; ISR meldt de knopverandering |

Timer0 en Timer1 delen een prescalerblok, maar de code reset dat gedeelde blok
niet. ISR's nesten niet. Variabelen die uitsluitend door ISR's worden gebruikt
hebben daarom geen overdracht met main nodig. De bestaande pinconfiguratie en
PCF8574-adresrange 0x20..0x27 zijn behouden.

In de vorige reviews is de AVR-disassembly bekeken: geen van de zes ISR's had
calls, delingen, allocaties of lussen. Na de vereenvoudigingen was hun machinecode
identiek. Hieronder staan de toen vastgelegde instructietellingen en ruime
**statische schattingen**, geen gemeten uitvoeringstijden.

| ISR | Instructies | Calls | Statische bovengrens bij 16 MHz |
| --- | ---: | ---: | ---: |
| ADC | 14 | 0 | 67 cycli / 4,19 us |
| Timer1 overflow | 39 | 0 | 167 cycli / 10,44 us |
| Timer1 capture | 57 | 0 | 239 cycli / 14,94 us |
| Timer1 compare A | 24 | 0 | 107 cycli / 6,69 us |
| PCINT2 | 14 | 0 | 67 cycli / 4,19 us |
| Timer0 compare A | 57 | 0 | 239 cycli / 14,94 us |

De schatting telt 4 cycli per aanwezige instructie, inclusief proloog, epiloog en
RETI, plus 11 voor interrupt-entry, vectorsprong en de lopende instructie.
Door alle takken mee te tellen is dit ruim gerekend. De schatting omvat geen
wachttijd door andere ISR's, atomic blocks of een storm aan externe flanken.
De kortste ingestelde Timer0-periode is 368 us. De debounceklok heeft afhankelijk
van de toon een tijdresolutie van ongeveer 0,368..2,160 ms, met extra vertraging
door de hoofdloop. Exacte signaaltiming moet nog worden gemeten.

### Cppcheck en geheugenbeperkingen

De eerste Cppcheck-run gaf FAIL: 21 `unusedFunction`-meldingen en één `uninitvar`.
PlatformIO analyseert per bestand en ziet bij unusedFunction de aanroepen uit
andere bestanden niet. De instellingen activeren daarom warning, style,
performance en portability, zonder unusedFunction. Bij het sorteren is één
plaatselijke uninitvar-uitzondering vastgelegd: na toevoegen geldt altijd
`1 <= sample_count <= filter_size <= 15` en zijn alle te sorteren elementen
gevuld. Andere uninitvar-controles blijven aan. De uiteindelijke run gaf PASS.
`--skip-packages` slaat toolchain-includepaden over; de echte AVR-build controleert
de AVR-headers en registerdeclaraties. Dit is geen bewijs van hardwaregedrag.

Een FilterSample neemt op deze AVR 3 bytes in. De twee grootste verschillende
buffers tijdens resize bevatten samen maximaal 84 bytes aan samples, plus
allocatoradministratie. De sorteerkopie gebruikt maximaal 45 stackbytes. Calls,
qsort en ISR's gebruiken extra stack. De native geheugencontrole is gericht en
vervangt geen volledige sanitizer-run of meting van de stack-/heapmarge op de Uno.

## Alle 35 Unity-testgevallen

Dit zijn de in de vorige review uitgevoerde tests. De namen staan zonder het
voorvoegsel `test_`; ze zijn terug te vinden in de
[testcode](../test/test_core/test_main.c). Eén test kan veel waarden doorlopen:
65.536 invoerwaarden tellen als één testgeval, niet als 65.536 losse tests.
| Testnaam (zonder `test_`) | Verwacht resultaat | Werkelijk resultaat | Status |
| --- | --- | --- | --- |
| uninitialized_filter_passes_through | Grootte 0; 0 en 65535 onveranderd; geen allocatie | 0, 65535; 0 allocaties | PASS |
| all_valid_filter_sizes | Alle acht groottes werken; oude buffers vrij | Groottes 1..15 oneven; 8 allocaties, 7 frees vóór teardown | PASS |
| all_invalid_sizes_preserve_filter | Alle 248 ongeldige bytewaarden afgewezen; historie intact | Alle afgewezen; grootte 3; volgende mediaan 30 | PASS |
| size_one_always_returns_latest | Altijd de nieuwste waarde, inclusief uitersten | Reeks 0,65535,100,1,30000 exact terug | PASS |
| unsorted_samples_and_lower_warmup_median | Mediaan correct; even opvulling kiest onderste | Reeks 50,10,30,20,30 | PASS |
| duplicates_and_uint16_limits | Duplicaten en 0/65535 zonder comparatoroverflow | Getoetste medianen 65535,0,0 | PASS |
| sliding_window_evicts_oldest_not_smallest | Oudste sample vervalt, ongeacht de waarde | Medianen 90,2,80,3 | PASS |
| shrink_keeps_newest_in_correct_order | 5 naar 3 bewaart nieuwste en juiste vervangvolgorde | Volgende medianen 3 en 50 | PASS |
| grow_preserves_samples_and_their_order | 3 naar 5 behoudt historie/volgorde | Volgende medianen 10,50,10 | PASS |
| resize_partially_filled_filter | Resize gebruikt alleen gevulde samples | Volgende medianen 20,20 | PASS |
| same_size_does_not_allocate_or_reset | Geen extra malloc/free of historieverlies | 1 allocatie, 0 frees; mediaan 20 | PASS |
| initial_allocation_failure_and_recovery | NULL veilig; doorvoer; nieuwe poging werkt | Resize false, grootte 0, doorvoer 123, herstel naar 1 met waarde 42 | PASS |
| failed_grow_preserves_buffer_and_history | NULL bij groeien behoudt oude buffer | Grootte 3, 1 levend blok, 0 frees; mediaan 2 | PASS |
| failed_shrink_preserves_buffer_and_history | NULL bij krimpen behoudt oude buffer | Grootte 5; mediaan 40 | PASS |
| free_is_idempotent_and_reinitialization_is_empty | Geen dubbele free, geen oude samples na herstart | 1 free vóór herstart; doorvoer 7; nieuwe mediaan 10 | PASS |
| long_sequence_with_resizes_matches_reference | 30.000 samples met resizes gelijk aan onafhankelijk model | Alle 30.000 gelijk; steeds 1 buffer en geldige guards | PASS |
| repeated_resize_and_free_has_no_leaks | 100 cycli door acht groottes, alle buffers vrij | 800 allocaties en 800 frees; steeds 0 na cleanup | PASS |
| distance_rounding_boundaries | Correcte afronding en validiteitsranden | 0,57,58,173,174,7540,7597,7598 ticks geven 0,0,1,1,2,65,65,66 cm | PASS |
| distance_avr_overflow_regression | Geen terugval naar 0 bij hoogste ticks | 65477,65478,65481 -> 564; 65482,65535 -> 565; maximum ongeldig | PASS (native) |
| every_echo_tick_matches_wide_reference | Alle 65.536 tickwaarden gelijk aan 64-bit referentie en monotoon | Alle gelijk en niet-dalend | PASS |
| distance_validity_limits | Alleen 2..65 cm geldig | 0,1,66,65535 false; 2,65 true | PASS |
| frequency_mapping_and_invalid_input | Geldige mapping, veilige fallback buiten bereik | 2,10,50,65 -> 1364,1220,500,230 Hz; ongeldige invoer -> 230 | PASS |
| frequency_is_monotonic_over_valid_distances | Elke extra cm verlaagt aanvraag met 18 Hz | Alle stappen 3..65 exact 18 | PASS |
| timer_top_clamping_and_known_values | Clamp en gehele afronding correct | 0,229,230,500,1000,1400,65535 Hz -> TOP 134,134,134,62,31,22,22 | PASS |
| every_frequency_has_safe_monotonic_timer_top | Alle 65.536 frequentie-invoeren veilig, monotoon | Alle TOP-waarden 22..134, niet-stijgend | PASS |
| adc_volume_known_rounding_values | Bekende ADC-grenzen correct afgerond | 0,1,2,126,127,128,254,255 -> 0,0,1,49,50,50,100,100% | PASS |
| all_adc_values_round_to_nearest_percent | Alle 256 waarden monotoon, 0..100, afrondfout <= 127/255 procentpunt | Alle assertions geldig | PASS |
| pwm_volume_clamping_and_rounding | PWM rondt omlaag; invoer boven 100 begrensd | 0,1,50,99,100,101,255 -> 0,2,127,252,255,255,255 | PASS |
| all_pwm_inputs_are_bounded_and_monotonic | Alle 256 invoeren volgen begrensde conversie | Alle gelijk aan referentie en niet-dalend | PASS |
| debounce_waits_twenty_ms_and_emits_once | Voor 1250 ticks geen event; daarna eenmaal | Bij t=1349 geen event, t=1350 bit 0x10; vasthouden geen herhaling | PASS |
| bounce_restarts_stability_interval | Elke flank start wachttijd opnieuw | Op laatste flank +1249 geen; +1250 bit 0x10 | PASS |
| release_then_second_press | Loslaten geen druk; nieuwe druk opnieuw event | Twee druk-events, geen release-event | PASS |
| debounce_handles_clock_wrap | Wachttijd correct over uint32-wrap | t=748 geen event; t=749 bit 0x10 | PASS |
| edge_at_deadline_defers_acceptance | Flank op deadline wordt niet direct geaccepteerd | Geen event op deadline of na stabiele release | PASS |
| simultaneous_presses_and_initially_held_button | Beide drukbits samen; bij opstart vastgehouden knop geen kunstmatig event | 0x30 bij gelijktijdige druk; 0 bij al vastgehouden knop | PASS |

Voor alle 35 tests controleert teardown bovendien buffergrenzen, ongeldige frees,
eindgrootte 0 en nul actieve allocaties. De knoptests testen de echte debouncekern.
De hardwareflag en de plus/minus-keuze in de registerdriver zijn beoordeeld maar
worden niet door native tests uitgevoerd.

## Nog openstaande hardwarecontroles

De gespecialiseerde timing-, geheugen- en foutinjectietests zijn **NIET
UITGEVOERD**. De zes praktijktests hierboven veranderen die status niet.

| Controle | Doel / verwacht gedrag | Status |
| --- | --- | --- |
| Trigger- en echotiming met logic analyzer | Triggerduur, pulsmeting, timeout en cooldown controleren | NIET UITGEVOERD |
| PWM, toonfrequenties en ISR-timing meten | PWM-carrier en modulatie controleren, ook bij snelle frequentiewijzigingen; geen gemiste periode | NIET UITGEVOERD |
| Langdurige geheugen- en stabiliteitstest | Veel resizes; heapfragmentatie, stackmarge en eventuele resets onderzoeken | NIET UITGEVOERD |
| Malloc-fouten op de Uno forceren | Opstartfout: display 0 en doorvoer; plus kan herstellen. Latere fout: oude filtertoestand behouden | NIET UITGEVOERD |
| Display-/I2C-fouten op de Uno forceren | Ontbrekend display en communicatieverlies mogen de overige functies niet blijvend blokkeren | NIET UITGEVOERD |

Knopdetails zoals doelbewuste contactdender, beide knoppen tegelijk en bedienen
tijdens snelle toonwisselingen zijn niet afzonderlijk beschreven in de zes
praktijkresultaten. Ook snel resizen bij wisselende afstanden is niet apart
vastgelegd. Daarvoor wordt hier geen aanvullende PASS geclaimd. Een eventuele
vervolgtest kan in dit document worden toegevoegd met datum en waarneming.

## Resterende beperkingen

- Native GCC heeft een 32-bit int, AVR een 16-bit int. De native afstandstest
  alleen bewijst de AVR-overflowfix niet; de oude expressie zou op de pc ook
  werken. In de eerdere AVR-assembly is daarom de verbreding vóór optellen en
  de 32-bit deling (`__udivmodsi4`) gecontroleerd. Er zijn geen Unity-tests op de
  AVR zelf uitgevoerd.
- Pc-tests modelleren geen registers, interruptprioriteiten, elektrische ruis,
  PWM-overgangen of I2C. De hardwareflag en de knopkeuze zijn beoordeeld, terwijl
  de native tests alleen de hardware-onafhankelijke debouncekern uitvoeren.
- Sensorwerking, temperatuur en afronden op hele centimeters beperken de
  meetnauwkeurigheid. Door hele timerticks liggen de berekende toonuitersten
  ongeveer op 231..1359 Hz; deze frequenties zijn niet als meetwaarden opgegeven.
- Ongeldige metingen worden niet aan het filter toegevoegd; oude geldige samples
  blijven staan bij herstel. Er is alleen ruimte voor het nieuwste nog niet
  verwerkte sensorresultaat, geen meetqueue voor lange blokkades in main.
- I2C-wachten heeft een timeout per handeling. Een display dat bij opstarten
  ontbreekt, wordt later niet opnieuw gezocht. Alleen 0x20..0x27 wordt gescand.

## Repository en werkwijze

De repository bewaart broncode, headers, PlatformIO-configuratie, Unity-tests,
de README en dit testrapport. De relevante testtabel, rubriconderbouwing,
rekenbereiken, ISR-controle, beperkingen en reviewkeuzes uit de eerdere bijlagen
zijn hier samengevoegd. Losse reviewdocumenten en gegenereerde testuitvoer zijn
verwijderd. `.pio/` en eventuele `docs/test-results/`-exports blijven buiten Git.

De eerdere vereenvoudigingen zijn behouden: main gebruikt rechtstreeks de
fallback van het filter; knopvoorwaarden accepteren alleen plus of min;
`stable_buttons` geeft de opgeslagen toestand duidelijk aan. Comments verklaren
de rekenconstanten en hardwareflags. De testcode gebruikt een expliciete 64-bit
controleberekening en afzonderlijke pointer-/tellerstappen. Er is geen extra
abstractielaag of testframework toegevoegd. libc-free wist de pointer niet;
`median_filter_free()` doet dat zelf.

AI-assistentie (Codex) is gebruikt voor code, tests, reviews en documentatie.
De eerdere automatische controles zijn lokaal door de assistent uitgevoerd.
De handmatige praktijktests zijn door de student gemeld. Er zijn geen
schoolregels over AI-gebruik meegeleverd; houd de vermelding en het gebruik in
overeenstemming met de eisen van de opleiding. Er zijn geen persoonlijke
ervaringen of niet-uitgevoerde metingen toegevoegd.

Er is uitsluitend op `feature/code-quality-tests` gewerkt. Bestaande wijzigingen
zijn behouden; er is niets gecommit, gepusht of gemerged en geen branch gewisseld.
De historie is niet aangepast en er zijn geen ontwikkelstappen achteraf verzonnen.

## Technische bronnen

- [Microchip ATmega328P-datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf): CTC zonder double buffering, Input Capture en register-/interruptgedrag.
- [PlatformIO Unity](https://docs.platformio.org/en/stable/advanced/unit-testing/frameworks/unity.html): native Unity-tests en testlevenscyclus.
- [PlatformIO test_build_src](https://docs.platformio.org/en/latest/projectconf/sections/env/options/test/test_build_src.html) en [build_src_filter](https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/build_src_filter.html): productiecode met de test-main bouwen en hardwaredrivers uitsluiten.
