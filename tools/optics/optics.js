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

  function makeSetup(p) {
    const s = Object.assign({
      f: 50, lensD: 25, splitter: "plate", size: 25, plateH: 20, plateT: 1,
      eyeToSplitter: 20, lensToSplitter: null, imageDist: 2000, pupil: 4,
    }, p || {});
    if (s.lensToSplitter == null) s.lensToSplitter = splitterDepth(s) / 2 + 4;
    return s;
  }

  function splitterDepth(s) {
    return s.splitter === "cube" ? s.size : s.size / Math.SQRT2;
  }

  function glassSlope(slope) {
    const a = Math.atan(slope);
    return Math.tan(Math.asin(Math.sin(a) / GLASS_N));
  }

  // Traces one ray backward from the eye (offset y0) along the unfolded path.
  function rayVisible(s, y0, slope, horizontal, lensZ) {
    const rLens = s.lensD / 2 - LENS_RIM;
    const e = s.eyeToSplitter;
    let yLens;
    if (s.splitter === "cube") {
      const c = s.size;
      const g = e - c / 2;
      const yNear = y0 + g * slope;
      const yFar = yNear + c * glassSlope(slope);
      if (Math.abs(yNear) > c / 2 || Math.abs(yFar) > c / 2) return false;
      yLens = yFar + (lensZ - (g + c)) * slope;
    } else {
      if (horizontal) {
        if (Math.abs(1 - slope) < 1e-9) return false;
        const zHit = (y0 + e) / (1 - slope);
        const yHit = zHit - e;
        if (Math.abs(yHit) > s.size / (2 * Math.SQRT2)) return false;
      } else if (Math.abs(y0 + e * slope) > s.plateH / 2) {
        return false;
      }
      yLens = y0 + lensZ * slope;
    }
    return Math.abs(yLens) <= rLens;
  }

  const PUPIL_SAMPLES = [-1, -0.5, 0, 0.5, 1];

  function fieldVisible(s, offset, slope, horizontal, lensZ, fullPupil) {
    if (!fullPupil) return rayVisible(s, offset, slope, horizontal, lensZ);
    const half = s.pupil / 2;
    return PUPIL_SAMPLES.every((u) => rayVisible(s, offset + u * half, slope, horizontal, lensZ));
  }

  // Share of the pupil that sees a screen point (0 = hidden, 1 = full brightness).
  function brightness(s, offset, slope, horizontal, lensZ) {
    const n = 11;
    let seen = 0;
    for (let i = 0; i < n; i++) {
      const u = (2 * i) / (n - 1) - 1;
      if (rayVisible(s, offset + (u * s.pupil) / 2, slope, horizontal, lensZ)) seen++;
    }
    return seen / n;
  }

  function eyebox(s, maxSlope, horizontal, lensZ) {
    const step = 0.05;
    let lo = null;
    let hi = null;
    // Integer steps (not y += step) so the positions match optics.py exactly.
    for (let i = 0; i <= 800; i++) {
      const y = -20 + i * step;
      const ok = PUPIL_SAMPLES.every((k) => fieldVisible(s, y, k * maxSlope, horizontal, lensZ, true));
      if (ok) {
        if (lo === null) lo = y;
        hi = y;
      }
    }
    return lo === null ? 0 : hi - lo + step;
  }

  function visibleFraction(s, maxSlope, horizontal, lensZ) {
    const n = 101;
    let seen = 0;
    for (let i = 0; i < n; i++) {
      if (fieldVisible(s, 0, ((2 * i) / (n - 1) - 1) * maxSlope, horizontal, lensZ, false)) seen++;
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
    const grams = s.splitter === "cube"
      ? Math.pow(s.size / 10, 3) * GLASS_DENSITY
      : (s.size / 10) * (s.plateH / 10) * (s.plateT / 10) * GLASS_DENSITY;

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
      eyeboxH: eyebox(s, slopeH, true, lensZ),
      eyeboxV: eyebox(s, slopeV, false, lensZ),
      visibleH: visibleFraction(s, slopeH, true, lensZ),
      visibleV: visibleFraction(s, slopeV, false, lensZ),
      pathLength: sd + s.lensToSplitter,
      splitterGrams: grams,
    };
  }

  const Optics = { SCREEN_W, SCREEN_H, PIXEL, makeSetup, splitterDepth, analyze, brightness, rayVisible };
  if (typeof module !== "undefined" && module.exports) module.exports = Optics;
  else root.Optics = Optics;
})(typeof window !== "undefined" ? window : globalThis);
