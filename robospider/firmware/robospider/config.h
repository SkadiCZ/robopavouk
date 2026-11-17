// =====================================================================
//  Robopavouk – nastavení (piny, adresy, serva)
// =====================================================================
#pragma once

// ---------- piny ESP32-C3 SuperMini ----------
#define PIN_SDA   8      // I2C k oběma PCA9685
#define PIN_SCL   9
#define PIN_TRIG  4      // HC-SR04 TRIG
#define PIN_ECHO  5      // HC-SR04 ECHO  !!! přes dělič 1k/2k na 3,3 V (ECHO dává 5 V) !!!

// ---------- PCA9685 ----------
#define PCA_LEFT   0x40  // levé nohy  (kanály 0-8)
#define PCA_RIGHT  0x41  // pravé nohy (kanály 0-8), zapájená propojka A0
// kanály na každé desce: přední noha 0,1,2 / prostřední 3,4,5 / zadní 6,7,8  (coxa, stehno, koleno)
#define PCA_OSC_HZ 25000000.0f   // když serva nedojíždějí na správný úhel, zkus 26-27 MHz

// ---------- serva MG90S ----------
#define SERVO_US_MIN  500    // šířka pulzu pro 0°
#define SERVO_US_MAX  2500   // šířka pulzu pro 180°
#define SERVO_MAX_DPS 500.0f // nejvyšší rychlost, kterou firmware servu dovolí (°/s)

// směr otáčení serv: +1 nebo -1 pro [noha][coxa, stehno, koleno]
// pořadí nohou: LP, LS, LZ, PP, PS, PZ. Je to odhad podle modelu, ověř příkazem "test"
// a případně otoč příkazem "smer" (uloží se do paměti a přepíše tuto tabulku).
static const int8_t SERVO_DIR_DEFAULT[6][3] = {
  { -1,  1,  1 }, { -1,  1,  1 }, { -1,  1,  1 },     // levé
  { -1, -1, -1 }, { -1, -1, -1 }, { -1, -1, -1 },     // pravé (zrcadlově)
};

// ---------- WiFi ovládání ----------
#define WIFI_SSID "Robopavouk"
#define WIFI_PASS "pavouk1234"     // aspoň 8 znaků; změň si ho

// ---------- ultrazvukový senzor ----------
#define SONAR_ENABLED   1
#define GUARD_CM        14    // při chůzi vpřed zastaví, když je překážka blíž
#define STARTLE_CM      8     // když stojí a něco se přiblíží blíž, lekne se
#define EXPLORE_TURN_CM 20    // při průzkumu se před překážkou otočí
#define FOLLOW_NEAR_CM  10    // "následuj": blíž než tohle couvá
#define FOLLOW_FAR_CM   16    // dál než tohle jde vpřed
#define FOLLOW_MAX_CM   50    // dál než tohle ruku nevidí a stojí
