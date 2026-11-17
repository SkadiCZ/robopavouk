// =====================================================================
//  ROBOPAVOUK – firmware pro ESP32-C3 SuperMini
//  18x MG90S přes 2x PCA9685, ultrazvukový senzor HC-SR04
//
//  Ovládání: sériová linka 115200 Bd (napiš "help") nebo WiFi:
//  připoj se k síti "Robopavouk" a otevři http://192.168.4.1
//
//  Deska v Arduino IDE: "ESP32C3 Dev Module", USB CDC On Boot: Enabled.
//  Žádné další knihovny nejsou potřeba.
// =====================================================================
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "config.h"
#include "motion.h"
#include "webpage.h"

using rs::Motion;

Motion motion;
Preferences prefs;
WebServer server(80);

// ---------------------------------------------------------------------
//  PCA9685
// ---------------------------------------------------------------------
static void pcaWrite(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr); Wire.write(reg); Wire.write(val); Wire.endTransmission();
}
static bool pcaInit(uint8_t addr) {
  Wire.beginTransmission(addr);
  if (Wire.endTransmission() != 0) return false;
  const uint8_t prescale = (uint8_t)(PCA_OSC_HZ / (4096.0f * 50.0f) + 0.5f) - 1;   // 50 Hz
  pcaWrite(addr, 0x00, 0x10);        // MODE1: sleep
  pcaWrite(addr, 0xFE, prescale);
  pcaWrite(addr, 0x01, 0x04);        // MODE2: totem pole
  pcaWrite(addr, 0x00, 0x20);        // MODE1: auto-increment, probudit
  delay(2);
  pcaWrite(addr, 0x00, 0xA0);        // restart
  return true;
}
// pulz v mikrosekundách; us = 0 výstup vypne (servo povolí)
static void pcaPulse(uint8_t addr, uint8_t ch, uint16_t us) {
  uint16_t on = 0, off = 0x1000;     // bit 12 v OFF = výstup trvale vypnutý
  if (us > 0) {
    const uint16_t w = (uint16_t)((uint32_t)us * 4096UL / 20000UL);
    on = (uint16_t)(ch * 227) & 0x0FFF;          // rozložení náběhů, menší proudové špičky
    off = (on + w) & 0x0FFF;
  }
  Wire.beginTransmission(addr);
  Wire.write(0x06 + 4 * ch);
  Wire.write(on & 0xFF); Wire.write(on >> 8); Wire.write(off & 0xFF); Wire.write(off >> 8);
  Wire.endTransmission();
}

// ---------------------------------------------------------------------
//  Serva
// ---------------------------------------------------------------------
enum OutMode { OUT_RELAX, OUT_CENTER, OUT_RUN, OUT_TEST };
OutMode outMode = OUT_RELAX;
int8_t servoDir[6][3];
float servoTrim[6][3];           // jemné doladění ve stupních
float servoCur[6][3];            // právě posílaný úhel serva (0-180)
bool servoValid = false;         // servoCur odpovídá skutečné poloze
int testLeg = -1, testJoint = 0; float testDelta = 0;
bool pcaOk[2] = { false, false };

static const char *JOINT_NAMES[3] = { "coxa", "stehno", "koleno" };

static void loadCal() {
  memcpy(servoDir, SERVO_DIR_DEFAULT, sizeof servoDir);
  memset(servoTrim, 0, sizeof servoTrim);
  prefs.begin("pavouk", true);
  if (prefs.getBytesLength("dir") == sizeof servoDir) prefs.getBytes("dir", servoDir, sizeof servoDir);
  if (prefs.getBytesLength("trim") == sizeof servoTrim) prefs.getBytes("trim", servoTrim, sizeof servoTrim);
  prefs.end();
}
static void saveCal() {
  prefs.begin("pavouk", false);
  prefs.putBytes("dir", servoDir, sizeof servoDir);
  prefs.putBytes("trim", servoTrim, sizeof servoTrim);
  prefs.end();
}
// úhel kloubu z modelu -> úhel serva (90° = montážní póza)
static float servoAngle(int leg, int joint, float model) {
  const float neutral = joint == 0 ? rs::NEUTRAL.coxa : (joint == 1 ? rs::NEUTRAL.hip : rs::NEUTRAL.knee);
  return constrain(90.0f + servoDir[leg][joint] * (model - neutral) + servoTrim[leg][joint], 0.0f, 180.0f);
}
static void servoWrite(int leg, int joint, float deg) {
  const uint16_t us = (uint16_t)(SERVO_US_MIN + (SERVO_US_MAX - SERVO_US_MIN) * deg / 180.0f);
  pcaPulse(leg < 3 ? PCA_LEFT : PCA_RIGHT, (leg % 3) * 3 + joint, us);
}
static void servosRelax() {
  for (int leg = 0; leg < 6; leg++) for (int j = 0; j < 3; j++) pcaPulse(leg < 3 ? PCA_LEFT : PCA_RIGHT, (leg % 3) * 3 + j, 0);
  servoValid = false;
}
// pošle cílové úhly všem servům, s omezením rychlosti
static void servosOutput(float dt) {
  if (outMode == OUT_RELAX) return;
  for (int leg = 0; leg < 6; leg++) {
    const float m[3] = { motion.out[leg].coxa, motion.out[leg].hip, motion.out[leg].knee };
    for (int j = 0; j < 3; j++) {
      float target;
      if (outMode == OUT_RUN) target = servoAngle(leg, j, m[j]);
      else {
        target = constrain(90.0f + servoTrim[leg][j], 0.0f, 180.0f);
        if (outMode == OUT_TEST && leg == testLeg && j == testJoint)
          target = constrain(target + servoDir[leg][j] * testDelta, 0.0f, 180.0f);
      }
      if (!servoValid) servoCur[leg][j] = target;
      else servoCur[leg][j] = rs::approach(servoCur[leg][j], target, SERVO_MAX_DPS * dt);
      servoWrite(leg, j, servoCur[leg][j]);
    }
  }
  servoValid = true;
}

// ---------------------------------------------------------------------
//  Ultrazvukový senzor HC-SR04 (měření přes přerušení, nic neblokuje)
// ---------------------------------------------------------------------
volatile uint32_t echoStart = 0, echoDur = 0;
volatile bool echoDone = false;
float distCm = 999;              // vyhlazená vzdálenost; 999 = nic v dosahu
float distHist[3] = { 999, 999, 999 };
uint32_t sonarLast = 0;

void IRAM_ATTR echoIsr() {
  if (digitalRead(PIN_ECHO)) echoStart = micros();
  else { echoDur = micros() - echoStart; echoDone = true; }
}
static void sonarTick(uint32_t now) {
#if SONAR_ENABLED
  if (now - sonarLast < 60) return;
  sonarLast = now;
  float d = 999;
  if (echoDone) { d = echoDur / 58.0f; if (d < 2 || d > 300) d = 999; }
  echoDone = false;
  distHist[0] = distHist[1]; distHist[1] = distHist[2]; distHist[2] = d;
  // medián ze tří posledních měření
  const float a = distHist[0], b = distHist[1], c = distHist[2];
  distCm = max(min(a, b), min(max(a, b), c));
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10); digitalWrite(PIN_TRIG, LOW);
#endif
}

// ---------------------------------------------------------------------
//  Chování se senzorem
// ---------------------------------------------------------------------
enum AutoMode { A_OFF, A_EXPLORE, A_FOLLOW };
AutoMode autoMode = A_OFF;
bool guardOn = true;             // hlídání překážky při chůzi vpřed
bool startleOn = true;           // lekání
enum ExploreState { E_FWD, E_BACK, E_TURN };
ExploreState exState = E_FWD;
uint32_t exUntil = 0, startleCooldown = 0, backUntil = 0;
float exTurnDir = 1;

static void behaviourTick(uint32_t now) {
#if SONAR_ENABLED
  if (!motion.standing()) return;
  // po leknutí kousek couvne
  if (backUntil) {
    if (now < backUntil) { motion.drive(-1, 0, 0); return; }
    backUntil = 0; motion.stop();
  }
  const bool fwd = motion.mode == Motion::WALK && motion.cmd[0] > 0;
  if (autoMode == A_EXPLORE) {
    switch (exState) {
      case E_FWD:
        if (distCm < EXPLORE_TURN_CM) { exState = E_BACK; exUntil = now + 1200; motion.drive(-1, 0, 0); }
        else motion.drive(1, 0, 0);
        break;
      case E_BACK:
        if (now >= exUntil) { exState = E_TURN; exUntil = now + 1200 + random(1400); exTurnDir = random(2) ? 1 : -1; motion.drive(0, 0, exTurnDir); }
        break;
      case E_TURN:
        if (now >= exUntil) { exState = E_FWD; motion.drive(1, 0, 0); }
        break;
    }
    return;
  }
  if (autoMode == A_FOLLOW) {
    if (distCm < FOLLOW_NEAR_CM) motion.drive(-1, 0, 0);
    else if (distCm > FOLLOW_FAR_CM && distCm < FOLLOW_MAX_CM) motion.drive(1, 0, 0);
    else motion.stop();
    return;
  }
  if (guardOn && fwd && distCm < GUARD_CM) {
    motion.stop();
    if (startleOn) motion.start(Motion::S_STARTLE);
    return;
  }
  if (startleOn && motion.mode == Motion::STAND && distCm < STARTLE_CM && now > startleCooldown) {
    startleCooldown = now + 4000;
    if (motion.start(Motion::S_STARTLE)) backUntil = now + 1400 + 1200;   // po leknutí 1,2 s couvá
  }
#endif
}

// ---------------------------------------------------------------------
//  Příkazy
// ---------------------------------------------------------------------
uint32_t standUpAt = 0;          // odložený start vstávání (serva nejdřív najedou do lehu)
uint32_t restSince = 0;

static int legIndex(const String &s) {
  for (int i = 0; i < 6; i++) if (s.equalsIgnoreCase(rs::LEGS[i].id)) return i;
  return -1;
}
static int jointIndex(const String &s) {
  if (s == "coxa" || s == "c") return 0;
  if (s == "stehno" || s == "s") return 1;
  if (s == "koleno" || s == "k") return 2;
  return -1;
}
static String statusText() {
  static const char *MODES[] = { "lezi", "stoji", "jde", "sekvence" };
  String s = String("stav: ") + MODES[motion.mode];
  s += outMode == OUT_RELAX ? ", serva vypnuta" : (outMode == OUT_CENTER ? ", serva na stredu" : (outMode == OUT_TEST ? ", test serva" : ""));
  s += String(", chuze ") + rs::GAITS[motion.gait].name;
  s += String(", vzdalenost ") + (distCm > 400 ? String("-") : String(distCm, 0) + " cm");
  s += autoMode == A_EXPLORE ? ", pruzkum" : (autoMode == A_FOLLOW ? ", nasleduj" : "");
  s += String(", PCA ") + (pcaOk[0] ? "L ok" : "L CHYBI") + (pcaOk[1] ? " P ok" : " P CHYBI");
  return s;
}
static const char HELP[] PROGMEM =
  "vstan, lehni, stop, stav\n"
  "vpred(w) vzad(s) vlevo(a) vpravo(d) otoc_l(q) otoc_p(e)\n"
  "chuze tripod|ripple|vlna\n"
  "mavej tanec uklona utok podej zavrt kliky pochod krouzeni\n"
  "pruzkum, nasleduj, auto_stop, hlidej 0|1, lek 0|1, zivy 0|1\n"
  "rychlost <mm/s>, frekv <Hz>, krok <mm>, vyska <mm>, rozkrok <mm>\n"
  "stred (vsechna serva na 90, pro montaz), uvolni (vypne serva)\n"
  "test <LP|LS|LZ|PP|PS|PZ> <coxa|stehno|koleno> <stupne>\n"
  "smer <noha> <kloub> (otoci smer serva), trim <noha> <kloub> <stupne>, kalibrace (vypis)\n";

static bool needStanding(String &r) {
  if (outMode != OUT_RUN || motion.mode == Motion::REST) { r = "nejdriv 'vstan'"; return false; }
  return true;
}
static String handleCommand(String line) {
  line.trim(); line.toLowerCase();
  if (!line.length()) return "";
  String a[4]; int n = 0;
  while (line.length() && n < 4) {
    const int sp = line.indexOf(' ');
    a[n++] = sp < 0 ? line : line.substring(0, sp);
    if (sp < 0) break;
    line = line.substring(sp + 1); line.trim();
  }
  const String &c = a[0];
  String r = "ok";

  if (c == "help" || c == "?") return String(FPSTR(HELP));
  if (c == "stav") return statusText();
  if (c == "vstan") {
    if (motion.mode != Motion::REST) return "uz stoji";
    outMode = OUT_RUN; servoValid = false; standUpAt = millis() + 700;
    return "vstavam";
  }
  if (c == "lehni") {
    autoMode = A_OFF; backUntil = 0;
    if (motion.mode == Motion::WALK) motion.stop();
    return motion.start(Motion::S_SITDOWN) ? "leham" : "ted to nejde";
  }
  if (c == "uvolni") { if (motion.mode != Motion::REST) return "nejdriv 'lehni'"; outMode = OUT_RELAX; servosRelax(); return "serva vypnuta"; }
  if (c == "stred") { if (motion.mode != Motion::REST) return "nejdriv 'lehni'"; outMode = OUT_CENTER; return "vsechna serva na 90"; }
  if (c == "test") {
    const int l = legIndex(a[1]), j = jointIndex(a[2]);
    if (motion.mode != Motion::REST) return "nejdriv 'lehni'";
    if (l < 0 || j < 0) return "pouziti: test LP coxa 20";
    testLeg = l; testJoint = j; testDelta = constrain(a[3].toFloat(), -60.0f, 60.0f); outMode = OUT_TEST;
    return String("test ") + rs::LEGS[l].id + " " + JOINT_NAMES[j] + " " + String(testDelta, 0) +
           (j == 0 ? "  (+ = noha se toci proti smeru hodin pri pohledu shora)" : (j == 1 ? "  (+ = stehno dolu)" : "  (+ = koleno se vic ohne, spicka k telu)"));
  }
  if (c == "smer") {
    const int l = legIndex(a[1]), j = jointIndex(a[2]);
    if (l < 0 || j < 0) return "pouziti: smer LP coxa";
    servoDir[l][j] = -servoDir[l][j]; saveCal();
    return String("smer ") + rs::LEGS[l].id + " " + JOINT_NAMES[j] + " = " + String((int)servoDir[l][j]);
  }
  if (c == "trim") {
    const int l = legIndex(a[1]), j = jointIndex(a[2]);
    if (l < 0 || j < 0) return "pouziti: trim LP koleno -3";
    servoTrim[l][j] = constrain(a[3].toFloat(), -25.0f, 25.0f); saveCal();
    return String("trim ") + rs::LEGS[l].id + " " + JOINT_NAMES[j] + " = " + String(servoTrim[l][j], 1);
  }
  if (c == "kalibrace") {
    String s;
    for (int l = 0; l < 6; l++) {
      s += rs::LEGS[l].id; s += ":";
      for (int j = 0; j < 3; j++) { s += " "; s += JOINT_NAMES[j]; s += servoDir[l][j] > 0 ? " +" : " -"; s += " trim "; s += String(servoTrim[l][j], 1); }
      s += "\n";
    }
    return s;
  }
  if (c == "chuze") {
    if (a[1] == "tripod") motion.gait = 0; else if (a[1] == "ripple") motion.gait = 1;
    else if (a[1] == "vlna" || a[1] == "wave") motion.gait = 2; else return "chuze tripod|ripple|vlna";
    return String("chuze ") + rs::GAITS[motion.gait].name;
  }
  if (c == "rychlost") { motion.P.speed = constrain(a[1].toFloat(), 5.0f, 60.0f); return r; }
  if (c == "frekv")    { motion.P.freq = constrain(a[1].toFloat(), 0.3f, 1.8f); return r; }
  if (c == "krok")     { motion.P.lift = constrain(a[1].toFloat(), 6.0f, 22.0f); return r; }
  if (c == "vyska")    { motion.P.height = constrain(a[1].toFloat(), 45.0f, 68.0f); return r; }
  if (c == "rozkrok")  { motion.P.spread = constrain(a[1].toFloat(), 62.0f, 78.0f); return r; }
  if (c == "hlidej")   { guardOn = a[1].toInt() != 0; return r; }
  if (c == "lek")      { startleOn = a[1].toInt() != 0; return r; }
  if (c == "zivy")     { motion.alive = a[1].toInt() != 0; return r; }
  if (c == "auto_stop") { autoMode = A_OFF; backUntil = 0; motion.stop(); return r; }

  // vše ostatní potřebuje, aby robot stál
  if (!needStanding(r)) return r;
  if (c == "stop") { autoMode = A_OFF; backUntil = 0; motion.stop(); return r; }
  if (c == "pruzkum")  { autoMode = A_EXPLORE; exState = E_FWD; return "pruzkum (zastavis 'stop')"; }
  if (c == "nasleduj") { autoMode = A_FOLLOW; return "nasleduju ruku (zastavis 'stop')"; }

  struct { const char *name, *alias; float vx, vy, w; } const moves[] = {
    { "vpred", "w", 1, 0, 0 }, { "vzad", "s", -1, 0, 0 }, { "vlevo", "a", 0, 1, 0 }, { "vpravo", "d", 0, -1, 0 },
    { "otoc_l", "q", 0, 0, 1 }, { "otoc_p", "e", 0, 0, -1 } };
  for (auto &m : moves) if (c == m.name || c == m.alias) {
    if (motion.busy()) return "pockej, az dokonci sekvenci";
    autoMode = A_OFF; motion.drive(m.vx, m.vy, m.w); return r;
  }
  struct { const char *name; Motion::Seq s; } const seqs[] = {
    { "mavej", Motion::S_WAVE }, { "tanec", Motion::S_DANCE }, { "uklona", Motion::S_BOW }, { "utok", Motion::S_ATTACK },
    { "podej", Motion::S_SHAKE }, { "zavrt", Motion::S_WIGGLE }, { "kliky", Motion::S_PUSHUPS }, { "pochod", Motion::S_MARCH },
    { "krouzeni", Motion::S_SWAY } };
  for (auto &q : seqs) if (c == q.name) {
    autoMode = A_OFF; backUntil = 0;
    if (motion.mode == Motion::WALK) motion.stop();
    return motion.start(q.s) ? r : "pockej, az dokonci sekvenci";
  }
  return "neznamy prikaz, napis 'help'";
}

// ---------------------------------------------------------------------
//  Web
// ---------------------------------------------------------------------
static void webSetup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  server.on("/", []() { server.send_P(200, "text/html; charset=utf-8", PAGE); });
  server.on("/c", []() { server.send(200, "text/plain; charset=utf-8", handleCommand(server.arg("x"))); });
  server.begin();
}

// ---------------------------------------------------------------------
//  setup + loop
// ---------------------------------------------------------------------
String serialLine;
uint32_t lastTick = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  loadCal();
  pcaOk[0] = pcaInit(PCA_LEFT); pcaOk[1] = pcaInit(PCA_RIGHT);
  servosRelax();                  // po zapnutí se nic nehýbe, čeká na "vstan"
#if SONAR_ENABLED
  pinMode(PIN_TRIG, OUTPUT); digitalWrite(PIN_TRIG, LOW);
  pinMode(PIN_ECHO, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_ECHO), echoIsr, CHANGE);
#endif
  webSetup();
  motion.update(0.02f);
  lastTick = millis();
  Serial.println("Robopavouk pripraven. Napis 'help'.");
  Serial.println(statusText());
}

void loop() {
  server.handleClient();
  while (Serial.available()) {
    const char ch = (char)Serial.read();
    if (ch == '\n' || ch == '\r') { if (serialLine.length()) { Serial.println(handleCommand(serialLine)); serialLine = ""; } }
    else if (serialLine.length() < 60) serialLine += ch;
  }
  const uint32_t now = millis();
  if (now - lastTick < 20) return;             // 50 Hz
  const float dt = min((now - lastTick) / 1000.0f, 0.05f);
  lastTick = now;

  sonarTick(now);
  if (standUpAt && now >= standUpAt) { standUpAt = 0; motion.start(Motion::S_STANDUP); }
  if (outMode == OUT_RUN) behaviourTick(now);
  motion.update(dt);
  servosOutput(dt);

  // po ulehnutí serva po chvíli vypnout, ať se nehřejí
  if (outMode == OUT_RUN && motion.mode == Motion::REST && !standUpAt) {
    if (!restSince) restSince = now;
    else if (now - restSince > 800) { outMode = OUT_RELAX; servosRelax(); restSince = 0; }
  } else restSince = 0;
}
