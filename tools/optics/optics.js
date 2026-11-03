// Optics model for the DisplayGlasses heads-up display.
// A line-for-line port of optics.py, so the web calculator and the Python
// tool give the same numbers. Works in a browser (window.Optics) and in Node
// (module.exports). All lengths in millimeters, angles in degrees.
(function (root) {
  "use strict";

  const SCREEN_W = 21.744; // 0.96" SSD1306 active area, 128 x 64 px at 0.17 mm
  const SCREEN_H = 10.864;
  const PIXEL = 0.17;
  const CAP_HEIGHT_PX = 7;
  const GLASS_N = 1.5;
  const GLASS_DENSITY = 2.5;
  const LENS_RIM = 1.0;
  const SQRT2 = Math.SQRT2;

  // Eyelashes: keep the splitter's nearest edge about 16 mm from the pupil.
  // A 25 mm plate centered 25 mm out leaves 16.2 mm; under 16 mm is a warning.
  const NEAR_EDGE_TARGET = 16.2;
  const NEAR_EDGE_MIN = 16.0;

  // Screen points checked for the eye box (fractions of the half width and
  // half height): corners first, then edges, then the middle.
  const SCREEN_POINTS = [[-1, -1], [1, -1], [-1, 1], [1, 1],
    [-0.5, -1], [0.5, -1], [-0.5, 1], [0.5, 1],
    [-1, 0], [1, 0], [-0.5, 0], [0.5, 0],
    [0, -1], [0, 1], [0, 0]];

  // Where rays leave the pupil (fractions of its radius): a ring of 12 at the
  // edge, a ring of 12 halfway in, and the center.
  const PUPIL_POINTS = [];
  [1.0, 0.5].forEach((k) => {
    for (let n = 0; n < 12; n++) PUPIL_POINTS.push([k * Math.cos((n * Math.PI) / 6), k * Math.sin((n * Math.PI) / 6)]);
  });
  PUPIL_POINTS.push([0, 0]);

  // A denser spread over the pupil for shading the simulated view.
  const SHADE_POINTS = [];
  for (let i = -4; i <= 4; i++) {
    for (let j = -4; j <= 4; j++) if (i * i + j * j <= 16) SHADE_POINTS.push([i / 4, j / 4]);
  }

  function makeSetup(p) {
    const s = Object.assign({
      f: 60, lensD: 30, splitter: "plate", size: 25, plateShape: "round", plateH: 25, plateT: 1,
      eyeToSplitter: 25, lensToSplitter: null, imageDist: 2000, pupil: 4,
    }, p || {});
    if (s.splitter === "plate" && s.plateShape === "round") s.plateH = s.size;
    if (s.lensToSplitter == null) s.lensToSplitter = splitterDepth(s) / 2 + 4;
    return s;
  }

  function splitterDepth(s) {
    return s.splitter === "cube" ? s.size : s.size / SQRT2;
  }

  // How much closer to the eye the splitter's nearest point is than its center.
  function nearOffset(s) {
    return s.splitter === "cube" ? s.size / 2 : s.size / (2 * SQRT2);
  }

  // Pupil to splitter center that keeps the near edge clear of eyelashes.
  function safeEyeDistance(splitter, size) {
    return Math.round((NEAR_EDGE_TARGET + nearOffset({ splitter, size })) * 10) / 10;
  }

  // Ray slopes inside glass for a ray with slopes (sy, sv) in air (Snell's law).
  function glassSlopes(sy, sv) {
    const k = 1 / (GLASS_N * Math.sqrt(1 + sy * sy + sv * sv));
    const ty = sy * k, tv = sv * k;
    const tz = Math.sqrt(1 - ty * ty - tv * tv);
    return [ty / tz, tv / tz];
  }

  // Traces one ray backward from the eye along the unfolded path. y is
  // sideways (the way the splitter tilts), v is up/down; the ray leaves the
  // pupil at (y0, v0) with slopes sy and sv.
  function rayVisible(s, y0, v0, sy, sv, lensZ) {
    const e = s.eyeToSplitter;
    let yLens, vLens;
    if (s.splitter === "cube") {
      const c = s.size;
      const g = e - c / 2;
      let y = y0 + g * sy, v = v0 + g * sv;
      if (Math.abs(y) > c / 2 || Math.abs(v) > c / 2) return false;
      const [gy, gv] = glassSlopes(sy, sv);
      y += c * gy; v += c * gv;
      if (Math.abs(y) > c / 2 || Math.abs(v) > c / 2) return false;
      const rest = lensZ - (g + c);
      yLens = y + rest * sy; vLens = v + rest * sv;
    } else {
      if (Math.abs(1 - sy) < 1e-9) return false;
      const zHit = (y0 + e) / (1 - sy);
      const u = (zHit - e) * SQRT2;
      const v = v0 + zHit * sv;
      const a = s.size / 2, b = s.plateH / 2;
      if (s.plateShape === "round") {
        if ((u / a) ** 2 + (v / b) ** 2 > 1) return false;
      } else if (Math.abs(u) > a || Math.abs(v) > b) {
        return false;
      }
      yLens = y0 + lensZ * sy; vLens = v0 + lensZ * sv;
    }
    const r = s.lensD / 2 - LENS_RIM;
    return yLens * yLens + vLens * vLens <= r * r;
  }

  // Share of the pupil that sees a screen point (0 = hidden, 1 = full brightness).
  function brightness(s, y0, v0, sy, sv, lensZ) {
    const half = s.pupil / 2;
    let seen = 0;
    for (const [p, q] of SHADE_POINTS) if (rayVisible(s, y0 + p * half, v0 + q * half, sy, sv, lensZ)) seen++;
    return seen / SHADE_POINTS.length;
  }

  // Width of the range of eye positions that see the whole screen, corners
  // included. fullPupil: no dimming anywhere; otherwise only the pupil's
  // center ray must get through (an edge at worst about half as bright).
  function eyebox(s, slopeH, slopeV, horizontal, lensZ, fullPupil) {
    const half = s.pupil / 2;
    const pupil = fullPupil ? PUPIL_POINTS.map(([p, q]) => [p * half, q * half]) : [[0, 0]];
    const rays = [];
    for (const [i, j] of SCREEN_POINTS) for (const [py, pv] of pupil) rays.push([i * slopeH, j * slopeV, py, pv]);
    const step = 0.05;
    let lo = null;
    let hi = null;
    // Integer steps (not y += step) so the positions match optics.py exactly.
    for (let i = 0; i <= 800; i++) {
      const d = -20 + i * step;
      const y0 = horizontal ? d : 0, v0 = horizontal ? 0 : d;
      if (rays.every(([sy, sv, py, pv]) => rayVisible(s, y0 + py, v0 + pv, sy, sv, lensZ))) {
        if (lo === null) lo = d;
        hi = d;
      }
    }
    return lo === null ? 0 : hi - lo + step;
  }

  // Fraction of the screen's width (or height) the centered eye sees; a
  // column counts when its top, middle and bottom all show.
  function visibleFraction(s, slopeH, slopeV, horizontal, lensZ) {
    const n = 101;
    let seen = 0;
    for (let i = 0; i < n; i++) {
      const k = (2 * i) / (n - 1) - 1;
      const ok = [-1, 0, 1].every((j) => (horizontal
        ? rayVisible(s, 0, 0, k * slopeH, j * slopeV, lensZ)
        : rayVisible(s, 0, 0, j * slopeH, k * slopeV, lensZ)));
      if (ok) seen++;
    }
    return seen / n;
  }

  function screenDistance(f, imageFromLens) {
    return Number.isFinite(imageFromLens) ? (f * imageFromLens) / (f + imageFromLens) : f;
  }

  function analyze(p) {
    const s = makeSetup(p);
    const lensToEye = s.splitter === "cube"
      ? s.eyeToSplitter + s.lensToSplitter - s.size * (1 - 1 / GLASS_N)
      : s.eyeToSplitter + s.lensToSplitter;
    const lensZ = s.eyeToSplitter + s.lensToSplitter;
    const distFor = (fromEye) => screenDistance(s.f, fromEye - lensToEye);

    const sd = distFor(s.imageDist);
    const v = (sd * s.f) / (s.f - sd);
    const mag = v / sd;
    const slopeH = ((SCREEN_W / 2) * mag) / (v + lensToEye);
    const slopeV = ((SCREEN_H / 2) * mag) / (v + lensToEye);
    let grams;
    if (s.splitter === "cube") {
      grams = Math.pow(s.size / 10, 3) * GLASS_DENSITY;
    } else {
      const area = s.plateShape === "round" ? (Math.PI / 4) * s.size * s.size : s.size * s.plateH;
      grams = (area / 100) * (s.plateT / 10) * GLASS_DENSITY;
    }

    return {
      setup: s,
      lensToEye,
      lensZ,
      screenToLens: sd,
      focusRange: [distFor(500), s.f],
      slopeH,
      slopeV,
      fovH: 2 * (Math.atan(slopeH) * 180) / Math.PI,
      fovV: 2 * (Math.atan(slopeV) * 180) / Math.PI,
      textMmAt1m: ((CAP_HEIGHT_PX * PIXEL * mag) / (v + lensToEye)) * 1000,
      eyeboxH: eyebox(s, slopeH, slopeV, true, lensZ, true),
      eyeboxV: eyebox(s, slopeH, slopeV, false, lensZ, true),
      eyeboxHalfH: eyebox(s, slopeH, slopeV, true, lensZ, false),
      eyeboxHalfV: eyebox(s, slopeH, slopeV, false, lensZ, false),
      visibleH: visibleFraction(s, slopeH, slopeV, true, lensZ),
      visibleV: visibleFraction(s, slopeH, slopeV, false, lensZ),
      nearEdge: s.eyeToSplitter - nearOffset(s),
      pathLength: sd + s.lensToSplitter,
      splitterGrams: grams,
    };
  }

  const Optics = {
    SCREEN_W, SCREEN_H, PIXEL, NEAR_EDGE_TARGET, NEAR_EDGE_MIN,
    makeSetup, splitterDepth, nearOffset, safeEyeDistance, analyze, brightness, rayVisible,
  };
  if (typeof module !== "undefined" && module.exports) module.exports = Optics;
  else root.Optics = Optics;
})(typeof window !== "undefined" ? window : globalThis);
