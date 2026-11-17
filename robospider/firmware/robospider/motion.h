// =====================================================================
//  Robopavouk – pohybové jádro (kinematika, chůze, sekvence)
//  Čisté C++ bez závislosti na Arduinu, aby šlo testovat i na počítači.
//  Rozměry a výpočty jsou stejné jako v 3D modelu a v simulátoru.
// =====================================================================
#pragma once
#include <math.h>

namespace rs {

const float PI_F = 3.14159265f, D2R = PI_F / 180.0f, R2D = 180.0f / PI_F;

// ---------- rozměry z robospider.scad (mm) ----------
const float RF = 43.5f;      // osa coxa -> osa stehna (radiálně)
const float ZF = 12.0f;      // osa stehna nad rovinou coxa serv
const float L1 = 46.0f;      // stehno
const float TIPX = 70.52f, TIPZ = -44.14f;   // špička bérce v rámci kolene
const float DOFF = 22.0f;    // boční posun roviny nohy
const float FOOT_R = 3.5f;
static const float LT = sqrtf(TIPX * TIPX + TIPZ * TIPZ);
static const float G0 = atan2f(TIPZ, TIPX);

struct Angles { float coxa, hip, knee; bool ok; };
// póza při montáži: všechna serva na 90°
const Angles NEUTRAL = { 0, -20, 90, true };
const float LIM_COXA[2] = { -60, 60 }, LIM_HIP[2] = { -90, 60 }, LIM_KNEE[2] = { -40, 110 };

struct LegDef { const char *id; float cx, cy, h; int s; };
// pořadí nohou: 0 LP, 1 LS, 2 LZ (levé: přední, střední, zadní), 3 PP, 4 PS, 5 PZ (pravé)
const LegDef LEGS[6] = {
  { "LP",  65,  45,   45,  1 }, { "LS",   0,  60,   90,  1 }, { "LZ", -65,  45,  135,  1 },
  { "PP",  65, -45,  -45, -1 }, { "PS",   0, -60,  -90, -1 }, { "PZ", -65, -45, -135, -1 },
};

struct Gait { const char *name; float duty; float off[6]; };
const Gait GAITS[3] = {
  { "tripod", 1.0f / 2, { 0, 0.5f, 0, 0.5f, 0, 0.5f } },
  { "ripple", 2.0f / 3, { 2.0f / 3, 1.0f / 3, 0, 1.0f / 6, 5.0f / 6, 0.5f } },
  { "wave",   5.0f / 6, { 2.0f / 6, 1.0f / 6, 0, 5.0f / 6, 4.0f / 6, 3.0f / 6 } },
};

inline float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
inline float smooth(float x) { x = clampf(x, 0, 1); return x * x * (3 - 2 * x); }
inline float approach(float a, float b, float r) { return a + clampf(b - a, -r, r); }
// náběh a doběh: 0 -> 1 za tIn, drží, od tOut jde za tLen zpět na 0
inline float env(float t, float tIn, float tOut, float tLen) { return smooth(t / tIn) * (1 - smooth((t - tOut) / tLen)); }

// ---------- inverzní kinematika: bod špičky v rámci nohy -> úhly kloubů ----------
inline Angles ik(const LegDef &leg, const float p[3]) {
  Angles a; a.ok = true;
  float rho = hypotf(p[0], p[1]);
  const float d = leg.s * DOFF;
  if (rho < fabsf(d) + 1) { rho = fabsf(d) + 1; a.ok = false; }
  const float coxa = atan2f(p[1], p[0]) - asinf(d / rho);
  const float u = sqrtf(rho * rho - d * d);
  const float dx = u - RF, dz = p[2] - ZF;
  float D = hypotf(dx, dz);
  const float dmin = fabsf(L1 - LT) + 0.5f, dmax = L1 + LT - 0.5f;
  if (D < dmin) { D = dmin; a.ok = false; }
  if (D > dmax) { D = dmax; a.ok = false; }
  const float c = (D * D - L1 * L1 - LT * LT) / (2 * L1 * LT);
  const float psi = -acosf(clampf(c, -1, 1));
  const float e = atan2f(dz, dx) - atan2f(LT * sinf(psi), L1 + LT * cosf(psi));
  a.coxa = coxa * R2D; a.hip = -e * R2D; a.knee = (G0 - psi) * R2D;
  return a;
}
// přímá kinematika (jen pro kontrolu)
inline void fk(const LegDef &leg, const Angles &a, float out[3]) {
  const float c = a.coxa * D2R, e = -a.hip * D2R, k = a.knee * D2R, t = e - k + G0;
  const float u = RF + L1 * cosf(e) + LT * cosf(t), d = leg.s * DOFF;
  out[0] = u * cosf(c) - d * sinf(c); out[1] = u * sinf(c) + d * cosf(c);
  out[2] = ZF + L1 * sinf(e) + LT * sinf(t);
}

const float H_REST = 20.0f;     // výška v lehu: břicho (kolébka baterie) leží na zemi
const float EXT_REST = 24.0f;   // o kolik jsou nohy v lehu víc roztažené
// pořadí, v jakém nohy po jedné přešlapují při vstávání a lehání (index = noha)
const int ORDER[6] = { 0, 2, 4, 1, 3, 5 };

// =====================================================================
class Motion {
public:
  struct Params {
    float speed = 45;    // mm/s
    float freq = 1.2f;   // kroků za sekundu
    float lift = 16;     // výška kroku (mm)
    float height = 57;   // výška roviny coxa serv nad zemí (mm)
    float spread = 69;   // vzdálenost chodidla od osy coxa (mm)
  } P;

  enum Mode { REST, STAND, WALK, SEQ };
  enum Seq { S_NONE, S_STANDUP, S_SITDOWN, S_WAVE, S_DANCE, S_BOW, S_ATTACK, S_SHAKE, S_WIGGLE,
             S_PUSHUPS, S_MARCH, S_SWAY, S_STARTLE, S_COUNT };

  Mode mode = REST;
  Seq seq = S_NONE;
  int gait = 0;
  bool alive = true;                 // "dýchání" při stání
  float cmd[3] = { 0, 0, 0 };        // požadovaný směr: vpřed, vlevo, otáčení (-1..1)
  float v[3] = { 0, 0, 0 };          // skutečná rychlost (mm/s, mm/s, °/s)
  float phase = 0, liftScale = 0, t = 0, seqT = 0, idleT = 0;
  float hCur = H_REST;               // aktuální výška
  float ext[6] = { EXT_REST, EXT_REST, EXT_REST, EXT_REST, EXT_REST, EXT_REST };
  float freqEff = 0, vmaxEff = 0;

  // výstupy posledního kroku
  Angles out[6];
  bool onGround[6];
  float foot[6][3];                  // chodidla v rámci těla (bez naklopení)
  float shift[3], rot[3];

  static float seqDuration(Seq s) {
    switch (s) {
      case S_STANDUP: return 1.8f + 6 * 0.4f + 0.2f;
      case S_SITDOWN: return 6 * 0.4f + 1.8f + 0.2f;
      case S_WAVE: return 7.6f;   case S_DANCE: return 10.0f; case S_BOW: return 3.4f;
      case S_ATTACK: return 8.4f; case S_SHAKE: return 7.0f;  case S_WIGGLE: return 2.2f;
      case S_PUSHUPS: return 5.4f; case S_MARCH: return 4.5f; case S_SWAY: return 6.0f;
      case S_STARTLE: return 1.4f;
      default: return 0;
    }
  }
  bool busy() const { return mode == SEQ; }
  bool standing() const { return mode == STAND || mode == WALK; }

  // spustí sekvenci; vrací false, když to teď nejde
  bool start(Seq s) {
    if (mode == SEQ) return false;
    if (s == S_STANDUP) { if (mode != REST) return false; }
    else if (mode == REST) return false;
    cmd[0] = cmd[1] = cmd[2] = 0;
    seq = s; seqT = 0; mode = SEQ;
    return true;
  }
  void drive(float vx, float vy, float w) {
    if (mode == SEQ || mode == REST) return;
    cmd[0] = vx; cmd[1] = vy; cmd[2] = w;
    mode = (vx != 0 || vy != 0 || w != 0) ? WALK : STAND;
  }
  void stop() { drive(0, 0, 0); }

  // jeden krok výpočtu, dt v sekundách
  void update(float dt) {
    t += dt;
    const Gait &g = GAITS[gait];
    const bool walking = mode == WALK;
    // přenos nohy musí trvat aspoň 0,28 s (MG90S) a krok je omezený
    float freq = fminf(P.freq, (1 - g.duty) / 0.28f);
    freqEff = freq;
    const float strideMax = g.duty > 0.8f ? 30.0f : 44.0f;   // vlnová chůze: kratší krok, jinak koleno naráží na mez
    const float vmax = fminf(P.speed, strideMax * freq / g.duty), wmax = vmax * 0.55f;
    vmaxEff = vmax;
    const float tgt[3] = { walking ? cmd[0] * vmax : 0, walking ? cmd[1] * vmax : 0, walking ? cmd[2] * wmax : 0 };
    const float acc = 90 * dt;
    v[0] = approach(v[0], tgt[0], acc); v[1] = approach(v[1], tgt[1], acc); v[2] = approach(v[2], tgt[2], acc * 1.2f);
    const bool moving = fabsf(v[0]) + fabsf(v[1]) + fabsf(v[2]) > 0.5f;
    const bool wantMove = walking && (tgt[0] != 0 || tgt[1] != 0 || tgt[2] != 0);

    for (int i = 0; i < 3; i++) { shift[i] = 0; rot[i] = 0; }
    float legOff[6][2] = {}, legLift[6] = {}, blend[6] = {};
    Angles ovr[6] = {};
    float march = 0;

    if (mode == SEQ) {
      seqT += dt;
      runSeq(seqT, legOff, legLift, ovr, blend, march);
      if (seqT >= seqDuration(seq)) {
        mode = (seq == S_SITDOWN) ? REST : STAND;
        seq = S_NONE; idleT = 0;
      }
    } else if (mode == STAND) {
      hCur = approach(hCur, P.height, 25 * dt);
      idleT += dt;
      if (alive && !moving && idleT > 2) {            // dýchání a rozhlížení
        const float a = smooth((idleT - 2) / 2);
        shift[2] += 2.0f * a * sinf(idleT * 2 * PI_F * 0.25f);
        rot[2] += 2.5f * a * sinf(idleT * 2 * PI_F * 0.08f);
      }
    } else if (mode == WALK) {
      hCur = approach(hCur, P.height, 25 * dt);
      idleT = 0;
    }

    liftScale = approach(liftScale, (moving || wantMove || march > 0) ? 1 : 0, dt * 3);
    if (liftScale > 0 || moving) { phase += dt * freq; phase -= floorf(phase); }

    // matice naklopení těla R = Rz * Ry * Rx
    const float ca = cosf(rot[0] * D2R), sa = sinf(rot[0] * D2R), cb = cosf(rot[1] * D2R), sb = sinf(rot[1] * D2R),
                cc = cosf(rot[2] * D2R), sc = sinf(rot[2] * D2R);
    const float R[3][3] = {
      { cc * cb, cc * sb * sa - sc * ca, cc * sb * ca + sc * sa },
      { sc * cb, sc * sb * sa + cc * ca, sc * sb * ca - cc * sa },
      { -sb,     cb * sa,                cb * ca } };

    const float st[3] = { v[0] * g.duty / freq, v[1] * g.duty / freq, v[2] * D2R * g.duty / freq };
    for (int i = 0; i < 6; i++) {
      const LegDef &L = LEGS[i];
      const float ch = cosf(L.h * D2R), sh = sinf(L.h * D2R);
      // neutrální bod chodidla v rámci těla
      const float nl[3] = { P.spread + ext[i], L.s * DOFF, -(hCur - FOOT_R) };
      const float nb[3] = { L.cx + nl[0] * ch - nl[1] * sh, L.cy + nl[0] * sh + nl[1] * ch, nl[2] };
      float ph = phase + g.off[i]; ph -= floorf(ph);
      float k, lift = 0; bool og = true;
      if (ph < g.duty) k = 0.5f - ph / g.duty;
      else {
        const float u = (ph - g.duty) / (1 - g.duty);
        k = -0.5f + smooth(u);
        lift = sinf(PI_F * u) * P.lift * liftScale;
        if (lift > 1) og = false;
      }
      const float f[3] = { nb[0] + (st[0] - st[2] * nb[1]) * k + legOff[i][0],
                           nb[1] + (st[1] + st[2] * nb[0]) * k + legOff[i][1],
                           nb[2] + lift + legLift[i] };
      if (legLift[i] > 1) og = false;
      foot[i][0] = f[0]; foot[i][1] = f[1]; foot[i][2] = f[2];
      // do rámce naklopeného těla: fb = R^T * (f - shift)
      const float q[3] = { f[0] - shift[0], f[1] - shift[1], f[2] - shift[2] };
      const float fb[3] = { R[0][0] * q[0] + R[1][0] * q[1] + R[2][0] * q[2],
                            R[0][1] * q[0] + R[1][1] * q[1] + R[2][1] * q[2],
                            R[0][2] * q[0] + R[1][2] * q[1] + R[2][2] * q[2] };
      const float x = fb[0] - L.cx, y = fb[1] - L.cy;
      const float pl[3] = { x * ch + y * sh, -x * sh + y * ch, fb[2] };
      Angles a = ik(L, pl);
      if (blend[i] > 0) {
        a.coxa += (ovr[i].coxa - a.coxa) * blend[i]; a.hip += (ovr[i].hip - a.hip) * blend[i];
        a.knee += (ovr[i].knee - a.knee) * blend[i];
        if (blend[i] > 0.02f) og = false;
      }
      // pojistka: nikdy mimo mechanické meze modelu
      const float c0 = a.coxa, h0 = a.hip, k0 = a.knee;
      a.coxa = clampf(a.coxa, LIM_COXA[0], LIM_COXA[1]);
      a.hip = clampf(a.hip, LIM_HIP[0], LIM_HIP[1]);
      a.knee = clampf(a.knee, LIM_KNEE[0], LIM_KNEE[1]);
      if (fabsf(a.coxa - c0) + fabsf(a.hip - h0) + fabsf(a.knee - k0) > 0.5f) a.ok = false;
      out[i] = a; onGround[i] = og;
    }
  }

private:
  // jeden krok nohy: v čase t0..t0+dur přesune posun z "from" na "to" a zvedne chodidlo
  static float stepU(float t, float t0, float dur) { return clampf((t - t0) / dur, 0, 1); }
  static float stepLift(float u, float h) { return (u > 0 && u < 1) ? sinf(PI_F * u) * h : 0; }

  void runSeq(float t, float legOff[6][2], float legLift[6], Angles ovr[6], float blend[6], float &march) {
    const float W = 2 * PI_F;
    switch (seq) {
      case S_STANDUP: {
        // 1) zvednout tělo na všech šesti (nohy jsou roztažené, páka je rozložená na 6 noh)
        // 2) nohy po jedné přitáhnout do chodicí polohy
        hCur = H_REST + (P.height - H_REST) * smooth(t / 1.8f);
        for (int i = 0; i < 6; i++) {
          const float u = stepU(t, 1.8f + ORDER[i] * 0.4f, 0.4f);
          ext[i] = EXT_REST * (1 - smooth(u));
          legLift[i] = stepLift(u, 14);
        }
      } break;
      case S_SITDOWN: {
        for (int i = 0; i < 6; i++) {
          const float u = stepU(t, ORDER[i] * 0.4f, 0.4f);
          ext[i] = EXT_REST * smooth(u);
          legLift[i] = stepLift(u, 14);
        }
        hCur = P.height + (H_REST - P.height) * smooth((t - 2.4f) / 1.8f);
      } break;
      case S_WAVE: {        // zamávání levou přední nohou
        const float s1 = env(t, 0.9f, 6.6f, 0.9f);
        shift[0] -= 14 * s1; shift[1] -= 16 * s1; shift[2] += 4 * s1;
        rot[0] += 3 * s1; rot[1] -= 4 * s1;
        blend[0] = smooth((t - 0.9f) / 0.8f) * (1 - smooth((t - 5.8f) / 0.8f));
        const float wt = fmaxf(0, t - 1.7f);
        const float amp = smooth(wt / 0.6f) * (1 - smooth((t - 5.2f) / 0.6f));
        ovr[0].coxa = 12 + 26 * amp * sinf(wt * W * 1.1f); ovr[0].hip = -72;
        ovr[0].knee = 28 + 16 * amp * sinf(wt * W * 1.1f + 1.6f);
      } break;
      case S_SHAKE: {       // podání nohy
        const float s1 = env(t, 0.9f, 6.0f, 0.9f);
        shift[0] -= 14 * s1; shift[1] -= 16 * s1; shift[2] += 4 * s1;
        rot[0] += 3 * s1; rot[1] -= 4 * s1;
        blend[0] = smooth((t - 0.9f) / 0.8f) * (1 - smooth((t - 5.2f) / 0.8f));
        const float wt = fmaxf(0, t - 2.4f);
        const float amp = smooth(wt / 0.4f) * (1 - smooth((t - 4.6f) / 0.4f));
        ovr[0].coxa = -32; ovr[0].hip = -38 + 7 * amp * sinf(wt * W * 1.6f); ovr[0].knee = 34;
      } break;
      case S_DANCE: {
        const float a = env(t, 1.0f, 9.0f, 1.0f), w = t * W * 0.45f;
        rot[0] = 4 * a * sinf(w); rot[1] = 4 * a * sinf(w * 2 + 1); rot[2] = 6 * a * sinf(w * 0.5f);
        shift[0] = 7 * a * sinf(w * 0.5f + 0.7f); shift[1] = 7 * a * cosf(w); shift[2] = 3 * a * sinf(w * 1.5f);
      } break;
      case S_BOW: {         // úklona
        const float e = env(t, 0.9f, 2.5f, 0.9f);
        rot[1] = 8 * e; shift[0] = 6 * e; shift[2] = -1 * e;
      } break;
      case S_SWAY: {        // kroužení tělem
        const float a = env(t, 1.0f, 5.0f, 1.0f), w = t * W * 0.5f;
        shift[0] = 11 * a * cosf(w); shift[1] = 11 * a * sinf(w);
        rot[1] = 4 * a * cosf(w); rot[0] = -4 * a * sinf(w);
      } break;
      case S_WIGGLE: {      // zavrtění
        const float a = env(t, 0.3f, 1.8f, 0.4f), w = t * W * 3;
        rot[2] = 8 * a * sinf(w); shift[1] = 4 * a * sinf(w + 1.2f);
      } break;
      case S_PUSHUPS: {     // 3 kliky
        const float a = (t < 4.8f) ? 0.5f - 0.5f * cosf(t * W / 1.6f) : 0;
        shift[2] = -14 * a;
      } break;
      case S_MARCH: {       // pochod na místě
        march = env(t, 0.5f, 3.8f, 0.6f);
      } break;
      case S_STARTLE: {     // leknutí: rychle se odtáhne dozadu
        const float e = smooth(t / 0.18f) * (1 - smooth((t - 0.9f) / 0.5f));
        shift[0] = -16 * e; shift[2] = 5 * e; rot[1] = -7 * e;
      } break;
      case S_ATTACK: {      // útočný postoj: stojí na čtyřech, přední nohy nahoře
        // prostřední nohy nejdřív ukročí dopředu, aby těžiště zůstalo nad oporou
        const float uL = stepU(t, 0.0f, 0.5f) - stepU(t, 7.4f, 0.5f);
        const float uR = stepU(t, 0.5f, 0.5f) - stepU(t, 7.9f, 0.5f);
        legOff[1][0] = 30 * smooth(uL); legOff[4][0] = 30 * smooth(uR);
        legLift[1] = stepLift(stepU(t, 0.0f, 0.5f), 14) + stepLift(stepU(t, 7.4f, 0.5f), 14);
        legLift[4] = stepLift(stepU(t, 0.5f, 0.5f), 14) + stepLift(stepU(t, 7.9f, 0.5f), 14);
        const float p = smooth((t - 1.0f) / 0.7f) * (1 - smooth((t - 6.7f) / 0.7f));
        shift[0] = -16 * p; shift[2] = 3 * p; rot[1] = -6 * p;
        const float b = smooth((t - 1.7f) / 0.7f) * (1 - smooth((t - 6.0f) / 0.7f));
        const float wt = fmaxf(0, t - 2.4f), amp = smooth(wt / 0.5f) * (1 - smooth((t - 5.4f) / 0.5f));
        blend[0] = blend[3] = b;
        ovr[0].coxa = -22; ovr[0].hip = -66 + 8 * amp * sinf(wt * W * 1.2f); ovr[0].knee = 22 + 14 * amp * sinf(wt * W * 1.2f + 1.2f);
        ovr[3].coxa = 22;  ovr[3].hip = -66 - 8 * amp * sinf(wt * W * 1.2f); ovr[3].knee = 22 - 14 * amp * sinf(wt * W * 1.2f + 1.2f);
      } break;
      default: break;
    }
  }
};

}  // namespace rs
