# Zapojení motor driveru JYQD V7.3E2

Návod na propojení **LaskaKit ESP32-S3-DEVKit** (logika **3,3 V**) s ovladačem **JYQD-V7.3E2** (řídicí vstupy **5 V**) přes **bidirectional level shifter** na bázi **BSS138**.

## Reference

| Dokument | Soubor |
|----------|--------|
| Motor driver | [`datasheeds/jyqd-v7_3e2-english.pdf`](../datasheeds/jyqd-v7_3e2-english.pdf) |
| Level shifter (BSS138) | [`datasheeds/bss138.pdf`](../datasheeds/bss138.pdf) |
| Schéma level shifteru | [`datasheeds/logic_level_bidirectional_scheme.pdf`](../datasheeds/logic_level_bidirectional_scheme.pdf) |
| Pinout desky | [LaskaKit ESP32-S3-DEVKit pinout](https://github.com/LaskaKit/ESP32-S3-DEVKit/blob/main/img/ESP32-S3-DEVKIT_pinout.png) |

## Přehled komponent

| Komponenta | Popis |
|------------|-------|
| **LaskaKit ESP32-S3-DEVKit** | Řídicí deska, GPIO **3,3 V** |
| **JYQD-V7.3E2** | Driver pro **1× BLDC motor s Hall senzorem** (12–36 V, max 15 A) |
| **Level shifter** | Modul nebo vlastní zapojení s **BSS138** (3,3 V ↔ 5 V) |
| **Baterie** | 12–36 V pro motory (oddělené napájení od ESP32) |

Pro sekačku se dvěma koly jsou potřeba **2× JYQD driver** a **2× 4kanálový level shifter** (nebo **1× 8kanálový**).

---

## Řídicí port motor driveru

Pinout řídicího konektoru (rozteč **2,54 mm**), shora dolů dle datasheetu:

| Pin | Název | Směr | Popis |
|-----|-------|------|-------|
| 1 | **5V** | Výstup | Interní 5 V z driveru — **nepřipojovat na ESP32** |
| 2 | **EL** | Vstup | Enable: **5 V / volno** = povoleno, **GND** = vypnuto |
| 3 | **Signal** | Výstup | Pulzní signál otáček (5 V logika) |
| 4 | **Z/F** | Vstup | Směr: **5 V / volno** = vpřed, **GND** = vzad |
| 5 | **VR** | Vstup | Rychlost: analog **0,1–5 V** nebo **PWM** 1–20 kHz (duty 0–100 %) proti GND |
| 6 | **GND** | — | Společná zem řídicí části |

> **Upozornění z datasheetu:** Pin **5V** slouží jen pro externí potenciometr nebo spínač. **Ne napájet z něj ESP32** ani jiné spotřebiče — pouze jako referenci **HV** pro level shifter (malý proud přes pull-up rezistory).

---

## Proč level shifter

| Signál | ESP32 | JYQD | Level shifter |
|--------|-------|------|---------------|
| **VR**, **Z/F**, **EL** | GPIO výstup 3,3 V | očekává 5 V logiku | **Ano** (3,3 V → 5 V) |
| **Signal** | GPIO vstup max 3,3 V | výstup 5 V | **Ano** (5 V → 3,3 V) |
| **GND** | GND | GND | **Společná zem** (přímo) |
| **5V** (driver) | — | interní výstup | Jen na pin **HV** shifteru |

Bez level shifteru hrozí u signálu **Signal** poškození ESP32 (5 V na GPIO) a u výstupů **Z/F** / **EL** nespolehlivá detekce logické 1 (3,3 V může být na hraně).

---

## Level shifter (BSS138)

Použij hotový **4kanálový bidirectional modul** (BSS138) nebo zapojení dle [`logic_level_bidirectional_scheme.pdf`](../datasheeds/logic_level_bidirectional_scheme.pdf).

### Pinout modulu — fyzické rozložení pinů

Modul má **dvě strany** (rozteč 2,54 mm). Kanály jsou v řadě proti sobě — **LV1** je na straně ESP32, **HV1** na straně JYQD, oba patří k jednomu signálu.

```
  STRANA ESP32 (3,3 V)              STRANA JYQD (5 V)
  ────────────────────              ───────────────────

       LV1  ─────────────────────────  HV1
       LV2  ─────────────────────────  HV2
       LV3  ─────────────────────────  HV3
       LV4  ─────────────────────────  HV4

       LV   ───► 3V3 (levá řada, nahoře)
       GND  ───► společná zem ◄────────  GND
       HV   ─────────────────────────  HV  ◄── JYQD 5V
```

**Levá / LV strana** (6 pinů):

| Pin | Kam zapojit |
|-----|-------------|
| **LV1** | GPIO 4 (PWM → VR) |
| **LV2** | GPIO 5 (Z/F) |
| **LV3** | GPIO 6 (EL) |
| **LV4** | GPIO 18 (Signal — vstup z driveru) |
| **LV** | Pin **3V3** — levá řada pinů, úplně nahoře |
| **GND** | **GND ESP32** (+ propoj s GND na druhé straně a GND driveru) |

**Pravá / HV strana** (6 pinů):

| Pin | Kam zapojit |
|-----|-------------|
| **HV1** | JYQD **VR** |
| **HV2** | JYQD **Z/F** |
| **HV3** | JYQD **EL** |
| **HV4** | JYQD **Signal** |
| **HV** | JYQD **5V** (řídicí konektor) |
| **GND** | **GND driveru** (control) — stejná zem jako GND na LV straně |

> Oba piny **GND** (LV i HV strana) jsou na modulu propojené — stačí zapojit jeden, ideálně oba do společné země s ESP32 a driverem.

**LV1↔HV1**, **LV2↔HV2**, **LV3↔HV3**, **LV4↔HV4** = čtyři nezávislé kanály. Číslo musí sedět: GPIO na **LV3** patří k pinu **HV3** na JYQD, ne k HV1.

| Typ pinu | Účel |
|----------|------|
| **LV**, **HV**, **GND** | Reference napětí pro shifter — **nejsou datové signály** |
| **LV1…4** / **HV1…4** | Datové signály (3,3 V ↔ 5 V) |

### Napájení shifteru

```
3,3 V (viz níže)  ──►  LV   (LV strana modulu)
ESP32 GND         ──►  GND  (LV nebo HV strana)
JYQD 5V           ──►  HV   (HV strana modulu)
JYQD GND          ──►  GND  (HV strana modulu)
```

#### Kde vzít 3,3 V na LaskaKit ESP32-S3-DEVKit

**Primární:** pin **3V3** — **horní pin levé řady** hlavních pinů (dle [oficiálního pinoutu](https://github.com/LaskaKit/ESP32-S3-DEVKit/blob/main/img/ESP32-S3-DEVKIT_pinout.png)). Ten patří na **LV** shifteru.

**Alternativa:** konektor **uŠup I2C** (pin 3,3 V), ale jen když je **GPIO 47 = HIGH** (v projektu dělá `UshupBus::begin()`).

| Zdroj 3,3 V | Kde na desce |
|-------------|--------------|
| **3V3** | Levá řada pinů, **úplně nahoře** (nad EN) |
| uŠup I2C pin 1 | JST konektor uprostřed desky (GPIO 47 = HIGH) |

> **VCC** (5 V vstup, dole na levé straně) **≠ 3V3**. Na **LV** shifteru nikdy nepoužívej **VCC**.

**Pořadí pinů uŠup I2C** (pokud bereš 3,3 V odtud):

| Pin | Signál | Barva kabelu (typicky) |
|-----|--------|------------------------|
| 1 | **3,3 V** | červená → **LV** shifteru |
| 2 | **GND** | černá |
| 3 | **SCL** (GPIO 2) | žlutá |
| 4 | **SDA** (GPIO 42) | modrá |

> Před zapojením změř multimetrem: s GPIO 47 = HIGH by na pinu 1 uŠupu mělo být **3,3 V** vůči GND.

```
uŠup I2C (JST-SH)          Level shifter
─────────────────          ──────────────
Pin 1  3,3 V  ───────────► LV
Pin 2  GND    ───────────► GND (LV strana)
```

**Alternativa:** externí modul **3,3 V stabilizátor** (např. AMS1117-3.3) napájený z **VCC** (5 V) a GND desky. Vhodné, pokud nechceš brát 3,3 V z uŠupu nebo potřebuješ víc proudu.

#### VCC vs 3,3 V — shrnutí

| Zdroj | Napětí | Kam patří |
|-------|--------|-----------|
| **VCC** (piny desky) | 5 V **vstup** | Napájení desky — **ne** na LV shifteru |
| **3V3** (levá řada, nahoře) | 3,3 V **výstup** | **LV** na level shifteru (doporučeno) |
| **uŠup pin 1** (GPIO 47 = HIGH) | 3,3 V **výstup** | **LV** — alternativa |
| **JYQD 5V** (řídicí port) | 5 V | **HV** na level shifteru |

> **LV** → pin **3V3** na levé straně desky. **HV** → **5V** z JYQD driveru.

- **LV**, **HV** a **GND** musí být na každém modulu zapojené vždy.
- **GND ESP32**, **GND driveru (control)** a **GND baterie (P−)** musí být **společné**.

### Kanály shifteru (1 motor = 4 kanály)

Každý řádek = jeden pár **LVx** + **HVx** na modulu:

| Kanál modulu | LVx ← ESP32 GPIO | HVx → JYQD | Směr dat |
|--------------|------------------|------------|----------|
| **1** (LV1 / HV1) | GPIO PWM | **VR** | ESP → driver |
| **2** (LV2 / HV2) | GPIO digitální | **Z/F** | ESP → driver |
| **3** (LV3 / HV3) | GPIO digitální | **EL** | ESP → driver |
| **4** (LV4 / HV4) | GPIO digitální | **Signal** | driver → ESP |

Bidirectional shifter funguje i pro **PWM na VR** (frekvence 1–20 kHz dle datasheetu driveru).

---

## Doporučené GPIO (ESP32-S3)

### Které piny jsou fyzicky na hlavních řadách

Dle [pinoutu LaskaKit](https://github.com/LaskaKit/ESP32-S3-DEVKit/blob/main/img/ESP32-S3-DEVKIT_pinout.png) — **levá řada** (shora):

`3V3`, `EN`, **GPIO 4**, **5**, **6**, **7**, **15**, **16**, **17**, **18**, 8, 19, 20, `GND`, 46, `VCC`

**GPIO 10 není na hlavních řadách pinů.** Je jen na konektoru **uŠup SPI** jako `CS-10`. Proto pro otáčkoměr motoru 1 použij **GPIO 18** (je na levé řadě, hned pod GPIO 17).

| GPIO | Kde je |
|------|--------|
| **4, 5, 6, 7, 15, 16, 17, 18** | Hlavní řady pinů (levá strana) |
| **10, 11, 12, 13** | Pouze konektor **uŠup SPI** (CS, MOSI, SCK, MISO) |
| **42, 2** | uŠup I2C (+ pravá řada: GPIO 42, 2) |
| **47** | Řízení napájení uŠup (není jako GPIO pin na konektoru) |

### Piny obsazené v projektu — nepoužívat

| GPIO | Použití v projektu |
|------|---------------------|
| 47 | uŠup I2C power |
| 42 | uŠup SDA |
| 2 | uŠup SCL |
| 45 | RGB LED |
| 9 | měření baterie (ADC) |
| 35, 36, 37 | Interní PSRAM (modul N16**R8**) — nefungují jako GPIO |

### Motor 1 (levý) — 1× modul level shifter

| Funkce | ESP32 GPIO | LV strana | HV strana | JYQD |
|--------|------------|-----------|-----------|------|
| Rychlost (PWM) | **GPIO 4** | **LV1** | **HV1** | **VR** |
| Směr | **GPIO 5** | **LV2** | **HV2** | **Z/F** |
| Enable | **GPIO 6** | **LV3** | **HV3** | **EL** |
| Otáčkoměr | **GPIO 18** | **LV4** | **HV4** | **Signal** |
| Reference 3,3 V | — | **LV** ← **3V3** (levá řada, nahoře) | — | — |
| Reference 5 V | — | — | **HV** ← 5V | **5V** |
| Zem | GND | **GND** | **GND** | **GND** |

### Motor 2 (pravý) — druhý modul level shifter

| Funkce | ESP32 GPIO | LV strana | HV strana | JYQD |
|--------|------------|-----------|-----------|------|
| Rychlost (PWM) | **GPIO 7** | **LV1** | **HV1** | **VR** |
| Směr | **GPIO 15** | **LV2** | **HV2** | **Z/F** |
| Enable | **GPIO 16** | **LV3** | **HV3** | **EL** |
| Otáčkoměr | **GPIO 17** | **LV4** | **HV4** | **Signal** |
| Reference 3,3 V | — | **LV** ← **3V3** (levá řada, nahoře) | — | — |
| Reference 5 V | — | — | **HV** ← 5V | **5V** |
| Zem | GND | **GND** | **GND** | **GND** |

> Na **druhém** modulu jsou popisky zase **LV1–LV4** / **HV1–HV4** — jde o stejné názvy kanálů, ne globální číslování přes oba moduly.

---

## Schéma zapojení (1 motor)

```
  ESP32-S3                         Level shifter                    JYQD V7.3E2
  ────────                         ─────────────                    ───────────

  GPIO 4 (PWM) ──────► LV1 ═════════ HV1 ─────────────────────────► VR
  GPIO 5       ──────► LV2 ═════════ HV2 ─────────────────────────► Z/F
  GPIO 6       ──────► LV3 ═════════ HV3 ─────────────────────────► EL
  GPIO 18      ◄────── LV4 ═════════ HV4 ◄───────────────────────── Signal

  3V3 (levá řada, nahoře) ──► LV
  GND          ──────► GND ═════════ GND ─────────────────────────► GND
                       HV  ◄──────── 5V (řídicí port)
```

---

## Zapojení mimo ESP32 (přímo na driver)

Tyto části **level shifter nepotřebují**:

### Power port (napájení motoru)

| JYQD | Zapojení |
|------|----------|
| **P+** | + baterie (12–36 V) |
| **P−** | − baterie → **společné GND** s ESP32 |

### Hall port

| JYQD | Motor |
|------|-------|
| **Ha, Hb, Hc** | Hall fáze A, B, C |
| **GND** | Hall GND |
| **5V** | Hall napájení (výstup z driveru) |

### Motorové fáze

| JYQD | Motor |
|------|-------|
| **MA, MB, MC** | fáze A, B, C |

Pokud motor cuká nebo jede špatným směrem, prohoď dvě fáze (**MA/MB/MC**) nebo Hall vodiče (**Ha/Hb/Hc**) dle datasheetu.

---

## Logika řízení (po level shifteru)

| Pin JYQD | HIGH (povoleno / vpřed) | LOW (vypnuto / vzad) |
|----------|-------------------------|----------------------|
| **EL** | 5 V nebo nechat pull-up (motor povolen) | GND (motor vypnut) |
| **Z/F** | 5 V nebo volno (vpřed) | GND (vzad) |
| **VR** | PWM duty 0–100 % nebo 0,1–5 V (rychlost) | GND = stop |

Doporučený startovní stav firmware:

- **EL** = HIGH (povoleno)
- **Z/F** = HIGH (vpřed)
- **VR** = PWM 0 % (stojí)

---

## Kompletní tabulka vodičů (2 motory)

| Z | Do | Poznámka |
|---|-----|----------|
| ESP32 **3V3** (levá řada, nahoře) | **LV** na obou shifterech | nebo uŠup 3,3 V |
| ESP32 **GND** | **GND** shifterů, JYQD **GND** (control), baterie **P−** | jedna společná zem |
| JYQD **5V** (control) | **HV** na obou shifterech | 5 V reference, malý proud |
| ESP32 GPIO **4** | LV1 shifter 1 → HV1 → JYQD1 **VR** | PWM motor 1 |
| ESP32 GPIO **5** | LV2 shifter 1 → HV2 → JYQD1 **Z/F** | směr motor 1 |
| ESP32 GPIO **6** | LV3 shifter 1 → HV3 → JYQD1 **EL** | enable motor 1 |
| JYQD1 **Signal** | HV4 shifter 1 → LV4 → ESP32 GPIO **18** | otáčky motor 1 |
| ESP32 GPIO **7** | LV1 shifter 2 → HV1 → JYQD2 **VR** | PWM motor 2 |
| ESP32 GPIO **15** | LV2 shifter 2 → HV2 → JYQD2 **Z/F** | směr motor 2 |
| ESP32 GPIO **16** | LV3 shifter 2 → HV3 → JYQD2 **EL** | enable motor 2 |
| JYQD2 **Signal** | HV4 shifter 2 → LV4 → ESP32 GPIO **17** | otáčky motor 2 |
| Baterie +/− | **P+** / **P−** obou driverů | napájení motorů |
| Hall + fáze | každý driver ↔ svůj motor | bez ESP32 |

---

## Bezpečnost a praktické tipy

1. **Nikdy** nepropojuj **5V z driveru** s **VCC** nebo **uŠup 3,3 V** na ESP32.
2. Vždy nejdřív společná **GND**, teprve pak signály.
3. Při délce kabelu driver ↔ motor **> 50 cm** použij **stíněné vodiče** (Hall + fáze).
4. Driver nemá kryt — zajisti **větrání**; nad ~100 W zvaž chladič.
5. **EL → GND** je rychlý nouzový stop (vhodné pro kill switch).
6. Ovládání je **PWM/analogové** — firmware `MotorController` používá LEDC PWM a GPIO.

---

## Kontrolní checklist před prvním spuštěním

- [ ] Společná GND: ESP32, oba drivery (control), baterie
- [ ] Level shifter: **LV** = **3V3** (levá řada nahoře), **HV** = 5V z driveru, **GND** zapojeno
- [ ] Žádný vodič z JYQD **5V** na ESP32
- [ ] Hall kabel a fáze motoru na správném driveru
- [ ] Napětí baterie v rozsahu **12–36 V**
- [ ] GPIO 47, 42, 2, 45, 9 nepoužita pro motory
- [ ] PWM frekvence pro **VR** nastavena v rozsahu **1–20 kHz**

---

## Související firmware

`MotorController` ovládá JYQD přes **PWM + GPIO** dle pinů v tomto návodu:

| Motor | VR (PWM) | Z/F | EL | Signal |
|-------|----------|-----|-----|--------|
| 1 | GPIO 4 | GPIO 5 | GPIO 6 | GPIO 18 |
| 2 | GPIO 7 | GPIO 15 | GPIO 16 | GPIO 17 |

API v kódu:

```cpp
MotorController motors;
motors.begin();
motors.setSpeed(1, 50);   // motor 1, 50 % vpřed
motors.setSpeed(2, -30);  // motor 2, 30 % vzad
motors.stop(0);           // oba stop
motors.emergencyStop();   // PWM 0 + EL vypnuto
```

PWM frekvence: **10 kHz**. Po `begin()` motory stojí (VR = 0 %, EL povoleno).
