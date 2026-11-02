// DisplayGlasses optics bench
//
// A rail with a millimeter scale and sliding holders for the OLED screen, a
// lens and the beam splitter. Slide them until the floating text looks sharp,
// then read the distances off the scale. Those numbers go into the glasses
// module.
//
// How to use this file:
//   1. Open it in OpenSCAD (openscad.org) and show Window > Customizer.
//   2. Enter your measured part sizes, pick a part, press F6 (render).
//   3. File > Export > Export as STL, then slice and print.
// Ready-made STLs for the default sizes are in cad/stl/.
//
// Every size is in millimeters.

/* [What to make] */
part = "assembly"; // [assembly:Everything assembled (preview only), rail:Rail with scale, oled:Screen holder, lens:Lens holder, cube:Beam splitter cube holder, plate:Flat glass splitter holder, fit_test:Fit test (print this first)]

/* [Your parts (measure them)] */
// Screen board width. Adafruit 0.96 inch OLED: 29.2
oled_w = 29.2;
// Screen board height. Adafruit 0.96 inch OLED: 26.7
oled_h = 26.7;
// Circuit board thickness
oled_pcb_t = 1.6;
// Tallest part on the back of the screen board (its connectors)
oled_back_t = 3.0;
// How far the middle of the lit-up area sits above the middle of the board
oled_center_offset = 0;
// Lens diameter
lens_d = 25;
// Beam splitter cube side length
cube_size = 15;
// Thickness of a flat glass or plastic splitter (microscope slide: about 1)
plate_t = 1.2;
// Height of that flat splitter, standing in its holder
plate_h = 25;

/* [Fit on your printer] */
// Gap for sliding and drop-in fits. Parts too tight? Make it bigger.
clearance = 0.25;
// How hard the spring finger grips the rail. Holders slide too easily? Make it bigger.
press = 0.3;

/* [Bench size] */
// Rail length (150 fits a 180 mm print bed)
rail_len = 150;
// Height of the optical axis above the rail (28 fits lenses up to about 36 mm)
axis_h = 28;

/* [Hidden] */
$fn = 72;
eps = 0.01;

// Rail
rail_w = 34;
rail_h = 8;
ridge_w = 8;
ridge_h = 3;
ridge_ch = 0.8;   // chamfer on the ridge's top edges
ridge_y = 22;     // the ridge (and the optical axis) sits this far from the front edge
scale_x0 = 5;     // x of the "0" mark
scale_back = 13;  // back edge of the tick marks = front edge of the holders
tick_w = 0.5;
engrave = 0.6;

// Holder base (every holder rides on one of these)
base_w = 18;
base_t = 7;
groove_w = ridge_w + 2 * clearance;
groove_d = ridge_h + 0.4;
finger_t = 1.6;     // spring finger thickness
finger_gap = 0.8;   // slot that frees the finger
finger_root = 4;    // where the finger is attached
pointer_len = 4;
pointer_t = 1.2;

// Screen holder
oled_plate_t = 3;
oled_standoff = oled_back_t + 0.6;
oled_wall = 1.6;
oled_bracket = 6;  // length of each corner bracket arm
oled_lip = 0.5;    // little lips on the side brackets that snap over the board

// Lens holder
lens_lip = 1.2;    // ring the lens rests on
lens_depth = 4;    // depth of the lens pocket
lens_rib = 0.45;   // crush ribs: squeeze the lens so it can't fall out

// Example layout for the assembly preview (scale readings)
demo_oled_at = 8;
demo_lens_at = 50;
demo_cube_at = 80;

// ---------------------------------------------------------------- choose part

if (part == "assembly") assembly();
else if (part == "rail") rail();
else if (part == "oled") print_on_back() oled_holder();
else if (part == "lens") print_on_back() lens_holder();
else if (part == "cube") cube_holder();
else if (part == "plate") plate_holder();
else if (part == "fit_test") fit_test();

// The screen and lens holders print lying on their back face (no supports).
module print_on_back() { rotate([0, -90, 0]) children(); }

// -------------------------------------------------------------------- rail

module rail(len = rail_len, marks = true) {
  difference() {
    union() {
      cube([len, rail_w, rail_h]);
      translate([0, ridge_y, rail_h - eps]) ridge(len);
    }
    if (marks) scale_marks(len);
  }
}

// The key the holders slide on. Its ends taper so holders slip on easily.
module ridge(len) {
  taper = 4;
  hull() {
    ridge_section(ridge_w - 2, ridge_h - 1);
    translate([taper, 0, 0]) ridge_section(ridge_w, ridge_h);
  }
  hull() {
    translate([taper, 0, 0]) ridge_section(ridge_w, ridge_h);
    translate([len - taper, 0, 0]) ridge_section(ridge_w, ridge_h);
  }
  hull() {
    translate([len - taper, 0, 0]) ridge_section(ridge_w, ridge_h);
    translate([len - eps, 0, 0]) ridge_section(ridge_w - 2, ridge_h - 1);
  }
}

module ridge_section(w, h) {
  rotate([90, 0, 90]) linear_extrude(eps)
    polygon([[-w / 2, 0], [w / 2, 0], [w / 2, h - ridge_ch], [w / 2 - ridge_ch, h],
             [-w / 2 + ridge_ch, h], [-w / 2, h - ridge_ch]]);
}

// Millimeter ticks along the front edge, numbered every 10 mm.
module scale_marks(len) {
  n = floor(len - 2 * scale_x0);
  for (i = [0 : n]) {
    l = (i % 10 == 0) ? 5.5 : ((i % 5 == 0) ? 4 : 2.5);
    translate([scale_x0 + i - tick_w / 2, scale_back - l, rail_h - engrave])
      cube([tick_w, l + 0.5, engrave + eps]);
  }
  for (k = [0 : 10 : n])
    translate([scale_x0 + k, 2.2, rail_h - engrave])
      linear_extrude(engrave + eps)
        text(str(k), size = 3.2, halign = "center", font = "Liberation Sans:style=Bold");
}

// ------------------------------------------------------------ holder base
//
// Holder coordinates: x runs along the rail, y = 0 is the optical axis,
// z = 0 is the rail's top surface. `mx` is the x of the thing being measured
// (screen surface, lens middle, splitter center); the pointer marks it.

module carriage_base(len, mx) {
  difference() {
    translate([0, -base_w / 2, 0]) cube([len, base_w, base_t]);
    // groove that rides on the ridge
    translate([-eps, -groove_w / 2, -eps]) cube([len + 2 * eps, groove_w, groove_d + eps]);
    // small chamfer along the groove's bottom edges (a squished first layer
    // would otherwise make it tight)
    translate([-eps, 0, 0]) rotate([90, 0, 90]) linear_extrude(len + 2 * eps)
      polygon([[-groove_w / 2 - 0.4, -eps], [groove_w / 2 + 0.4, -eps], [groove_w / 2, 0.4], [-groove_w / 2, 0.4]]);
    // lead-in at both ends of the groove
    groove_mouth();
    translate([len, 0, 0]) mirror([1, 0, 0]) groove_mouth();
    // free the spring finger: a slot behind it and a gap above it
    translate([finger_root, groove_w / 2 + finger_t, -eps])
      cube([len, finger_gap, groove_d + finger_gap + eps]);
    translate([finger_root, groove_w / 2 - eps, groove_d - eps])
      cube([len, finger_t + finger_gap + eps, finger_gap + eps]);
  }
  // bump near the finger's free end that presses on the ridge
  translate([len - 5, groove_w / 2, 0.3])
    linear_extrude(ridge_h - ridge_ch - 0.5)
      polygon([[-3, 0.2], [0, -(clearance + press)], [3, 0.2]]);
  // pointer over the scale
  translate([0, -base_w / 2, 0])
    linear_extrude(pointer_t)
      polygon([[max(0, mx - pointer_len), 0.5], [mx, -pointer_len], [min(len, mx + pointer_len), 0.5]]);
}

module groove_mouth() {
  hull() {
    translate([-eps, -groove_w / 2 - 0.8, -eps]) cube([eps, groove_w + 1.6, groove_d + 0.8]);
    translate([0.8, -groove_w / 2, -eps]) cube([eps, groove_w, groove_d]);
  }
}

// ---------------------------------------------------------- screen holder
//
// The board sits face out on four corner posts (room for its connectors
// behind it). Corner brackets keep it in place and leave the edges open for
// the cable.

module oled_holder() {
  cz = axis_h - oled_center_offset;  // board center height
  bw = oled_w + 2 * clearance;
  bh = oled_h + 2 * clearance;
  pw = bw + 2 * oled_wall;
  ztop = cz + bh / 2 + oled_wall;
  pcb_back = oled_plate_t + oled_standoff;
  pcb_front = pcb_back + oled_pcb_t;
  wall_x = pcb_front + 1;
  mx = pcb_front + 0.8;  // about where the image forms, inside the glass
  len = 20;

  carriage_base(len, mx);
  difference() {
    union() {
      // back plate
      translate([0, -pw / 2, base_t - eps]) cube([oled_plate_t, pw, ztop - base_t + eps]);
      // solid block that ties the plate to the base
      translate([0, -base_w / 2, base_t - eps])
        cube([wall_x, base_w, cz - bh / 2 - base_t + eps]);
      for (sy = [-1, 1], sz = [-1, 1]) {
        // corner posts
        translate([oled_plate_t - eps, sy * (bw / 2 - 2.5) - 2.5, cz + sz * (bh / 2 - 2.5) - 2.5])
          cube([oled_standoff + eps, 5, 5]);
        // corner brackets
        translate([0, sy * (bw / 2 + oled_wall / 2), cz + sz * (bh / 2 - oled_bracket / 2 + oled_wall / 2)])
          box_x(oled_plate_t - eps, wall_x, oled_wall, oled_bracket + oled_wall);
        translate([0, sy * (bw / 2 - oled_bracket / 2 + oled_wall / 2), cz + sz * (bh / 2 + oled_wall / 2)])
          box_x(oled_plate_t - eps, wall_x, oled_bracket + oled_wall, oled_wall);
        // snap lip over the board edge (the glass is narrower than the board here)
        translate([0, sy * (bw / 2 - oled_lip / 2 + eps), cz + sz * (bh / 2 - oled_bracket / 2)])
          box_x(pcb_front + 0.15, wall_x, oled_lip, oled_bracket);
      }
    }
    // window behind the board, plus slots for header pins at the top and bottom
    translate([-eps, -(bw / 2 - 6), cz - (bh / 2 - 6)]) cube([oled_plate_t + 2 * eps, bw - 12, bh - 12]);
    for (sz = [-1, 1])
      translate([-eps, -8, cz + sz * (bh / 2 - 3.5) - 3]) cube([oled_plate_t + 2 * eps, 16, 6]);
  }
}

// A box spanning x0..x1, centered on the current y/z.
module box_x(x0, x1, dy, dz) {
  translate([x0, -dy / 2, -dz / 2]) cube([x1 - x0, dy, dz]);
}

// ------------------------------------------------------------ lens holder
//
// The lens drops into a pocket from the open side and rests on a ring.
// Three thin ribs get squeezed by the lens and hold it.

module lens_holder() {
  r = lens_d / 2 + clearance;
  t = lens_lip + lens_depth;
  pw = lens_d + 8;
  ztop = axis_h + lens_d / 2 + 4;
  mx = lens_lip + 1.5;  // about the middle of a typical lens
  len = 20;

  carriage_base(len, mx);
  difference() {
    union() {
      translate([0, -pw / 2, base_t - eps]) cube([t, pw, ztop - base_t + eps]);
      for (sy = [-1, 1]) translate([0, sy * (base_w / 2 - 1.2), 0]) gusset(t, 10, 2.4);
    }
    translate([-eps, 0, axis_h]) rotate([0, 90, 0]) cylinder(r = lens_d / 2 - 1.5, h = t + 2 * eps);
    translate([lens_lip, 0, axis_h]) rotate([0, 90, 0]) cylinder(r = r, h = t);
  }
  for (a = [90, 210, 330])
    translate([0, 0, axis_h]) rotate([a, 0, 0])
      hull() {
        translate([lens_lip - eps, r - lens_rib, -0.5]) cube([t - lens_lip - 1, lens_rib + 0.3, 1]);
        translate([t - eps, r - 0.05, -0.5]) cube([eps, 0.35, 1]);
      }
}

// Triangular brace from a holder's front face down to the base.
module gusset(x0, size, thick) {
  rotate([90, 0, 0]) linear_extrude(thick, center = true)
    polygon([[x0 - eps, base_t - eps], [x0 + size, base_t - eps], [x0 - eps, base_t + size]]);
}

// ------------------------------------------------ beam splitter cube holder
//
// The cube sits on a small table with four corner posts. Light from the lens
// enters the side facing the lens and bounces out toward the front of the
// rail, where your eye is. The wall behind the cube soaks up the light that
// passes straight through (print it in black if you can).

module cube_holder() {
  c = cube_size + 2 * clearance;
  mx = 3 + c / 2;
  len = max(20, 3 + c + 2.4);
  top = axis_h - cube_size / 2;  // table surface
  w = c + 2 * 1.6;

  carriage_base(len, mx);
  // table
  translate([mx - c / 2 - 1.6, -w / 2, base_t - eps]) cube([c + 1.6, w, top - base_t + eps]);
  // corner posts
  for (sx = [-1, 1], sy = [-1, 1])
    translate([mx + sx * (c / 2 + 0.8) - 0.8, sy * (c / 2 + 0.8) - 0.8, top - eps]) cube([1.6, 1.6, 2.5]);
  // back wall
  translate([mx + c / 2, -w / 2, base_t - eps]) cube([2.4, w, top + cube_size + 1 - base_t]);
}

// -------------------------------------------- flat glass splitter holder
//
// For a microscope slide or a piece of clear plastic, before the cube
// arrives. The plate stands at 45 degrees in a slot.

module plate_holder() {
  len = 22;
  mx = 11;
  slot_w = plate_t + 2 * clearance;
  slot_bottom = axis_h - plate_h / 2;
  top = slot_bottom + 4;

  carriage_base(len, mx);
  difference() {
    translate([mx - 9, -base_w / 2, base_t - eps]) cube([18, base_w, top - base_t + eps]);
    // tilted so light coming down the rail bounces toward the front
    translate([mx, 0, slot_bottom]) rotate([0, 0, -45])
      translate([-20, -slot_w / 2, 0]) cube([40, slot_w, 10]);
  }
}

// --------------------------------------------------------------- fit test
//
// A short piece of rail and a bare holder base. A holder should slide with
// gentle pressure and stay put when you let go.

module fit_test() {
  rail(40, false);
  translate([10, rail_w + 8 + base_w / 2, 0]) carriage_base(20, 10);
}

// --------------------------------------------------------------- assembly

module at_reading(reading, mx) {
  translate([scale_x0 + reading - mx, ridge_y, rail_h]) children();
}

module assembly() {
  color("SlateGray") rail();

  // screen holder and screen
  oled_mx = oled_plate_t + oled_standoff + oled_pcb_t + 0.8;
  at_reading(demo_oled_at, oled_mx) {
    color("Orange") oled_holder();
    pcb_back = oled_plate_t + oled_standoff;
    translate([pcb_back, -oled_w / 2, axis_h - oled_center_offset - oled_h / 2]) {
      color("DarkGreen") cube([oled_pcb_t, oled_w, oled_h]);
      color("Black") translate([oled_pcb_t, (oled_w - 26.6) / 2, (oled_h - 19) / 2]) cube([1.4, 26.6, 19]);
    }
  }

  // lens holder and lens
  lens_mx = lens_lip + 1.5;
  at_reading(demo_lens_at, lens_mx) {
    color("Orange") lens_holder();
    color([0.6, 0.85, 1, 0.6]) translate([lens_mx, 0, axis_h]) rotate([0, 90, 0])
      resize([lens_d, lens_d, 3]) sphere(d = lens_d);
  }

  // cube holder and cube
  cube_mx = 3 + (cube_size + 2 * clearance) / 2;
  at_reading(demo_cube_at, cube_mx) {
    color("Orange") cube_holder();
    translate([cube_mx, 0, axis_h]) {
      color([0.5, 0.9, 1, 0.35]) cube(cube_size, center = true);
      // the mirror plane inside the cube: turns light going down the rail toward the front
      color([0.3, 0.6, 0.8, 0.6]) rotate([0, 0, -45]) cube([cube_size * 1.41, 0.3, cube_size], center = true);
    }
  }

  // light path: screen -> lens -> splitter -> eye (in front of the rail)
  ray_x0 = scale_x0 + demo_oled_at;
  ray_x1 = scale_x0 + demo_cube_at;
  color("Gold") {
    translate([ray_x0, ridge_y, rail_h + axis_h]) rotate([0, 90, 0]) cylinder(d = 0.8, h = ray_x1 - ray_x0, $fn = 12);
    translate([ray_x1, ridge_y, rail_h + axis_h]) rotate([90, 0, 0]) cylinder(d = 0.8, h = ridge_y + 25, $fn = 12);
    translate([ray_x1, -25, rail_h + axis_h]) rotate([90, 0, 0]) cylinder(d1 = 3, d2 = 0, h = 4, $fn = 12);
  }
}
