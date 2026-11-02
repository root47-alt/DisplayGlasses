#pragma once
// Text helpers for the HUD. Pure C++ (no Arduino), so they can be unit-tested
// on a computer.

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace text {

// Makes notification text drawable by the OLED font. Keeps ASCII and Latin-1
// (accents such as á, ñ, ü), turns smart quotes, dashes and ellipses into
// plain ASCII, collapses runs of spaces and blank lines, and drops anything
// the font can't draw (emoji, other scripts). Input and output are UTF-8.
std::string sanitize(const std::string& utf8);

// Width of a string in pixels, as the display font would draw it.
using MeasureFn = std::function<int(const std::string&)>;

// Word-wraps text into lines no wider than maxWidth pixels. '\n' starts a new
// line, and a word wider than a whole line is split across lines.
std::vector<std::string> wrap(const std::string& utf8, int maxWidth, const MeasureFn& measure);

// Number of Unicode code points in a UTF-8 string.
size_t length(const std::string& utf8);

}  // namespace text
