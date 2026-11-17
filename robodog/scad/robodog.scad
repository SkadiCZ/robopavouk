// =====================================================================
//  ROBODOG mini  –  3D tisknutelný čtyřnohý robot (12x MG90S, RPi 4B)
//  Parametrický model v OpenSCAD.
//
//  Použití:
//    openscad -D 'part="assembly"' robodog.scad        (náhled sestavy)
//    openscad -D 'part="femur"' -o femur.stl robodog.scad
//
//  part = assembly | body | lid | shoulder_L | shoulder_R | femur |
//         tibia | bushing | foot_cap | servo_test | plate_all
//
//  Souřadnice sestavy: X dopředu, Y doleva, Z nahoru, 0 = spodek trupu.
//  Všechny rozměry v mm.
// =====================================================================

part = "assembly";
$fn  = 48;

// ---------------- tolerance ----------------
clr = 0.3;          // vůle kolem serva (zvětšit při těsném lícování)

// ---------------- MG90S (počátek = osa hřídele na horní ploše pouzdra) ----
sv_L        = 22.8;   // délka pouzdra
sv_W        = 12.2;   // šířka pouzdra
sv_H        = 22.5;   // výška pouzdra (bez převodovky)
sv_sx       = 5.9;    // osa hřídele od bližšího čela pouzdra
sv_cx       = sv_L/2 - sv_sx;   // střed pouzdra vůči hřídeli (+5.5)
sv_tab_bot  = -6.6;   // spodní plocha úchytných oušek
sv_tab_t    = 2.5;    // tloušťka oušek
sv_tab_span = 32.2;   // celková délka přes ouška
sv_hole_sp  = 27.8;   // rozteč děr v ouškách
sv_boss_d   = 11.8;   // válcový výstupek převodovky
sv_boss_h   = 4.0;
sv_spline_d = 4.8;    // drážkovaný hřídel
sv_spline_h = 2.5;

// ---------------- jednoramenná páka (horn) ze sady MG90S ----------------
horn_top    = 8.0;    // horní plocha páky nad pouzdrem
horn_arm_t  = 1.8;    // tloušťka ramene
horn_hub_d  = 7.6;    // průměr náboje (+vůle)
horn_arm_len= 17.5;   // od osy po konec ramene
horn_tip_d  = 4.8;    // šířka konce ramene
horn_screw_access = 5.5;  // otvor pro šroubovák na středový šroub

// ---------------- držák serva (holder) ----------------
hold_y    = 10;                                  // polovina šířky držáku
floor_t   = 4;                                   // dno s kapsou pro matici M3
hold_x0   = sv_cx - sv_tab_span/2 - 2.3;         // -12.9
hold_x1   = sv_cx + sv_tab_span/2 + 2.3;         //  23.9
hold_ztop = sv_tab_bot + sv_tab_t;               //  -4.1
hold_zbot = -sv_H - floor_t;                     // -26.5

// ---------------- vidlice (fork) ----------------
gap      = 4;                       // montážní vůle (nasunutí na drážkování)
hp_z0    = horn_top - horn_arm_t - 0.2;   // spodní plocha deska u páky (6.0)
hp_t     = 4.5;
pp_t     = 4;
pp_z1    = hold_zbot - gap;               // horní plocha čepové desky (-30.5)
fork_zmin= pp_z1 - pp_t;                  // -34.5
fork_zmax= hp_z0 + hp_t;                  //  10.5
fork_r0  = 17;                            // vnitřní poloměr stěny vidlice
fork_r1  = 20;
fork_w   = hold_y;
bush_od  = 6;
bush_len = pp_t + gap + 0.3;

// ---------------- noha ----------------
L1 = 55;        // stehno: osa ramene -> osa kolene
L2 = 52;        // bérec: osa kolene -> střed chodidla
foot_r = 10;

// ---------------- trup ----------------
Lb = 150; Wb = 70; Hb = 50;
fw = sv_tab_bot*0 + 7.1;   // přední/zadní stěna (nese kyčelní serva)
sw = 2.5;                  // boční stěny
bt = 2.5;                  // dno
lid_t = 3;
xH = Lb/2 - 3;             // osa kyčelního serva: hřídel 3 mm za čelem
yH = 26;
zH = 14;

// poloha ramenního serva vůči kyčelnímu (v lokálních souř. kyčelního serva)
sh_dx = hp_z0 + hold_y;    // 16 mm dopředu
sh_dy = 32;                // 32 mm do strany
sh_dz = -38;               // 38 mm dolů

// pózy pro náhled
pose_abd = 0; pose_hip = 35; pose_knee = -70;

// =====================================================================
//  MG90S + páka (jen pro náhled)
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

// otisk páky do desky (kapsa zespodu + otvor na šroubovák)
module horn_cut(ang=180) {
    translate([0,0,hp_z0-1]) cylinder(d=horn_hub_d, h=horn_top-hp_z0+1.2);
    rotate([0,0,ang]) hull() {
        translate([0,0,hp_z0-1]) cylinder(d=horn_hub_d, h=horn_top-hp_z0+1.2);
        translate([horn_arm_len-horn_tip_d/2,0,hp_z0-1]) cylinder(d=horn_tip_d, h=horn_top-hp_z0+1.2);
    }
    translate([0,0,-1]) cylinder(d=horn_screw_access, h=40);
}

// =====================================================================
//  Držák serva: kapsa otevřená k +Z, ouška na římse, dno s maticí M3
// =====================================================================
module holder_solid(x1=hold_x1) {
    translate([hold_x0, -hold_y, hold_zbot]) cube([x1-hold_x0, 2*hold_y, hold_ztop-hold_zbot]);
}

module holder_cut() {
    // pouzdro
    translate([-sv_sx-clr, -sv_W/2-clr, -sv_H-clr]) cube([sv_L+2*clr, sv_W+2*clr, 60]);
    // ouška
    translate([sv_cx - sv_tab_span/2 - clr, -sv_W/2-clr, sv_tab_bot-clr]) cube([sv_tab_span+2*clr, sv_W+2*clr, 60]);
    // díry pro samořezné šrouby M2 do oušek
    for (s=[-1,1]) translate([sv_cx + s*sv_hole_sp/2, 0, sv_tab_bot-9]) cylinder(d=1.7, h=10, $fn=16);
    // kanálky pro kabel na obou čelech
    for (xe=[-sv_sx-clr-1.6, sv_L-sv_sx+clr-0.4])
        translate([xe, -2.2, -sv_H-clr]) cube([2, 4.4, 60]);
    // čep: díra M3 + boční kapsa pro matici M3
    translate([0,0,hold_zbot-1]) cylinder(d=3.3, h=floor_t+2, $fn=20);
    translate([0,0,hold_zbot+0.5]) hull() {
        cylinder(d=6.5, h=2.6, $fn=6);
        translate([0,-hold_y-1,0]) cylinder(d=6.5, h=2.6, $fn=6);
    }
}

// =====================================================================
//  Vidlice: deska na páce (nahoře), čepová deska (dole), stěna u -X
// =====================================================================
module fork_plate2d() {
    hull() {
        circle(r=hold_y);
        translate([-fork_r1, -fork_w]) square([fork_r1-fork_r0, 2*fork_w]);
    }
}
module fork_solid() {
    translate([0,0,hp_z0]) linear_extrude(hp_t) fork_plate2d();
    translate([0,0,fork_zmin]) linear_extrude(pp_t) fork_plate2d();
    translate([-fork_r1, -fork_w, fork_zmin]) cube([fork_r1-fork_r0, 2*fork_w, fork_zmax-fork_zmin]);
}
module fork_cut() {
    horn_cut(180);
    translate([0,0,fork_zmin-1]) cylinder(d=bush_od+0.3, h=pp_t+2);
}

// =====================================================================
//  DÍLY
// =====================================================================
// --- stehno: vidlice kolem ramenního serva + držák kolenního serva ----
module femur() {
    difference() {
        union() {
            fork_solid();
            translate([-(L1-hold_x1), -fork_w, fork_zmin])
                cube([L1-hold_x1-fork_r0+0.01, 2*fork_w, fork_zmax-fork_zmin]);
            translate([-L1,0,0]) holder_solid();
        }
        fork_cut();
        translate([-L1,0,0]) holder_cut();
        // odlehčení + průchod kabelu kolenního serva
        translate([-(L1-hold_x1)+4, -fork_w-1, -26]) cube([L1-hold_x1-fork_r0-8, 2*fork_w+2, 26]);
    }
}

// --- bérec: vidlice kolem kolenního serva + chodidlo ----
module tibia() {
    fz = (fork_zmin+fork_zmax)/2;
    difference() {
        union() {
            fork_solid();
            hull() {
                translate([-fork_r1, -fork_w, fork_zmin]) cube([fork_r1-fork_r0, 2*fork_w, fork_zmax-fork_zmin]);
                translate([-L2, 0, fz]) sphere(r=foot_r);
            }
        }
        fork_cut();
        // odlehčovací okna
        hull() {
            translate([-fork_r1-6, -fork_w-1, fz-12]) rotate([-90,0,0]) cylinder(d=10, h=2*fork_w+2);
            translate([-L2+18, -fork_w-1, fz-4])  rotate([-90,0,0]) cylinder(d=6, h=2*fork_w+2);
        }
    }
}

// --- rameno: deska na páce kyčelního serva + držák ramenního serva ----
module S_rel() { translate([sh_dz, -sh_dy, sh_dx]) rotate([90,0,0]) children(); }
module H_disc() { translate([0,0,hp_z0]) cylinder(r=12, h=hp_t); }
module shoulder_H() {          // v lokálních souřadnicích kyčelního serva
    difference() {
        union() {
            H_disc();
            S_rel() holder_solid();
            hull() {
                H_disc();
                S_rel() translate([hold_x1-0.01, -hold_y, hold_zbot]) cube([5, 2*hold_y, hold_ztop-hold_zbot]);
            }
        }
        horn_cut(-120);
        S_rel() holder_cut();
    }
}

// --- trup ----
module H_frame() { multmatrix([[0,0,1,xH],[0,-1,0,yH],[1,0,0,zH],[0,0,0,1]]) children(); }
module corners() {
    for (mx=[0,1], my=[0,1]) mirror([mx,0,0]) mirror([0,my,0]) children();
}
module H_wall_cut() {
    translate([-sv_sx-clr, -sv_W/2-clr, -10]) cube([sv_L+2*clr, sv_W+2*clr, 10+clr]);
    hull() { translate([0,0,-1]) cylinder(d=13, h=6); translate([6.5,0,-1]) cylinder(d=8.5, h=6); }
    for (s=[-1,1]) translate([sv_cx + s*sv_hole_sp/2, 0, -6]) cylinder(d=1.7, h=7.5, $fn=16);
}
lid_posts = [[ Lb/2-fw-3.5, 0], [-(Lb/2-fw-3.5), 0],
             [ 35,  Wb/2-sw-3.5], [ 35, -(Wb/2-sw-3.5)],
             [-35,  Wb/2-sw-3.5], [-35, -(Wb/2-sw-3.5)]];
module body() {
    difference() {
        union() {
            difference() {
                translate([-Lb/2, -Wb/2, 0]) cube([Lb, Wb, Hb]);
                translate([-Lb/2+fw, -Wb/2+sw, bt]) cube([Lb-2*fw, Wb-2*sw, Hb]);
            }
            for (p=lid_posts) translate([p[0],p[1],0]) cylinder(d=8, h=Hb);
        }
        corners() H_frame() H_wall_cut();
        for (p=lid_posts) translate([p[0],p[1],Hb-12]) cylinder(d=2.6, h=13, $fn=16);
        // mřížka děr M3 v podlaze (baterie, step-down měniče)
        for (x=[-40:10:40], y=[-20:10:20]) translate([x,y,-1]) cylinder(d=3.2, h=bt+2, $fn=16);
        // průchody pro suchý zip baterie
        for (x=[-30,30]) translate([x-10,-Wb/2+sw+1,-1]) cube([20,3,bt+2]);
        for (x=[-30,30]) translate([x-10, Wb/2-sw-4,-1]) cube([20,3,bt+2]);
        // zářezy pro kabely nohou v horní hraně čela/zadní stěny
        for (mx=[0,1], s=[-1,1]) mirror([mx,0,0])
            translate([Lb/2-fw-1, s*12-6, Hb-8]) cube([fw+2, 12, 9]);
        // vypínač / nabíjecí konektor v boku
        translate([0, -Wb/2-1, 20]) rotate([-90,0,0]) cylinder(d=12.2, h=sw+2);
    }
}

// --- víko s držáky Raspberry Pi 4B a PCA9685 ----
pi_x = -22;  pca_x = 47;
module lid() {
    difference() {
        union() {
            translate([-Lb/2, -Wb/2, 0]) cube([Lb, Wb, lid_t]);
            // Raspberry Pi 4B: díry 58 x 49
            for (x=[-29,29], y=[-24.5,24.5]) translate([pi_x+x, y, 0]) cylinder(d=6, h=lid_t+5);
            // PCA9685: díry 55.9 x 19.05 (delší strana napříč)
            for (x=[-9.525,9.525], y=[-27.95,27.95]) translate([pca_x+x, y, 0]) cylinder(d=5.5, h=lid_t+5);
        }
        for (x=[-29,29], y=[-24.5,24.5]) translate([pi_x+x, y, -1]) cylinder(d=2.2, h=20, $fn=16);
        for (x=[-9.525,9.525], y=[-27.95,27.95]) translate([pca_x+x, y, -1]) cylinder(d=2.2, h=20, $fn=16);
        for (p=lid_posts) translate([p[0],p[1],-5]) cylinder(d=3.4, h=20, $fn=16);
        // kabelové průchody
        translate([pca_x-25, -15, -5]) cube([8, 30, 20]);
        translate([pi_x-6, -6, -5]) cube([12, 12, 20]);
        // odlehčení pod Pi (chlazení)
        for (x=[-20:10:20]) translate([pi_x+x-3, -18, -5]) cube([6, 36, 20]);
    }
}

// --- drobnosti ----
module bushing() {
    difference() { cylinder(d=bush_od, h=bush_len); translate([0,0,-1]) cylinder(d=3.4, h=bush_len+2, $fn=20); }
}
module foot_cap() {     // TPU návlek na chodidlo
    difference() {
        sphere(r=foot_r+2);
        sphere(r=foot_r+0.1);
        translate([-20,-20,3]) cube([40,40,40]);
    }
}
// zkušební kus: držák serva + vidlice, ověří lícování před tiskem všeho
module servo_test() {
    difference() { holder_solid(); holder_cut(); }
    // vedle: kousek desky s otiskem páky (otisk nahoru)
    translate([0, 30, hold_zbot + hp_z0 + hp_t]) mirror([0,0,1]) difference() {
        translate([0,0,hp_z0]) linear_extrude(hp_t) hull() { circle(r=8); translate([-20,0]) circle(r=5); }
        horn_cut(180);
    }
}

// =====================================================================
//  SESTAVA
// =====================================================================
module leg(abd=0, hip=35, knee=-70, show_servos=true) {
    H_frame() {
        if (show_servos) { mg90s(); rotate([0,0,abd]) horn(-120); }
        rotate([0,0,abd]) {
            color([0.95,0.55,0.1]) shoulder_H();
            S_rel() {
                if (show_servos) { mg90s(); rotate([0,0,hip]) horn(180); }
                rotate([0,0,hip]) {
                    color([0.25,0.25,0.27]) femur();
                    translate([-L1,0,0]) {
                        if (show_servos) { mg90s(); rotate([0,0,knee]) horn(180); }
                        rotate([0,0,knee]) {
                            color([0.95,0.55,0.1]) tibia();
                            color([0.1,0.1,0.1]) translate([-L2,0,(fork_zmin+fork_zmax)/2])
                                rotate([0,90,0]) foot_cap();
                        }
                    }
                }
            }
        }
    }
}

module electronics() {
    // Raspberry Pi 4B
    color([0.1,0.55,0.25]) translate([pi_x-42.5, -28, Hb+lid_t+5]) cube([85,56,1.6]);
    color("silver") translate([pi_x+42.5-21, -28+2, Hb+lid_t+6.6]) cube([21,53,14]);
    // PCA9685
    color([0.1,0.3,0.7]) translate([pca_x-12.7, -31.25, Hb+lid_t+5]) cube([25.4,62.5,1.6]);
    color("black") translate([pca_x-12.7, -31.25, Hb+lid_t+6.6]) cube([8,62.5,8]);
    // baterie 2x18650
    color([0.85,0.2,0.2]) translate([-38.5, -20.5, bt]) cube([77,41,21]);
}

module assembly(abd=pose_abd, hip=pose_hip, knee=pose_knee) {
    color([0.92,0.92,0.9]) body();
    color([0.92,0.92,0.9]) translate([0,0,Hb]) lid();
    electronics();
    // přední nohy: koleno dozadu; zadní nohy zrcadlené, koleno také dozadu
    leg(abd, hip, knee);
    mirror([0,1,0]) leg(abd, hip, knee);
    mirror([1,0,0]) leg(abd, -hip, -knee);
    mirror([1,0,0]) mirror([0,1,0]) leg(abd, -hip, -knee);
}

// =====================================================================
//  Orientace pro tisk (každý díl leží rovnou plochou na podložce)
// =====================================================================
module print_shoulder_L() { translate([0,0,-(xH+hp_z0)]) rotate([0,-90,0]) H_frame() shoulder_H(); }
module print_shoulder_R() { mirror([0,1,0]) print_shoulder_L(); }
module print_femur() { translate([0,0,fork_w]) rotate([90,0,0]) femur(); }
module print_tibia() { translate([0,0,fork_w]) rotate([90,0,0]) tibia(); }

if (part == "assembly")   assembly();
if (part == "body")       body();
if (part == "lid")        lid();
if (part == "shoulder_L") print_shoulder_L();
if (part == "shoulder_R") print_shoulder_R();
if (part == "femur")      print_femur();
if (part == "tibia")      print_tibia();
if (part == "bushing")    bushing();
if (part == "foot_cap")   rotate([180,0,0]) translate([0,0,-3]) foot_cap();
if (part == "servo_test") translate([0,0,-hold_zbot]) servo_test();
if (part == "leg")        leg(pose_abd, pose_hip, pose_knee);
