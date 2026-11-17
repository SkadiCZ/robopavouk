# Robopavouk: firmware (ESP32-C3)

Program pro ESP32-C3 SuperMini. Řídí 18 serv MG90S přes dvě desky PCA9685, čte ultrazvukový senzor HC-SR04 a ovládá se z mobilu přes WiFi nebo ze sériové linky.

| Soubor | Co v něm je |
|---|---|
| `robospider/robospider.ino` | hlavní program: serva, senzor, příkazy, WiFi |
| `robospider/motion.h` | pohybové jádro: inverzní kinematika, chůze, sekvence (stejné výpočty jako simulátor) |
| `robospider/config.h` | piny, adresy, směry serv, vzdálenosti pro senzor |
| `robospider/webpage.h` | ovládací stránka pro mobil |
| `test/test_motion.cpp` | test jádra na počítači |

## Co je ověřené a co ne

- **Ověřené na počítači:** pohybové jádro prošlo testem ve všech chůzích, směrech a sekvencích. Žádný kloub nejde mimo meze modelu, žádné servo nepotřebuje víc než 350 °/s (MG90S zvládne asi 600 °/s) a těžiště zůstává nad oporou.
- **Ověřené náhradou:** hlavní program jsem přeložil a prošel s napodobeninou desky a senzoru (vstávání, chůze, zastavení před překážkou, průzkum, následování, lehnutí).
- **Neověřené:** překlad v Arduino IDE pro skutečný ESP32-C3 a chování na skutečném robotu. K nástrojům pro ESP32 jsem se odsud nedostal. Kdyby překlad hlásil chybu, pošli mi ji.
- **Směry serv jsou odhad** podle modelu. Ověříš je při kalibraci níže.

## Zapojení

| Odkud | Kam |
|---|---|
| ESP32 GPIO8 (SDA) | SDA obou PCA9685 |
| ESP32 GPIO9 (SCL) | SCL obou PCA9685 |
| ESP32 3V3 | VCC obou PCA9685 (logika) |
| ESP32 GND | GND obou PCA9685, GND senzoru, GND step-downů |
| step-down 6 V č. 1 | V+ levé PCA9685 (adresa 0x40) |
| step-down 6 V č. 2 | V+ pravé PCA9685 (adresa 0x41, zapájená propojka A0) |
| ESP32 5V | VCC senzoru HC-SR04 |
| ESP32 GPIO4 | TRIG senzoru |
| ECHO senzoru | přes dělič na ESP32 GPIO5 |

- **Dělič na ECHO je nutný.** Senzor dává 5 V a ESP32 snese jen 3,3 V. Zapoj ECHO přes rezistor 1 kΩ na GPIO5 a z GPIO5 rezistor 2 kΩ na GND.
- **Serva na deskách:** přední noha kanály 0, 1, 2 (coxa, stehno, koleno), prostřední 3, 4, 5, zadní 6, 7, 8. Levé nohy na desce 0x40, pravé na 0x41.
- ESP32 napájej z 5 V (malý regulátor z baterie nebo USB). Při ladění přes USB nech baterii serv zapojenou, USB serva neutáhne.

## Nahrání

1. V Arduino IDE přidej podporu desek ESP32 (Správce desek, „esp32“ od Espressif).
2. Otevři `robospider/robospider.ino`. Deska: **ESP32C3 Dev Module**, **USB CDC On Boot: Enabled**.
3. Nahraj. Žádné další knihovny nejsou potřeba.
4. Otevři sériový monitor na 115200 Bd a napiš `help`.

## První spuštění a kalibrace

Po zapnutí se nic nehýbe, serva jsou vypnutá, dokud nedáš `vstan`. Nejdřív ale kalibruj, s robotem položeným na krabičce, aby nohy visely volně.

1. `stav` má ukázat `PCA L ok P ok`. Když některá chybí, zkontroluj I2C a propojku A0.
2. `stred` nastaví všechna serva na 90°. V této poloze nasaď páky tak, aby nohy byly v montážní póze: coxa kolmo od trupu, stehno 20° nahoru, koleno 90°.
3. Pro každý kloub zkus směr, třeba `test LP coxa 20`. Správně je:
   - coxa: noha se otočí proti směru hodin při pohledu shora,
   - stehno: stehno jde dolů,
   - koleno: koleno se víc ohne, špička jde k tělu.
   Když jde kloub opačně, napiš `smer LP coxa`. Uloží se to natrvalo.
4. Drobné odchylky dorovnej příkazem `trim LP koleno -3` (stupně). `kalibrace` vypíše celou tabulku.
5. Nohy: LP, LS, LZ jsou levá přední, střední, zadní; PP, PS, PZ pravé.

Pak robota polož na stůl s nohama roztaženýma do stran a dej `vstan`.

## Ovládání

**WiFi:** připoj mobil k síti `Robopavouk` (heslo `pavouk1234`, změň si ho v `config.h`) a otevři `http://192.168.4.1`. Jsou tam tlačítka na všechno.

**Příkazy** (sériová linka i web):

| Příkaz | Co dělá |
|---|---|
| `vstan`, `lehni` | vstane ze země, lehne si a vypne serva |
| `vpred` `vzad` `vlevo` `vpravo` | chůze, vlevo a vpravo je krabí chůze do strany |
| `otoc_l` `otoc_p` | otáčení na místě |
| `stop` | zastaví chůzi i automatický režim |
| `chuze tripod` / `ripple` / `vlna` | druh chůze: rychlá trojnožka, střední, pomalá a nejšetrnější |
| `mavej` | zamává levou přední nohou |
| `podej` | podá přední nohu a zapumpuje |
| `uklona` | ukloní se |
| `utok` | útočný postoj: stojí na čtyřech, přední nohy nahoře |
| `tanec` | tanec tělem |
| `krouzeni` | krouží tělem na místě |
| `zavrt` | zavrtí se |
| `kliky` | tři kliky |
| `pochod` | pochoduje na místě |
| `pruzkum` | chodí sám a vyhýbá se překážkám |
| `nasleduj` | drží se asi 10–16 cm od ruky před senzorem, jde za ní a couvá před ní |
| `hlidej 0/1` | při chůzi vpřed zastaví před překážkou (zapnuto) |
| `lek 0/1` | když stojí a něco se přiblíží pod 8 cm, lekne se a couvne (zapnuto) |
| `zivy 0/1` | lehké „dýchání“ a rozhlížení při stání (zapnuto) |
| `rychlost` `frekv` `krok` `vyska` `rozkrok` | parametry chůze |
| `stred`, `test`, `smer`, `trim`, `kalibrace`, `uvolni` | kalibrace, jen když robot leží |

## Jak vstává a lehá

V lehu má robot nohy roztažené víc do stran, protože koleno se nemůže ohnout tak, aby byla chodidla pod tělem. Při `vstan` se nejdřív zvedne na všech šesti nohách najednou, takže je zátěž rozložená, a pak nohy po jedné přitáhne do chodicí polohy. `lehni` dělá totéž pozpátku.

## Ultrazvukový senzor

Senzor sedí v držáku `sonar_mount.stl` na víku vpředu, nad ESP32. Drží ho dva šrouby M3 s maticí, víko pro ně má dvě nové díry. Vzdálenosti, při kterých robot reaguje, jsou v `config.h`. Bez senzoru nastav `SONAR_ENABLED 0`.

## Test na počítači

```
cd firmware/test
g++ -std=c++11 -O2 -o test_motion test_motion.cpp && ./test_motion
```
Vypíše pro každou chůzi a sekvenci rozsahy kloubů, nejvyšší rychlost serva a rezervu stability. Hodí se po každé úpravě sekvencí v `motion.h`.
