// =====================================================================
//  ROBOSPIDER  –  3D tisknutelný šestinohý robot (hexapod)
//  18x MG90S (3 na nohu), 2x PCA9685, ESP32-C3 nebo Raspberry Pi 4B
//
//  openscad -D 'part="assembly"' robospider.scad
//  openscad -D 'part="femur"' -o femur.stl robospider.scad
//
//  part = assembly | body | lid | coxa_L | coxa_R | femur |
//         tibia_L | tibia_R | tibia_cap | bushing | tip_cap | servo_test | leg |
//         battery_cradle
//
//  Souřadnice: X dopředu, Y doleva, Z nahoru. Rozměry v mm.
//  Kloubové moduly (kapsa serva, vidlice, čep M3) jsou stejné jako u robopsa.
// =====================================================================

part = "assembly";
$fn  = 48;

// ---------------- tolerance ----------------
clr = 0.3;

// ---------------- MG90S (počátek = osa hřídele na horní ploše pouzdra) ----
sv_L = 22.8; sv_W = 12.2; sv_H = 22.5;
sv_sx = 5.9;  sv_cx = sv_L/2 - sv_sx;
sv_tab_bot = -6.6; sv_tab_t = 2.5; sv_tab_span = 32.2; sv_hole_sp = 27.8;
sv_boss_d = 11.8; sv_boss_h = 4.0; sv_spline_d = 4.8; sv_spline_h = 2.5;

// ---------------- jednoramenná páka ----------------
horn_top = 8.0; horn_arm_t = 1.8; horn_hub_d = 7.6;
horn_arm_len = 17.5; horn_tip_d = 4.8; horn_screw_access = 5.5;

// ---------------- držák serva ----------------
hold_y = 10; floor_t = 4;
hold_x0 = sv_cx - sv_tab_span/2 - 2.3;     // -12.9
hold_x1 = sv_cx + sv_tab_span/2 + 2.3;     //  23.9
hold_ztop = sv_tab_bot + sv_tab_t;         //  -4.1
hold_zbot = -sv_H - floor_t;               // -26.5
cab_w = 5.2;     // šířka drážky pro kabel serva (plochý kabel MG90S ~3.5 mm)
brg_t = 5;       // tloušťka můstku nad oknem pro kabel (nese šroub oušku)
cab_ch = 4;      // zkosení rohů čela držáku pro vedení kabelu

// ---------------- vidlice ----------------
gap = 4;
hp_z0 = horn_top - horn_arm_t - 0.2;  hp_t = 4.5;
pp_t = 4;  pp_z1 = hold_zbot - gap;
fork_zmin = pp_z1 - pp_t;  fork_zmax = hp_z0 + hp_t;
fork_r0 = 17; fork_r1 = 20; fork_w = hold_y;
bush_od = 6;  bush_len = pp_t + gap + 0.3;

// ---------------- noha ----------------
R_F  = 43.5;     // kyčel (coxa): svislá osa -> osa stehenního serva
y_F  = 16.5;     // boční posun stehenního serva (aby kapsa lícovala s vidlicí)
z_F  = 12;       // výška osy stehna nad osou coxa serva
L1   = 46;       // stehno: osa -> osa kolene
tib_r1 = 22;     // tlustší stěna bérce (šrouby víčka)
tib_R  = 40;     // poloměr oblouku bérce
tib_sweep = 75;  // úhel oblouku
tib_tip = 15;    // rovná špička

// ---------------- trup ----------------
// polohy svislých os coxa serv (levá strana; pravá je zrcadlo) a směr nohy
legs = [ [ 65, 45,  45],     // přední
         [  0, 60,  90],     // prostřední
         [-65, 45, 135] ];   // zadní
body_z0 = -hold_ztop;        // spodek trupu (4.1) = horní hrana oušek
body_h  = 36;                // výška vany
wall = 2.5;
lid_t = 3;

// barvy náhledu: trup a stehna černé, coxa a bérce červené
c_black = [0.13,0.13,0.14];
c_red   = [0.82,0.12,0.09];

// póza pro náhled: coxa, stehno, koleno (stupně)
pose = [0, -20, 90];

// =====================================================================
//  MG90S + páka (náhled)
// =====================================================================
module mg90s() {
    color([0.15,0.35,0.85]) {
        translate([-sv_sx, -sv_W/2, -sv_H]) cube([sv_L, sv_W, sv_H]);
        difference() {
            translate([sv_cx - sv_tab_span/2, -sv_W/2, sv_tab_bot]) cube([sv_tab_span, sv_W, sv_tab_t]);
            for (s=[-1,1]) translate([sv_cx + s*sv_hole_sp/2, 0, -10]) cylinder(d=2, h=20);
        }
        cylinder(d=sv_boss_d, h=sv_boss_h);
        translate([6,0,0]) cylinder(d=6, h=2);
    }
    color("gold") translate([0,0,sv_boss_h]) cylinder(d=sv_spline_d, h=sv_spline_h);
}
module horn(ang=180) {
    color("white") {
        translate([0,0,sv_boss_h+0.4]) cylinder(d=7, h=horn_top-sv_boss_h-0.4);
        rotate([0,0,ang]) hull() {
            translate([0,0,horn_top-horn_arm_t+0.2]) cylinder(d=7, h=horn_arm_t-0.2);
            translate([horn_arm_len-2.3,0,horn_top-horn_arm_t+0.2]) cylinder(d=4.2, h=horn_arm_t-0.2);
        }
    }
}
horn_clr = 0.35;  // vůle navíc kolem páky (na každou stranu)
module horn2d(c, ang) {
    rotate([0,0,ang]) hull() {
        circle(d=horn_hub_d+2*c);
        translate([horn_arm_len+horn_clr-(horn_tip_d+2*horn_clr)/2,0]) circle(d=horn_tip_d+2*c);
    }
    circle(d=horn_hub_d+2*c);
}
module horn_cut(ang=180) {
    translate([0,0,hp_z0-1]) linear_extrude(horn_top-hp_z0+1.4) horn2d(horn_clr, ang);
    // náběh na ústí vybrání, páka se snáz zasune
    translate([0,0,hp_z0-0.01]) hull() {
        linear_extrude(0.01) horn2d(horn_clr+0.6, ang);
        translate([0,0,0.6]) linear_extrude(0.01) horn2d(horn_clr, ang);
    }
    translate([0,0,-1]) cylinder(d=horn_screw_access, h=40);
}

// =====================================================================
//  Držák serva a vidlice (stejné jako robopes)
// =====================================================================
// ---------------- zaoblení ----------------
r_edge  = 2;     // zaoblení hran bloků
r_plate = 1.2;   // zaoblení hran desek vidlic
r_body  = 3;     // zaoblení trupu
// kvádr se zaoblenými hranami, a = [x0,y0,z0], b = [x1,y1,z1]
module rbox(a, b, r=r_edge) {
    rr = max(0.2, min(r, (b[0]-a[0])/2-0.01, (b[1]-a[1])/2-0.01, (b[2]-a[2])/2-0.01));
    hull() for (x=[a[0]+rr, b[0]-rr], y=[a[1]+rr, b[1]-rr], z=[a[2]+rr, b[2]-rr])
        translate([x,y,z]) sphere(r=rr, $fn=16);
}
// deska se zaoblenými hranami z 2D tvaru (konvexního)
module rslab(z0, t, rp=r_plate) {
    translate([0,0,z0+rp]) minkowski() {
        linear_extrude(t-2*rp) offset(delta=-rp) children();
        sphere(r=rp, $fn=12);
    }
}
module holder_solid(ext=0) {
    rbox([hold_x0, -hold_y, hold_zbot], [hold_x1+ext, hold_y, hold_ztop]);
}
// ---------------- uložení serva (v2) ----------------
sclr   = 0.4;    // vůle kapsy kolem pouzdra serva
rib_h  = 0.35;   // přesah lisovacích žeber (servo drží bez vůle, žebra se přizpůsobí)
rib_r  = 0.7;
lead   = 0.5;    // náběh (zkosení) na ústí kapsy
srv_hole_d = 1.8;// díra pro samořezný šroub M2 v oušku
module pocket2d(c) {      // obdélník pouzdra s odlehčením rohů (tisk nezaoblí rohy)
    x0 = -sv_sx-c; x1 = sv_L-sv_sx+c; y1 = sv_W/2+c;
    translate([x0, -y1]) square([x1-x0, 2*y1]);
    for (x=[x0,x1], y=[-y1,y1]) translate([x,y]) circle(d=1.4, $fn=12);
}
module tab2d(c) {
    x0 = sv_cx-sv_tab_span/2-c; x1 = sv_cx+sv_tab_span/2+c; y1 = sv_W/2+c;
    translate([x0, -y1]) square([x1-x0, 2*y1]);
    for (x=[x0,x1], y=[-y1,y1]) translate([x,y]) circle(d=1.4, $fn=12);
}
module holder_cut() {
    difference() {
        union() {
            // kapsa pouzdra + kapsa oušek
            translate([0,0,-sv_H-sclr]) linear_extrude(60) pocket2d(sclr);
            translate([0,0,sv_tab_bot-sclr]) linear_extrude(60) tab2d(sclr);
            // náběh na ústí
            translate([0,0,hold_ztop-lead]) hull() {
                linear_extrude(0.01) tab2d(sclr);
                translate([0,0,lead+0.01]) linear_extrude(1) tab2d(sclr+lead);
            }
        }
        // lisovací žebra na dlouhých stěnách (2 na každé straně), nahoře se zužují
        for (x=[sv_cx-6, sv_cx+6], s=[-1,1])
            translate([x, s*(sv_W/2+sclr+rib_r-rib_h), -sv_H-sclr-1]) {
                cylinder(r=rib_r, h=(sv_tab_bot-sclr-2)-(-sv_H-sclr-1), $fn=16);
                translate([0,0,(sv_tab_bot-sclr-2)-(-sv_H-sclr-1)]) cylinder(r1=rib_r, r2=0.01, h=1.5, $fn=16);
            }
    }
    // šrouby do obou oušek
    for (s=[-1,1]) translate([sv_cx + s*sv_hole_sp/2, 0, sv_tab_bot-9]) cylinder(d=srv_hole_d, h=10, $fn=16);
    // kabel: okno v čelní stěně u hřídele pod můstkem; na můstek se přišroubuje ouško serva
    translate([hold_x0-1, -cab_w/2, -sv_H-sclr]) cube([(-sv_sx-sclr) - hold_x0 + 1.01, cab_w, (sv_tab_bot-sclr-brg_t) - (-sv_H-sclr)]);
    for (s=[-1,1]) translate([0,0,hold_zbot-1]) linear_extrude(hold_ztop-hold_zbot+2)
        polygon([[hold_x0-1, s*(hold_y+1)], [hold_x0-1, s*(hold_y-cab_ch-1)], [hold_x0+cab_ch+1, s*(hold_y+1)]]);
    // čep M3 + kapsa na matici
    translate([0,0,hold_zbot-1]) cylinder(d=3.3, h=floor_t+2, $fn=20);
    translate([0,0,hold_zbot+0.5]) hull() {
        cylinder(d=6.5, h=2.6, $fn=6);
        translate([0,-hold_y-1,0]) cylinder(d=6.5, h=2.6, $fn=6);
    }
}
module plate2d(r1=fork_r1) {
    hull() { circle(r=hold_y); translate([-r1, -fork_w]) square([r1-fork_r0, 2*fork_w]); }
}
module fork_solid(r1=fork_r1, pivot=true) {
    rslab(hp_z0, hp_t) plate2d(r1);
    if (pivot) rslab(fork_zmin, pp_t) plate2d(r1);
    rbox([-r1, -fork_w, pivot ? fork_zmin : pp_z1], [-fork_r0, fork_w, fork_zmax], r_plate);
}
module fork_cut() {
    horn_cut(180);
    translate([0,0,fork_zmin-1]) cylinder(d=bush_od+0.3, h=pp_t+2);
}

// =====================================================================
//  Rámce kloubů (kanonická noha: osa coxa v počátku, noha míří k +X)
// =====================================================================
module C_frame() { multmatrix([[-1,0,0,0],[0,1,0,0],[0,0,-1,0],[0,0,0,1]]) children(); }
module F_frame() { multmatrix([[-1,0,0,R_F],[0,0,1,y_F],[0,1,0,z_F],[0,0,0,1]]) children(); }

// =====================================================================
//  DÍLY
// =====================================================================
// --- vedení kabelů ---
// zacvakávací úchyt: dva sloupky se šikmou stříškou a štěrbinou nahoře.
// Plochý kabel se štěrbinou protlačí na hranu a uvnitř si lehne naplocho.
clip_cw = 2.3;    // polovina šířky dutiny (kabel ~3,6 mm)
clip_h  = 1.4;    // výška bočních stěn dutiny
clip_slit = 0.8;  // polovina šířky štěrbiny
clip_t  = 1.4;    // tloušťka stříšky
clip_len = 5;
module clip2d() {
    for (m=[0,1]) mirror([m,0]) polygon([
        [-clip_cw-1.5, -0.6], [-clip_cw-1.5, clip_h+clip_t],
        [-clip_slit, clip_h+(clip_cw-clip_slit)+clip_t], [-clip_slit, clip_h+(clip_cw-clip_slit)],
        [-clip_cw, clip_h], [-clip_cw, -0.6]]);
}
// úchyt na ploše y = y0 (kabel vede podél X), střed v [xc, zc]
module cable_clip(xc, y0, zc) {
    translate([xc-clip_len/2, y0, zc]) multmatrix([[0,0,1,0],[0,1,0,0],[1,0,0,0],[0,0,0,1]])
        linear_extrude(clip_len) clip2d();
}
// očko na horní ploše držáku coxy (tiskne se nastojato bez podpěr): dva kabely + konektor projdou
loop_x0 = 22; loop_len = 5; loop_gap = 4.2; loop_t = 2;
module coxa_loop() {
    zt = z_F + hold_y;                      // horní plocha držáku (22)
    y0 = y_F + hold_zbot; y1 = y_F + hold_ztop;
    translate([loop_x0, y0, zt+loop_gap]) cube([loop_len, y1-y0, loop_t]);          // vnější příčka
    for (y=[y0, y1-loop_t]) translate([loop_x0, y, zt-3]) cube([loop_len, loop_t, loop_gap+loop_t+3]);
}

// --- coxa: vidlice kolem svislého serva v trupu + držák stehenního serva ---
module coxa() {
    difference() {
        union() {
            C_frame() fork_solid();
            F_frame() holder_solid(2);
            coxa_loop();
        }
        C_frame() fork_cut();
        F_frame() holder_cut();
    }
}

// --- stehno ---
module femur() {
    difference() {
        union() {
            fork_solid();
            rbox([-(L1-hold_x1)-3, -fork_w, fork_zmin], [-fork_r0-1, fork_w, fork_zmax]);
            translate([-L1,0,0]) holder_solid(3);
            // úchyty kabelu kolenního serva na horní ploše stehna
            for (x=[-22, -36]) cable_clip(x, hold_y, (hold_zbot+hold_ztop)/2);
        }
        fork_cut();
        translate([-L1,0,0]) holder_cut();
    }
}

// --- bérec: oblouk dolů (v lokálních souř. -Y), čepová deska je zvlášť ---
tib_z1 = fork_zmax;                     // vnější (tisková) plocha
function tib_pt(t) = let(a = t*tib_sweep)
    [-tib_r1 - 6 - tib_R*sin(a), -tib_R*(1-cos(a))];
tib_end = tib_pt(1);
tib_dir = [-cos(tib_sweep), -sin(tib_sweep)];   // tečna na konci oblouku
tib_tip_pt = tib_end + tib_tip*tib_dir;
module tib_disc(p, r, th) { translate([p[0], p[1], tib_z1-th]) cylinder(r=r, h=th); }
module tibia() {
    n = 10;
    difference() {
        union() {
            fork_solid(tib_r1, pivot=false);
            for (i=[0:n-1]) hull() {
                tib_disc(tib_pt(i/n),     7 - 3*i/n,     18 - 8*i/n);
                tib_disc(tib_pt((i+1)/n), 7 - 3*(i+1)/n, 18 - 8*(i+1)/n);
            }
            hull() {
                tib_disc(tib_end, 4, 10);
                translate([tib_tip_pt[0], tib_tip_pt[1], tib_z1-5]) sphere(r=3.5);
            }
            // náběh mezi stěnou a obloukem
            hull() {
                rbox([-tib_r1, -fork_w, pp_z1], [-fork_r0, fork_w, fork_zmax], r_plate);
                tib_disc(tib_pt(0.15), 6.8, 17);
            }
        }
        fork_cut();
        // díry pro šrouby víčka (M2.5 samořezné)
        for (s=[-1,1]) translate([-(fork_r0+tib_r1)/2, s*6, pp_z1-1]) cylinder(d=2.2, h=12, $fn=16);
    }
}
module tibia_cap() {        // čepová deska bérce, přišroubuje se zespodu
    difference() {
        rslab(fork_zmin, pp_t) plate2d(tib_r1);
        translate([0,0,fork_zmin-1]) cylinder(d=bush_od+0.3, h=pp_t+2);
        for (s=[-1,1]) translate([-(fork_r0+tib_r1)/2, s*6, fork_zmin-1]) cylinder(d=2.9, h=pp_t+2, $fn=16);
        for (s=[-1,1]) translate([-(fork_r0+tib_r1)/2, s*6, fork_zmin-1]) cylinder(d=5, h=1.5, $fn=20);
    }
}

// --- trup: vana s šesti držáky coxa serv ---
// půdorys vany = průnik polorovin 23.9 mm za každou osou coxa
function hub_r(L) = (L[0]*cos(L[2]) + L[1]*sin(L[2])) - hold_x1 + 0.5;
hub_cap = 68;      // tupý předek a záď
module hub2d(inset=0) {
    intersection_for (i=[0:7]) {
        L = legs[i%3];
        a = i<3 ? L[2] : (i<6 ? -L[2] : (i==6 ? 0 : 180));
        r = i<6 ? hub_r(L) : hub_cap;
        rotate([0,0,a]) translate([r-inset-600, -300]) square(600);
    }
}
module leg_place(i, right=false) {
    L = legs[i];
    if (!right) translate([L[0], L[1], 0]) rotate([0,0,L[2]]) children();
    else mirror([0,1,0]) translate([L[0], L[1], 0]) rotate([0,0,L[2]]) children();
}
pca_pos = [[0, 15], [0, -15]];
ent_w = 6; ent_h = 10;          // okno pro kabel coxa serva
ent_side = [-1, 1, 1];          // na které straně držáku je okno (přední, prostřední, zadní noha)

// --- kolébka baterie: rámeček pro standardní držák 2x 18650 (77 x 41 x 21 mm) ---
bat_L = 77; bat_W = 41; bat_H = 21;
bat_clr = 0.6; bat_wall = 2; bat_lip = 3; bat_lip_t = 2;
bat_ear_x = 48; bat_ear_y = 8;  // šrouby M3 do mřížky děr v dně trupu
module battery_cradle() {
    xi = bat_L/2 + bat_clr; yi = bat_W/2 + bat_clr;
    zt = body_z0; zb = zt - bat_H - 0.3;         // dutina
    difference() {
        union() {
            rbox([-xi-bat_wall, -yi-bat_wall, zb-bat_lip_t], [xi+bat_wall, yi+bat_wall, zt], 1.2);
            // ouška se šikmou podpěrou (tisk bez podpěr, lemem dolů)
            for (sx=[-1,1], sy=[-1,1]) hull() {
                translate([sx*(xi+bat_wall-1), sy*bat_ear_y-4.5, zt-3]) cube([0.01, 9, 3]);
                translate([sx*bat_ear_x, sy*bat_ear_y, zt-3]) cylinder(d=9, h=3);
                translate([sx*(xi+bat_wall-1), sy*bat_ear_y-4.5, zt-3-(bat_ear_x-xi-bat_wall+4.5)]) cube([0.01, 9, 0.01]);
            }
        }
        translate([-xi, -yi, zb]) cube([2*xi, 2*yi, bat_H+10]);                       // dutina pro držák
        translate([-xi+bat_lip, -yi+bat_lip, zb-bat_lip_t-1]) cube([2*(xi-bat_lip), 2*(yi-bat_lip), bat_lip_t+2]);  // otevřené dno
        for (sx=[-1,1], sy=[-1,1]) translate([sx*bat_ear_x, sy*bat_ear_y, zt-30]) cylinder(d=3.4, h=40, $fn=20);
        translate([-xi-bat_wall-1, -4, zt-8]) cube([bat_wall+2, 8, 9]);              // výřez pro vodiče vzadu
        // odlehčení bočních stěn
        for (sy=[-1,1], x=[-24, 0, 24]) translate([x-9, sy*(yi+bat_wall/2)-2, zb+4]) cube([18, 4, bat_H-9]);
    }
}
module body() {
    difference() {
        union() {
            rslab(body_z0, body_h, r_body) hub2d();
            for (i=[0:2], r=[false,true]) leg_place(i, r) C_frame() holder_solid(3);
            // PCA9685 podstavce (2 desky vedle sebe, delší strana podél X)
            for (p=pca_pos, x=[-27.95,27.95], y=[-9.525,9.525])
                translate([p[0]+x, p[1]+y, body_z0]) cylinder(d=5.5, h=wall+5);
        }
        translate([0,0,body_z0+wall]) linear_extrude(body_h) offset(r=r_body) offset(delta=-r_body) hub2d(wall);
        for (i=[0:2], r=[false,true]) leg_place(i, r) C_frame() holder_cut();
        for (p=pca_pos, x=[-27.95,27.95], y=[-9.525,9.525])
            translate([p[0]+x, p[1]+y, body_z0-1]) cylinder(d=2.2, h=20, $fn=16);
        // mřížka M3 v přední a zadní části (step-down měniče)
        for (sx=[-1,1], x=[40:8:56], y=[-16:8:16])
            if (abs(x)+abs(y) < 62) translate([sx*x, y, body_z0-1]) cylinder(d=3.2, h=wall+2, $fn=16);
        // průchody pro pásek baterie (baterie je pod trupem)
        for (x=[-25,25], s=[-1,1]) translate([x-10, s*26-1.5, body_z0-1]) cube([20,3,wall+2]);
        // průchody kabelů serv: zářez v horní hraně u každé nohy
        for (i=[0:2], r=[false,true]) leg_place(i, r)
            translate([-hold_x1-4, -7, body_z0+body_h-13]) cube([10, 14, 14]);
        // vstup kabelu coxa serva: okno ve stěně trupu hned vedle držáku (projde i konektor)
        for (i=[0:2], r=[false,true]) leg_place(i, r)
            translate([-hold_x1-4, ent_side[i] > 0 ? hold_y+0.5 : -hold_y-0.5-ent_w, body_z0+wall+0.5])
                cube([10, ent_w, ent_h]);
        // průchod vodičů baterie (za kolébkou vzadu)
        translate([-bat_L/2-bat_clr-bat_wall-5, 0, body_z0-1]) cylinder(d=6, h=wall+2, $fn=24);
        // vypínač vzadu
        translate([-hub_cap, 0, body_z0+body_h/2]) rotate([0,90,0]) cylinder(d=12.2, h=10, center=true);
        // sloupky víka (díry)
        for (p=lid_posts) translate([p[0],p[1],body_z0+body_h-10]) cylinder(d=2.6, h=11, $fn=16);
    }
    for (p=lid_posts) difference() {
        translate([p[0],p[1],body_z0]) cylinder(d=7, h=body_h);
        translate([p[0],p[1],body_z0+body_h-10]) cylinder(d=2.6, h=11, $fn=16);
    }
}
lid_posts = [[60, 0], [-60, 0], [20, 31], [-20, 31], [20, -31], [-20, -31]];

// --- víko: Raspberry Pi 4B nebo ESP32-C3 SuperMini ---
module lid() {
    difference() {
        union() {
            rslab(0, lid_t) hub2d();
            for (x=[-29,29], y=[-24.5,24.5]) translate([x, y, 0]) cylinder(d=6, h=lid_t+5);
            // kolébka ESP32-C3 SuperMini (22.5 x 18) vpředu
            rbox([52-13, -11, 0], [52+13, 11, lid_t+4], 1.5);
        }
        for (x=[-29,29], y=[-24.5,24.5]) translate([x, y, -1]) cylinder(d=2.2, h=20, $fn=16);
        translate([52-11.5, -9.25, lid_t]) cube([23, 18.5, 10]);
        translate([52+8, -6, lid_t-1]) cube([10, 12, 10]);        // USB-C
        for (p=lid_posts) translate([p[0],p[1],-1]) cylinder(d=3.4, h=20, $fn=16);
        // kabelové průchody + chlazení
        for (x=[-20:10:20]) translate([x-3, -18, -1]) cube([6, 36, 10]);
        translate([-52, -10, -1]) cube([8, 20, 10]);
    }
}

// --- drobnosti ---
module bushing() {     // konce zkosené proti "sloní noze" z první vrstvy
    difference() {
        hull() {
            cylinder(d=bush_od-0.8, h=bush_len);
            translate([0,0,0.4]) cylinder(d=bush_od, h=bush_len-0.8);
        }
        translate([0,0,-1]) cylinder(d=3.4, h=bush_len+2, $fn=20);
    }
}
module tip_cap() {          // TPU návlek na špičku bérce
    difference() {
        hull() { sphere(r=5.5); translate([0,0,10]) cylinder(r=5.5, h=1); }
        hull() { sphere(r=3.6); translate([0,0,12]) cylinder(r=3.9, h=1); }
    }
}
module servo_test() {
    difference() { holder_solid(); holder_cut(); }
    translate([0, 30, hold_zbot + hp_z0 + hp_t]) mirror([0,0,1]) difference() {
        translate([0,0,hp_z0]) linear_extrude(hp_t) hull() { circle(r=8); translate([-20,0]) circle(r=5); }
        horn_cut(180);
    }
}

// =====================================================================
//  SESTAVA
// =====================================================================
module leg(p=pose, servos=true) {
    if (servos) C_frame() { mg90s(); }
    rotate([0,0,p[0]]) {
        if (servos) C_frame() horn(180);
        color(c_red) coxa();
        F_frame() {
            if (servos) { mg90s(); rotate([0,0,p[1]]) horn(180); }
            rotate([0,0,p[1]]) {
                color(c_black) femur();
                translate([-L1,0,0]) {
                    if (servos) { mg90s(); rotate([0,0,p[2]]) horn(180); }
                    rotate([0,0,p[2]]) { color(c_red) tibia(); color(c_red) tibia_cap(); }
                }
            }
        }
    }
}
module electronics() {
    color([0.1,0.55,0.25]) translate([-42.5, -28, body_z0+body_h+lid_t+5]) cube([85,56,1.6]);
    for (p=pca_pos) color([0.1,0.3,0.7]) translate([p[0]-31.25, p[1]-12.7, body_z0+wall+5]) cube([62.5,25.4,1.6]);
    color([0.35,0.37,0.4]) translate([-bat_L/2, -bat_W/2, body_z0-bat_H]) cube([bat_L,bat_W,bat_H]);
}
module assembly(p=pose) {
    color(c_black) body();
    color(c_black) translate([0,0,body_z0+body_h]) lid();
    color(c_black) battery_cradle();
    electronics();
    for (i=[0:2]) {
        leg_place(i) leg(p);
        leg_place(i, true) leg(p);
    }
}

// =====================================================================
//  Tisková orientace
// =====================================================================
module print_coxa_L() { translate([0,0,fork_w]) rotate([90,0,0]) coxa(); }
module print_coxa_R() { mirror([0,1,0]) print_coxa_L(); }
module print_femur()  { translate([0,0,fork_w]) rotate([90,0,0]) femur(); }
module print_tibia_L(){ translate([0,0,tib_z1]) mirror([0,0,1]) tibia(); }
module print_tibia_R(){ mirror([1,0,0]) print_tibia_L(); }

if (part=="assembly")   assembly();
if (part=="leg")        leg();
if (part=="body")       translate([0,0,-body_z0]) body();
if (part=="lid")        lid();
if (part=="coxa_L")     print_coxa_L();
if (part=="coxa_R")     print_coxa_R();
if (part=="femur")      print_femur();
if (part=="tibia_L")    print_tibia_L();
if (part=="tibia_R")    print_tibia_R();
if (part=="tibia_cap")  translate([0,0,-fork_zmin]) tibia_cap();
if (part=="bushing")    bushing();
if (part=="battery_cradle") translate([0,0,-(body_z0-bat_H-0.3-bat_lip_t)]) battery_cradle();
if (part=="tip_cap")    translate([0,0,5.5]) tip_cap();
if (part=="servo_test") translate([0,0,-hold_zbot]) servo_test();
