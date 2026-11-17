// Test pohybového jádra na počítači:  g++ -std=c++11 -O2 -o test_motion test_motion.cpp && ./test_motion
// Kontroluje: IK == FK, meze kloubů, dosah, rychlost serv a stabilitu ve všech chůzích a sekvencích.
#include <cstdio>
#include <vector>
#include <algorithm>
#include "../robospider/motion.h"
using namespace rs;

struct P2 { float x, y; };
static float cross(P2 o, P2 a, P2 b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); }
static float margin(std::vector<P2> p, P2 c) {
  if (p.size() < 3) return -1;
  std::sort(p.begin(), p.end(), [](P2 a, P2 b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
  std::vector<P2> lo, hi;
  for (auto q : p) { while (lo.size() >= 2 && cross(lo[lo.size() - 2], lo.back(), q) <= 0) lo.pop_back(); lo.push_back(q); }
  for (int i = (int)p.size() - 1; i >= 0; i--) { auto q = p[i]; while (hi.size() >= 2 && cross(hi[hi.size() - 2], hi.back(), q) <= 0) hi.pop_back(); hi.push_back(q); }
  lo.pop_back(); hi.pop_back(); lo.insert(lo.end(), hi.begin(), hi.end());
  float m = 1e9;
  for (size_t i = 0; i < lo.size(); i++) {
    P2 a = lo[i], b = lo[(i + 1) % lo.size()];
    float ex = b.x - a.x, ey = b.y - a.y;
    m = std::min(m, -((c.x - a.x) * ey - (c.y - a.y) * ex) / hypotf(ex, ey));
  }
  return m;
}

struct Stat { float spd = 0, marg = 1e9; int bad = 0; float mn[3] = { 1e9, 1e9, 1e9 }, mx[3] = { -1e9, -1e9, -1e9 }; };
static int fails = 0;

static void run(Motion &m, float secs, Stat &s) {
  const float dt = 0.02f;
  Angles prev[6]; bool have = false;
  for (float t = 0; t < secs; t += dt) {
    m.update(dt);
    std::vector<P2> sup;
    for (int i = 0; i < 6; i++) {
      const Angles &a = m.out[i];
      if (!a.ok) s.bad++;
      const float v[3] = { a.coxa, a.hip, a.knee };
      for (int j = 0; j < 3; j++) { s.mn[j] = std::min(s.mn[j], v[j]); s.mx[j] = std::max(s.mx[j], v[j]); }
      if (have) s.spd = std::max(s.spd, std::max(fabsf(a.coxa - prev[i].coxa), std::max(fabsf(a.hip - prev[i].hip), fabsf(a.knee - prev[i].knee))) / dt);
      prev[i] = a;
      if (m.onGround[i]) sup.push_back({ m.foot[i][0], m.foot[i][1] });
    }
    have = true;
    s.marg = std::min(s.marg, margin(sup, { m.shift[0], m.shift[1] }));
  }
}
static void report(const char *name, const Stat &s, float minMargin) {
  const bool ok = s.bad == 0 && s.spd <= 600 && s.marg >= minMargin;
  if (!ok) fails++;
  printf("%-22s %s  mimo dosah/meze: %4d  max rychlost %4.0f st/s  stabilita %5.1f mm  coxa %4.0f..%3.0f  stehno %4.0f..%3.0f  koleno %4.0f..%3.0f\n",
         name, ok ? "OK  " : "CHYBA", s.bad, s.spd, s.marg, s.mn[0], s.mx[0], s.mn[1], s.mx[1], s.mn[2], s.mx[2]);
}
static void standUp(Motion &m) { Stat s; m.start(Motion::S_STANDUP); run(m, Motion::seqDuration(Motion::S_STANDUP) + 0.5f, s); }

int main() {
  // 1) IK a FK se shodují
  float worst = 0;
  for (int i = 0; i < 6; i++) for (float x = 50; x <= 100; x += 10) for (float y = -30; y <= 30; y += 15) for (float z = -70; z <= -20; z += 10) {
    const float p[3] = { x, y, z }; Angles a = ik(LEGS[i], p); if (!a.ok) continue;
    float q[3]; fk(LEGS[i], a, q);
    worst = std::max(worst, hypotf(hypotf(q[0] - p[0], q[1] - p[1]), q[2] - p[2]));
  }
  printf("IK/FK nejhorsi odchylka: %.4f mm %s\n", worst, worst < 0.01f ? "OK" : "CHYBA"); if (worst >= 0.01f) fails++;
  { const float p[3] = { 69, 22, -53.5f }; Angles a = ik(LEGS[0], p); printf("neutral: coxa %.2f stehno %.2f koleno %.2f (ma byt 0, -20, 90)\n", a.coxa, a.hip, a.knee); }

  // 2) vstávání a lehání
  { Motion m; Stat s; m.update(0.02f); m.start(Motion::S_STANDUP); run(m, Motion::seqDuration(Motion::S_STANDUP) + 0.5f, s); report("vstan", s, 15);
    Stat s2; m.start(Motion::S_SITDOWN); run(m, Motion::seqDuration(Motion::S_SITDOWN) + 0.5f, s2); report("lehni", s2, 15); }

  // 3) chůze všemi směry ve všech chůzích
  const char *dn[] = { "vpred", "vzad", "vlevo", "vpravo", "otoc_l", "otoc_p", "oblouk" };
  const float dirs[][3] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }, { 1, 0, 0.6f } };
  for (int g = 0; g < 3; g++) for (int d = 0; d < 7; d++) {
    Motion m; standUp(m); m.gait = g; Stat s;
    m.drive(dirs[d][0], dirs[d][1], dirs[d][2]); run(m, 6, s); m.stop(); run(m, 2, s);
    char nm[40]; snprintf(nm, sizeof nm, "%s %s", GAITS[g].name, dn[d]); report(nm, s, 5);
  }
  // 4) sekvence
  const char *sn[] = { "", "", "", "mavej", "tanec", "uklona", "utok", "podej", "zavrt", "kliky", "pochod", "krouzeni", "leknuti" };
  for (int q = Motion::S_WAVE; q < Motion::S_COUNT; q++) {
    Motion m; standUp(m); Stat s;
    m.start((Motion::Seq)q); run(m, Motion::seqDuration((Motion::Seq)q) + 1, s);
    report(sn[q], s, q == Motion::S_MARCH ? 5 : 12);
  }
  printf(fails ? "\nNEPROSLO: %d\n" : "\nVse proslo.\n", fails);
  return fails ? 1 : 0;
}
