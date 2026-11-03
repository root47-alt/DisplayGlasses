// DisplayGlasses wearable parts: DRAFT v1
//
// A printed frame with a display module in place of the right lens, and a
// battery pod on the right temple. The module keeps the light path straight
// (screen -> lens -> beam splitter -> eye), so it sticks out past the side of
// your head like Google Glass. Every size is a parameter: put in your own
// face measurements and the numbers from the optics bench before printing.
//
// Coordinates: X to your right, Y forward, Z up, with the origin at the
// pupil of your right eye. All sizes in millimeters.
//
// Open in OpenSCAD, show Window > Customizer, pick a part, F6, export STL.

/* [What to make] */
part = "assembly"; // [assembly:Everything on a head outline (preview only), frame:Frame front, temple:Temple (print two), module_lower:Display module bottom half, module_upper:Display module top half, lens_ring:Lens ring, screen_sled:Screen sled, pod_body:Battery pod, pod_lid:Battery pod lid]

/* [Your face] */
// Distance between your pupils, for far vision (an optician or a phone app can measure it)
pd = 58.5;
// Width of the frame front, end to end (measure glasses that fit you)
frame_width = 140;
// On glasses that fit you: from the hinge to where the arm bends down behind your ear
temple_length = 100;
// Gap between the two lenses at your nose, on glasses that fit you
bridge_width = 16.5;
// Width of your head just above your ears (a phone app's "face width" is close enough). The arms angle out to it.
head_width = 150;
// How far in front of your pupils the frame front sits
frame_y = 15;

/* [Optics (from the optics bench)] */
// Pupil to the center of the beam splitter
eye_to_splitter = 25;
// Lens center to splitter center
lens_to_splitter = 12.8;
// Screen to lens with the text in focus (middle of the focus range)
screen_to_lens = 58.2;
// How far the screen can slide each way to fine-tune focus
focus_travel = 4;
// Lens diameter
lens_d = 30;
// Lens thickness at the middle (a 30 mm, f60 plano-convex is about 6)
lens_t = 6;
// Round plate beam splitter diameter
plate_d = 25;
// Plate thickness
plate_t = 1;

/* [Fit on your printer] */
clearance = 0.25;
wall = 1.8;

/* [Hidden] */
$fn = 64;
eps = 0.01;

// Adafruit 0.96" OLED (#326)
oled_w = 29.2;     // along Y in the module (the screen's long side)
oled_h = 26.7;     // along Z
oled_stack = 6.2;  // glass + board + connectors
// Adafruit Feather (#5477 / #5400) and battery (#3898)
feather = [52.3, 22.8, 7.2];
battery = [36, 17, 7.8];

// --- Module layout (derived) ---
ay = eye_to_splitter;                   // optical axis: the line y = ay, z = 0, along X
tiy = max(lens_d / 2 + 1, oled_w / 2 + 2.6);  // tube inner half-size across Y
tiz = max(lens_d / 2 + 1, oled_h / 2 + 2.6);  // tube inner half-size across Z
ring_t = lens_t + 1.6;                  // lens ring thickness along X
x_lens = lens_to_splitter;
x_tube0 = x_lens - ring_t / 2 - 1.4;    // where the tube starts (the splitter's corner is just short of it)
x_screen_far = x_lens + screen_to_lens + focus_travel;  // screen glass at its farthest
sled_t = oled_stack + 4.2;              // sled length along X
x_tube1 = x_screen_far + sled_t + 1.5;  // inside of the end wall
plate_r = plate_d / 2;
groove_depth = 3;
z_bracket = sqrt(plate_r * plate_r - pow(plate_r - groove_depth, 2)) > 0 ? plate_r - groove_depth : 0;

// Temple
temple_t = 3.6;  // thickness along X
temple_h = 7;    // height along Z
tongue_len = 9;  // the end that slides into a socket; its pin hole is 3 mm from the tip
grip = 2;        // how far each arm presses in against your head

// Frame
fy0 = frame_y;
fy1 = frame_y + 4;
rim_h = 42;
band = 3.5;
x_left_eye = -pd;
x_mid = -pd / 2;                        // the middle of your nose
x_left_end = x_mid - frame_width / 2;
x_rim_in = x_mid - bridge_width / 2;    // the left rim's edge at your nose...
x_rim_out = x_left_end + 5;             // ...and at the side of your head
rim_w = x_rim_in - x_rim_out;
x_post = x_mid + bridge_width / 2 - 1.5;  // the post on the right side of your nose
x_temple_l = x_left_end + 4;            // outer face of the left temple
x_temple_r = x_mid + frame_width / 2 - 4 - temple_t;  // inner face of the right temple, mirroring the left
joint_x = [24, 32];                     // the brow bar's screw slots (nominal pupil distance)

// Both temple sockets end at the module's back wall, so the two temples sit
// the same distance back and can be printed the same.
y_socket = ay - tiy - wall;             // front end of each temple slot
y_pin = y_socket - 3;
// The bend lands where it does on your own glasses, taking their hinge to be
// about 5 mm behind the frame front.
arm_len = y_socket - fy0 + temple_length;  // straight part, from the end of the tongue
pin_to_bend = arm_len + 2;
// Angle each arm out so it meets your head with a light grip.
splay = max(0, atan((head_width / 2 - grip - (frame_width / 2 - 4 - temple_t)) / pin_to_bend));

// --------------------------------------------------------------- dispatch

if (part == "assembly") assembly();
else if (part == "frame") print_frame();
else if (part == "temple") print_temple();
else if (part == "module_lower") print_module_lower();
else if (part == "module_upper") print_module_upper();
else if (part == "lens_ring") print_lens_ring();
else if (part == "screen_sled") print_screen_sled();
else if (part == "pod_body") print_pod_body();
else if (part == "pod_lid") print_pod_lid();

// ----------------------------------------------------------------- helpers

module box(x0, x1, y0, y1, z0, z1) { translate([x0, y0, z0]) cube([x1 - x0, y1 - y0, z1 - z0]); }

module rounded_rect(w, h, r) {
  offset(r = r) square([w - 2 * r, h - 2 * r], center = true);
}

// --------------------------------------------------------- display module
//
// A tube along X at eye height in front of your eye, split into a bottom
// and top half at z = 0. The lens ring drops in between two ribs, the
// screen sled slides near the far end, and two brackets hold the round
// splitter plate at 45 degrees in front of your eye.

module tube_outer() {
  box(x_tube0 - wall, x_tube1 + wall, ay - tiy - wall, ay + tiy + wall, -tiz - wall, tiz + wall);
}

module tube_inner() {
  box(x_tube0 - wall - eps, x_tube1, ay - tiy, ay + tiy, -tiz, tiz);
}

// Keeps the back of the tube away from the outer corner of your eye.
module eye_relief() {
  hull() {
    box(x_tube0 - wall - 1, x_tube0 + 6, ay - tiy - wall - 1, ay - tiy + 1.5, -tiz - wall - 1, tiz + wall + 1);
    box(x_tube0 - wall - 1, x_tube0 + 22, ay - tiy - wall - 1, ay - tiy - wall + eps, -tiz - wall - 1, tiz + wall + 1);
  }
}

module ribs() {
  // Stops on both sides of the lens ring, and rails that guide the sled.
  for (x = [x_lens - ring_t / 2 - clearance - 1.2, x_lens + ring_t / 2 + clearance]) {
    difference() {
      box(x, x + 1.2, ay - tiy, ay + tiy, -tiz, tiz);
      box(x - eps, x + 1.2 + eps, ay - tiy + 2.2, ay + tiy - 2.2, -tiz + 2.2, tiz - 2.2);
    }
  }
}

// The splitter brackets: a strip along the 45 degree line with a groove.
module splitter_bracket(top) {
  zf = top ? plate_r - groove_depth : -(plate_r - groove_depth);  // the bracket's face toward the eye line
  zb = top ? tiz + wall : -tiz - wall;
  translate([0, ay, 0]) rotate([0, 0, 45]) difference() {
    translate([-plate_r - 2, -4, min(zf, zb)]) cube([plate_r + 2 + x_tube0 * sqrt(2) + 2, 8, abs(zb - zf)]);
    // groove for the plate's edge, cut into the face toward the eye line
    translate([-plate_r - 3, -(plate_t / 2 + clearance), top ? zf - eps : zf - groove_depth])
      cube([2 * plate_r + 6, plate_t + 2 * clearance, groove_depth + eps]);
  }
}

// The slot and pin hole for a temple's tongue, with the temple's inner or
// outer face at x0, turned `a` degrees about the pin (positive swings the arm
// toward +X as it runs back).
module temple_slot(x0, a) {
  pivot = [x0 + temple_t / 2, y_pin, 0];
  translate(pivot) rotate([0, 0, a]) translate(-pivot) {
    translate([x0 - clearance, y_socket - 6 - eps, 3 - clearance]) cube([temple_t + 2 * clearance, 6 + eps, temple_h + 2 * clearance]);
    translate([x0 - 5, y_pin, 3 + temple_h / 2]) rotate([0, 90, 0]) cylinder(d = 2.1, h = temple_t + 10);
  }
}

module module_body() {
  difference() {
    union() {
      tube_outer();
      // temple socket on the back wall (full height of the top half, so it prints without support)
      box(x_temple_r - 4, x_temple_r + temple_t + 4, y_socket - 6, y_socket + eps, 0.05, tiz + wall);
    }
    tube_inner();
    eye_relief();
    temple_slot(x_temple_r, splay);
    // pilot holes for the M2 screws that hold the brow bar
    for (x = joint_x) translate([x, (fy0 + fy1) / 2, tiz - 3]) cylinder(d = 1.7, h = wall + 4);
    // focus slot in the top: the sled's tab sticks out here
    box(x_lens + screen_to_lens - focus_travel - 1.5 + 2, x_lens + screen_to_lens + focus_travel + 3.5 + 2,
        ay - 3, ay + 3, tiz - eps, tiz + wall + eps);
    // cable exit in the end wall
    box(x_tube1 - eps, x_tube1 + wall + eps, ay - 6, ay + 6, -tiz + 2, -tiz + 8);
  }
  ribs();
  splitter_bracket(true);
  splitter_bracket(false);
  // bosses under the brow bar's screws, so they have plastic to bite into
  for (x = joint_x) difference() {
    translate([x, (fy0 + fy1) / 2, tiz - 3]) cylinder(d = 5, h = 3 + eps);
    translate([x, (fy0 + fy1) / 2, tiz - 3 - eps]) cylinder(d = 1.7, h = 3 + 2 * eps);
  }
}

module module_lower() { intersection() { module_body(); box(-60, 200, -60, 120, -60, 0); } }
module module_upper() { intersection() { module_body(); box(-60, 200, -60, 120, 0, 60); } }

// Lens ring: holds the lens with three crush ribs, sits between the stops.
module lens_ring() {
  r = lens_d / 2 + clearance;
  difference() {
    box(x_lens - ring_t / 2, x_lens + ring_t / 2, ay - tiy + clearance, ay + tiy - clearance, -tiz + clearance, tiz - clearance);
    translate([x_lens - ring_t / 2 - eps, ay, 0]) rotate([0, 90, 0]) cylinder(r = lens_d / 2 - 1.5, h = ring_t + 2 * eps);
    translate([x_lens - ring_t / 2 + 1.2, ay, 0]) rotate([0, 90, 0]) cylinder(r = r, h = ring_t);
  }
  for (a = [90, 210, 330])
    translate([x_lens - ring_t / 2 + 1.2, ay, 0]) rotate([a, 0, 0])
      translate([0, r - 0.45, -0.5]) cube([ring_t - 2.2, 0.75, 1]);
}

// Screen sled: the OLED sits face-in on four corner posts; a tab sticks out
// of the top so you can slide it to focus.
module screen_sled(x_glass) {
  back = x_glass + oled_stack + 0.6;  // inner face of the sled's back plate
  difference() {
    union() {
      box(back, back + 2.4, ay - tiy + clearance, ay + tiy - clearance, -tiz + clearance, tiz - clearance);
      // side skids that ride on the tube walls
      for (s = [-1, 1]) box(x_glass - 1, back + 2.4, s > 0 ? ay + tiy - clearance - 1.6 : ay - tiy + clearance,
                            s > 0 ? ay + tiy - clearance : ay - tiy + clearance + 1.6, -tiz + clearance, tiz - clearance);
      // corner posts the board's back rests on
      for (sy = [-1, 1], sz = [-1, 1])
        box(x_glass + oled_stack - 3.6, back + eps, ay + sy * (oled_w / 2 - 2.5) - 2.5, ay + sy * (oled_w / 2 - 2.5) + 2.5,
            sz * (oled_h / 2 - 2.5) - 2.5, sz * (oled_h / 2 - 2.5) + 2.5);
      // focus tab
      box(back - 1, back + 2.4, ay - 2.2, ay + 2.2, tiz - clearance - eps, tiz + wall + 3);
    }
    // window for the board's connectors and cable
    box(back - eps, back + 2.4 + eps, ay - 9, ay + 9, -8, 8);
  }
}

// ------------------------------------------------------------------ frame
//
// Left rim, bridge, and a brow bar over the right eye that bolts onto the
// display module through two slots (slide it to line the splitter up with
// your pupil). The right "lens" opening is where the display module sits.

module frame_profile() {
  // left rim, from the side of your nose out to the end piece
  translate([(x_rim_in + x_rim_out) / 2, 1]) difference() {
    rounded_rect(rim_w, rim_h, 12);
    rounded_rect(rim_w - 2 * band, rim_h - 2 * band, 12 - band);
  }
  // bridge, arched over the nose: as wide as the gap on your own glasses
  difference() {
    translate([x_rim_in - band, 5]) square([x_post + 3.5 - (x_rim_in - band), 7]);
    translate([x_mid, 0]) circle(r = bridge_width / 2);
  }
  // nose-side post and brow bar over the right eye
  translate([x_post, 5]) square([3.5, 19]);
  translate([x_post, 18]) square([joint_x[1] + 6 - x_post, 6]);
  // left end piece
  translate([x_left_end, -1]) square([x_rim_out + band - x_left_end, 14]);
}

module frame() {
  difference() {
    union() {
      translate([0, fy1, 0]) rotate([90, 0, 0]) linear_extrude(fy1 - fy0) frame_profile();
      // thicker block where the brow bar meets the module
      box(joint_x[0] - 6, joint_x[1] + 6, fy0 - 4, fy1, 18, 24);
      // left temple socket, reaching back as far as the right one on the module
      box(x_left_end, x_left_end + temple_t + 8, y_socket - 6, fy0 + eps, -1, 13);
    }
    // screw slots (M2), long along X for adjusting
    for (x = joint_x) hull() for (dx = [-5, 5]) translate([x + dx, (fy0 + fy1) / 2 - 2, 17]) cylinder(d = 2.4, h = 8);
    temple_slot(x_temple_l, -splay);
  }
}

// ----------------------------------------------------------------- temple
//
// A straight arm with a tongue at the front (slides into a socket, held by a
// 2 mm pin or screw) and a gentle bend down behind the ear.

module temple_profile() {
  // in the Y/Z plane: the tongue's tip is at y = -tongue_len, the arm runs toward +y
  translate([-tongue_len, 0]) square([tongue_len + arm_len, temple_h]);
  translate([arm_len - 4, temple_h]) rotate(-35) translate([0, -temple_h]) square([38, temple_h]);
}

module temple() {
  difference() {
    rotate([90, 0, 90]) linear_extrude(temple_t) temple_profile();
    translate([-1, -tongue_len + 3, temple_h / 2]) rotate([0, 90, 0]) cylinder(d = 2.1, h = temple_t + 2);
  }
}

// ------------------------------------------------------------ battery pod
//
// Holds the Feather on top of the 400 mAh battery. Slides onto the right
// temple from the back; the lid slides in from the front.

pod_in = [feather[0] + 1.5, feather[1] + 1.5, feather[2] + battery[2] + 1.6];  // along temple, height, depth
pod_wall = 1.4;
pod_out = pod_in + [2 * pod_wall, 2 * pod_wall, pod_wall];

module pod_body() {
  difference() {
    cube(pod_out);  // x: along the temple, y: height, z: depth (the open side is z = top)
    translate([pod_wall, pod_wall, pod_wall]) cube(pod_in + [0, 0, 1]);
    // lid grooves along both long sides
    for (y = [pod_wall - 0.8, pod_out[1] - pod_wall - eps]) translate([-eps, y, pod_out[2] - 2.2]) cube([pod_out[0] + 2 * eps, 0.8 + eps, 1.2]);
    // USB-C and battery plug end
    translate([-eps, pod_wall + 2, pod_wall + battery[2] + 0.4]) cube([pod_wall + 2 * eps, pod_in[1] - 4, feather[2] + 1]);
    // screen cable end
    translate([pod_out[0] - pod_wall - eps, pod_out[1] / 2 - 5, pod_wall + battery[2] + 2]) cube([pod_wall + 2 * eps, 10, 6]);
  }
  // rail that slides onto the temple (on the side facing your head)
  translate([0, (pod_out[1] - temple_h) / 2 - 1.6, -temple_t - 2 * clearance - 1.6]) difference() {
    cube([pod_out[0], temple_h + 3.2, temple_t + 2 * clearance + 1.6 + eps]);
    translate([-eps, 1.6 - clearance, 1.6]) cube([pod_out[0] + 2 * eps, temple_h + 2 * clearance, temple_t + 2 * clearance + eps]);
    translate([-eps, 1.6 + 1.5, -eps]) cube([pod_out[0] + 2 * eps, temple_h - 3, 1.6 + 2 * eps]);
  }
}

module pod_lid() {
  difference() {
    union() {
      cube([pod_out[0], pod_in[1] - 2 * clearance, 1.2]);
      // tongues that ride in the grooves
      for (y = [-0.8 + clearance, pod_in[1] - 2 * clearance]) translate([0, y, 0]) cube([pod_out[0], 0.8 - clearance, 1]);
    }
    // window over the reset and boot buttons, at the USB end
    translate([3, 3, -eps]) cube([16, pod_in[1] - 2 * clearance - 6, 1.2 + 2 * eps]);
  }
}

// --------------------------------------------------------- print layouts

// The frame prints lying on its front face.
module print_frame() { translate([0, 0, fy1]) rotate([-90, 0, 0]) frame(); }
module print_temple() { rotate([0, 90, 0]) translate([-temple_t, 0, 0]) temple(); }
module print_module_lower() { translate([0, 0, tiz + wall]) module_lower(); }
// The top half prints upside down, on its outer face.
module print_module_upper() { translate([0, 0, tiz + wall]) rotate([180, 0, 0]) module_upper(); }
module print_lens_ring() { rotate([0, -90, 0]) translate([-(x_lens - ring_t / 2), 0, 0]) lens_ring(); }
module print_screen_sled() { rotate([0, 90, 0]) translate([-(x_lens + screen_to_lens + oled_stack + 3.0), 0, 0]) screen_sled(x_lens + screen_to_lens); }
module print_pod_body() { translate([0, 0, temple_t + 2 * clearance + 1.6]) pod_body(); }
module print_pod_lid() { pod_lid(); }

// --------------------------------------------------------------- assembly

module assembly() {
  // head outline, eyes and pupils for scale
  %translate([x_mid, -92, -5]) scale([75, 100, 105]) sphere(r = 1, $fn = 48);
  for (x = [0, x_left_eye]) {
    color("White") translate([x, -9, 0]) sphere(r = 12);
    color("Black") translate([x, 2.7, 0]) sphere(r = 1.8);
  }

  color("SlateGray") frame();
  color("SlateGray") temple_placed(x_temple_l, -splay) temple_in_place();
  color("SlateGray") temple_placed(x_temple_r, splay) temple_in_place();

  color("Orange") module_lower();
  color("Orange", 0.85) module_upper();
  color("Gold") lens_ring();
  color("Gold") screen_sled(x_lens + screen_to_lens);

  // optics
  color([0.6, 0.85, 1, 0.55]) translate([x_lens, ay, 0]) rotate([0, 90, 0]) resize([lens_d, lens_d, lens_t]) sphere(d = lens_d);
  color([0.4, 0.7, 0.9, 0.5]) translate([0, ay, 0]) rotate([0, 0, 45]) rotate([90, 0, 0]) cylinder(d = plate_d, h = plate_t, center = true);
  color("DarkGreen") translate([x_lens + screen_to_lens + 1.6, ay - oled_w / 2, -oled_h / 2]) cube([1.6, oled_w, oled_h]);
  color("Black") translate([x_lens + screen_to_lens, ay - 13.3, -9.5]) cube([1.6, 26.6, 19]);
  // light path
  color("Gold") {
    translate([0, ay, 0]) rotate([0, 90, 0]) cylinder(d = 0.8, h = x_lens + screen_to_lens, $fn = 8);
    translate([0, ay, 0]) rotate([90, 0, 0]) cylinder(d = 0.8, h = ay - 3, $fn = 8);
  }

  // battery pod on the right temple, a little behind the module
  color("Teal") temple_placed(x_temple_r, splay)
    translate([temple_t + clearance + 1.6, -18 - pod_out[0], temple_h / 2 - pod_out[1] / 2]) rotate([90, 0, 90]) pod_in_place();
}

// Puts a temple (or something riding on it) in its socket: the tongue's tip
// at the slot's front end, turned `a` degrees about the pin.
module temple_placed(x0, a) {
  translate([x0 + temple_t / 2, y_pin, 3]) rotate([0, 0, a]) translate([-temple_t / 2, 3, 0]) children();
}

module temple_in_place() {
  // temple() is built with the arm running toward +Y; flip it so the tip is at
  // the origin and the arm runs back toward -Y
  mirror([0, 1, 0]) translate([0, tongue_len, 0]) temple();
}

module pod_in_place() {
  translate([pod_out[0], 0, 0]) mirror([1, 0, 0]) pod_body();
}
