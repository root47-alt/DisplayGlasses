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
// Screen board width. Adafruit 0.96 inch OLED with STEMMA QT (#326): 29.21
oled_w = 29.21;
// Screen board height, counting the two mounting ears at the bottom: 31.75
oled_h = 31.75;
// How far each mounting hole sits in from the board's side and top or bottom edge: 2.54
oled_hole_inset = 2.54;
// Middle of the lit-up area, measured up from the bottom of the ears: 19.4
oled_lit_v = 19.4;
// Circuit board thickness
oled_pcb_t = 1.6;
// Tallest part on the back of the screen board (its connectors)
oled_back_t = 3.0;
// Lens diameter
lens_d = 25;
// Lens thickness at the middle (Edmund #45-241, 30 mm f60: 6.0)
lens_t = 6.0;
// Beam splitter cube side length
cube_size = 15;
// Thickness of a flat glass or plastic splitter (Knight BAE2511: 1.1; microscope slide: about 1)
plate_t = 1.1;
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
finger_top = groove_d - finger_gap;  // the finger stops short of the groove's ceiling
pointer_len = 2;    // how far the pointer reaches over the scale
pointer_w = 1.5;    // half its width, so the ticks beside it stay visible
pointer_t = 1.2;

// Screen holder
oled_plate_t = 3;
oled_standoff = oled_back_t + 0.6;
oled_post_d = 4;     // posts behind the mounting holes (the board keeps parts 2 mm away from them)
oled_pin_d = [2.4, 2.0];  // pins through the 2.5 mm holes, tapering toward the tip

// Lens holder
lens_len = 20;     // base length
lens_lip = 1.2;    // ring the lens rests on
lens_ledge = 1.0;  // how far that ring reaches in over the lens
lens_depth = 4;    // depth of the lens pocket
lens_rib = 0.45;   // crush ribs: squeeze the lens so it can't fall out

// Flat splitter holder
plate_clear = 0.2;  // gap on each side of the plate in its slot
plate_slot = 7;     // slot depth: deep enough that the plate stands up straight

// Example layout for the assembly preview (scale readings)
demo_oled_at = 8;      // the default lens's focus: 58.2 mm to the screen...
demo_lens_at = 66.2;
demo_plate_at = 79;    // ...and the splitter 12.8 mm past it, as in the glasses

// ---------------------------------------------------------------- choose part

if (part == "assembly") assembly();
else if (part == "rail") rail();
else if (part == "oled") print_on_back() oled_holder();
else if (part == "lens") print_on_front(lens_len) lens_holder();
else if (part == "cube") cube_holder();
else if (part == "plate") plate_holder();
else if (part == "fit_test") fit_test();

// The screen holder prints lying on its back face, the lens holder on its
// front face (no supports).
module print_on_back() { rotate([0, -90, 0]) children(); }
module print_on_front(len) { translate([0, 0, len]) rotate([0, 90, 0]) children(); }

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
    // free the spring finger: a slot behind it, and room above it under the
    // groove's ceiling, so the ceiling prints as one bridge between solid walls
    translate([finger_root, groove_w / 2 + finger_t, -eps])
      cube([len, finger_gap, groove_d + eps]);
    translate([finger_root, groove_w / 2 - eps, finger_top])
      cube([len, finger_t + finger_gap + eps, groove_d - finger_top + eps]);
  }
  // bump near the finger's free end that presses on the ridge. The holder
  // shifts until the ridge touches the far wall, so the bump reaches past the
  // ridge's edge by `press`.
  translate([len - 5, groove_w / 2, 0.3])
    linear_extrude(ridge_h - ridge_ch - 0.5)
      polygon([[-3, 0.2], [0, -(2 * clearance + press)], [3, 0.2]]);
  // pointer over the scale
  translate([0, -base_w / 2, 0])
    linear_extrude(pointer_t)
      polygon([[max(0, mx - pointer_w), 0.5], [mx, -pointer_len], [min(len, mx + pointer_w), 0.5]]);
}

module groove_mouth() {
  hull() {
    translate([-eps, -groove_w / 2 - 0.8, -eps]) cube([eps, groove_w + 1.6, groove_d + 0.8]);
    translate([0.8, -groove_w / 2, -eps]) cube([eps, groove_w, groove_d]);
  }
}

// ---------------------------------------------------------- screen holder
//
// The board hangs on four pins through its mounting holes, screen facing out
// and mounting ears at the bottom. The pins taper, so it wedges on. Posts
// behind the holes keep it clear of the parts on its back, and nothing covers
// its edges: the STEMMA QT cable plugs in at either side, and the display's
// ribbon wraps around the bottom edge between the ears.

module oled_holder() {
  z0 = axis_h - oled_lit_v;  // bottom of the board's ears: puts the lit area on the axis
  pcb_back = oled_plate_t + oled_standoff;
  pcb_front = pcb_back + oled_pcb_t;
  mx = pcb_front + 0.8;  // about where the image forms, inside the glass
  len = 20;
  pw = oled_w + 4;
  ztop = z0 + oled_h + 1;
  holes = [for (u = [oled_hole_inset, oled_w - oled_hole_inset], v = [oled_hole_inset, oled_h - oled_hole_inset])
             [u - oled_w / 2, z0 + v]];

  carriage_base(len, mx);
  difference() {
    union() {
      // back plate
      translate([0, -pw / 2, base_t - eps]) cube([oled_plate_t, pw, ztop - base_t + eps]);
      // small braces from the plate down to the base, under the gap between the ears
      for (sy = [-1, 1]) translate([0, sy * (base_w / 2 - 1.2), 0])
        gusset(oled_plate_t, min(3, oled_standoff - 0.4), 2.4);
    }
    // window behind the middle of the board, for the parts on its back
    translate([-eps, -(oled_w / 2 - 5), z0 + 6]) cube([oled_plate_t + 2 * eps, oled_w - 10, oled_h - 11]);
  }
  for (h = holes) {
    // post the board's back rests on
    translate([oled_plate_t - eps, h[0], h[1]]) rotate([0, 90, 0]) cylinder(d = oled_post_d, h = oled_standoff + eps);
    // pin through the mounting hole
    translate([pcb_back - eps, h[0], h[1]]) rotate([0, 90, 0])
      cylinder(d1 = oled_pin_d[0], d2 = oled_pin_d[1], h = oled_pcb_t + 1.2 + eps, $fn = 24);
  }
}
// A box spanning x0..x1, centered on the current y/z.
module box_x(x0, x1, dy, dz) {
  translate([x0, -dy / 2, -dz / 2]) cube([x1 - x0, dy, dz]);
}

// ------------------------------------------------------------ lens holder
//
// The lens sits at the splitter end of its base, so the splitter holder can
// come as close as it does in the glasses. It drops into a pocket that opens
// toward the splitter, flat side first, and rests on a ring. Three thin ribs
// get squeezed by the lens and hold it.

module lens_holder() {
  r = lens_d / 2 + clearance;
  t = lens_lip + lens_depth;
  pw = lens_d + 8;
  ztop = axis_h + lens_d / 2 + 4;
  len = lens_len;
  x0 = len - t;                         // the lens plate's screen side
  mx = x0 + lens_lip + lens_t / 2;      // the middle of the lens

  carriage_base(len, mx);
  difference() {
    union() {
      translate([x0, -pw / 2, base_t - eps]) cube([t, pw, ztop - base_t + eps]);
      // braces toward the screen
      for (sy = [-1, 1]) translate([0, sy * (base_w / 2 - 1.2), 0]) gusset(x0, -10, 2.4);
    }
    // the ring the lens rests on, and the pocket in front of it
    translate([x0 - eps, 0, axis_h]) rotate([0, 90, 0]) cylinder(r = r - lens_ledge, h = t + 2 * eps);
    translate([x0 + lens_lip, 0, axis_h]) rotate([0, 90, 0]) cylinder(r = r, h = t);
  }
  for (a = [90, 210, 330])
    translate([0, 0, axis_h]) rotate([a, 0, 0])
      hull() {
        translate([x0 + lens_lip - eps, r - lens_rib, -0.5]) cube([t - lens_lip - 1, lens_rib + 0.3, 1]);
        translate([len - eps, r - 0.05, -0.5]) cube([eps, 0.35, 1]);
      }
}

// Triangular brace from a holder's plate down to the base. A negative size
// points it toward -x.
module gusset(x0, size, thick) {
  rotate([90, 0, 0]) linear_extrude(thick, center = true)
    polygon([[x0, base_t - eps], [x0 + size, base_t - eps], [x0, base_t + abs(size)]]);
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
  slot_w = plate_t + 2 * plate_clear;
  slot_bottom = axis_h - plate_h / 2;
  top = slot_bottom + plate_slot;

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
  // A short, narrow piece of rail: just the ridge and the strip a holder
  // rides on, so it prints quickly.
  intersection() {
    translate([0, 0, -(rail_h - 3)]) rail(30, false);
    translate([-1, ridge_y - 10, 0]) cube([32, 20, rail_h + ridge_h]);
  }
  translate([5, ridge_y + 10 + 6 + base_w / 2, 0]) carriage_base(20, 10);
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
    translate([pcb_back, -oled_w / 2, axis_h - oled_lit_v]) {
      color("DarkGreen") difference() {
        cube([oled_pcb_t, oled_w, oled_h]);
        // the gap between the mounting ears, where the display's ribbon wraps around
        translate([-eps, 5.08, -eps]) cube([oled_pcb_t + 2 * eps, oled_w - 10.16, 5.08]);
      }
      // display glass (the lit area is 2.1 mm below its top edge)
      color("Black") translate([oled_pcb_t, (oled_w - 26.7) / 2, 7.67]) cube([1.4, 26.7, 19.26]);
    }
  }

  // lens holder and lens
  lens_mx = lens_len - lens_lip - lens_depth + lens_lip + lens_t / 2;
  at_reading(demo_lens_at, lens_mx) {
    color("Orange") lens_holder();
    color([0.6, 0.85, 1, 0.6]) translate([lens_mx, 0, axis_h]) rotate([0, 90, 0])
      resize([lens_d, lens_d, lens_t]) sphere(d = lens_d);
  }

  // flat splitter holder and a 25 mm round plate: turns light going down the
  // rail toward the front
  plate_mx = 11;
  at_reading(demo_plate_at, plate_mx) {
    color("Orange") plate_holder();
    color([0.5, 0.85, 1, 0.5]) translate([plate_mx, 0, axis_h]) rotate([0, 0, -45]) rotate([90, 0, 0])
      cylinder(d = plate_h, h = plate_t, center = true);
  }

  // light path: screen -> lens -> splitter -> eye (in front of the rail)
  ray_x0 = scale_x0 + demo_oled_at;
  ray_x1 = scale_x0 + demo_plate_at;
  color("Gold") {
    translate([ray_x0, ridge_y, rail_h + axis_h]) rotate([0, 90, 0]) cylinder(d = 0.8, h = ray_x1 - ray_x0, $fn = 12);
    translate([ray_x1, ridge_y, rail_h + axis_h]) rotate([90, 0, 0]) cylinder(d = 0.8, h = ridge_y + 25, $fn = 12);
    translate([ray_x1, -25, rail_h + axis_h]) rotate([90, 0, 0]) cylinder(d1 = 3, d2 = 0, h = 4, $fn = 12);
  }
}
