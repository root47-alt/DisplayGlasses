#include "text.h"

#include <cstdint>

namespace text {
namespace {

// Decodes the code point starting at s[i] and advances i past it. Returns
// false (still advancing) for malformed bytes, which callers skip.
bool decodeNext(const std::string& s, size_t& i, uint32_t& cp) {
  const unsigned char lead = static_cast<unsigned char>(s[i]);
  if (lead < 0x80) {
    cp = lead;
    i += 1;
    return true;
  }
  size_t len;
  uint32_t min;
  if ((lead & 0xE0) == 0xC0) {
    len = 2;
    cp = lead & 0x1F;
    min = 0x80;
  } else if ((lead & 0xF0) == 0xE0) {
    len = 3;
    cp = lead & 0x0F;
    min = 0x800;
  } else if ((lead & 0xF8) == 0xF0) {
    len = 4;
    cp = lead & 0x07;
    min = 0x10000;
  } else {
    i += 1;  // stray continuation byte or invalid lead byte
    return false;
  }
  if (i + len > s.size()) {
    i = s.size();  // truncated sequence at the end
    return false;
  }
  for (size_t k = 1; k < len; ++k) {
    const unsigned char c = static_cast<unsigned char>(s[i + k]);
    if ((c & 0xC0) != 0x80) {
      i += 1;  // resync on the next byte
      return false;
    }
    cp = (cp << 6) | (c & 0x3F);
  }
  i += len;
  return cp >= min && cp <= 0x10FFFF && !(cp >= 0xD800 && cp <= 0xDFFF);
}

void appendUtf8(std::string& out, uint32_t cp) {
  if (cp < 0x80) {
    out += static_cast<char>(cp);
  } else if (cp < 0x800) {
    out += static_cast<char>(0xC0 | (cp >> 6));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    out += static_cast<char>(0xE0 | (cp >> 12));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else {
    out += static_cast<char>(0xF0 | (cp >> 18));
    out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  }
}

bool isSpace(uint32_t cp) {
  return cp == ' ' || cp == '\t' || cp == 0xA0 || (cp >= 0x2000 && cp <= 0x200A) || cp == 0x202F ||
         cp == 0x205F || cp == 0x3000;
}

bool isLineBreak(uint32_t cp) { return cp == '\n' || cp == 0x2028 || cp == 0x2029; }

// ASCII stand-ins for common typographic characters outside Latin-1.
const char* replacement(uint32_t cp) {
  switch (cp) {
    case 0x2018: case 0x2019: case 0x201A: case 0x201B: case 0x2032:
      return "'";
    case 0x201C: case 0x201D: case 0x201E: case 0x201F: case 0x2033:
      return "\"";
    case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014: case 0x2015: case 0x2212:
      return "-";
    case 0x2026:
      return "...";
    case 0x2022: case 0x2023: case 0x2043:
      return "\xC2\xB7";  // middle dot, which the font has
    case 0x20AC:
      return "EUR";
    default:
      return nullptr;
  }
}

void trimTrailingSpace(std::string& s) {
  while (!s.empty() && (s.back() == ' ' || s.back() == '\n')) s.pop_back();
}

void wrapParagraph(const std::string& para, int maxWidth, const MeasureFn& measure,
                   std::vector<std::string>& lines) {
  std::string line;
  size_t pos = 0;
  while (pos < para.size()) {
    size_t end = para.find(' ', pos);
    if (end == std::string::npos) end = para.size();
    const std::string word = para.substr(pos, end - pos);
    pos = end + 1;
    if (word.empty()) continue;

    const std::string candidate = line.empty() ? word : line + " " + word;
    if (measure(candidate) <= maxWidth) {
      line = candidate;
      continue;
    }
    if (!line.empty()) {
      lines.push_back(line);
      line.clear();
    }
    if (measure(word) <= maxWidth) {
      line = word;
      continue;
    }
    // The word alone is too wide: split it between code points.
    size_t i = 0;
    while (i < word.size()) {
      const size_t start = i;
      uint32_t cp;
      decodeNext(word, i, cp);
      const std::string grown = line + word.substr(start, i - start);
      if (!line.empty() && measure(grown) > maxWidth) {
        lines.push_back(line);
        line = word.substr(start, i - start);
      } else {
        line = grown;
      }
    }
  }
  if (!line.empty()) lines.push_back(line);
}

}  // namespace

std::string sanitize(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  bool pendingSpace = false;
  size_t i = 0;
  while (i < in.size()) {
    uint32_t cp;
    if (!decodeNext(in, i, cp)) continue;

    if (isLineBreak(cp)) {
      trimTrailingSpace(out);
      if (!out.empty()) out += '\n';
      pendingSpace = false;
      continue;
    }
    if (isSpace(cp)) {
      pendingSpace = true;
      continue;
    }

    std::string piece;
    if (const char* r = replacement(cp)) {
      piece = r;
    } else if (cp >= 0x20 && cp != 0x7F && !(cp >= 0x80 && cp < 0xA0) && cp != 0xAD && cp <= 0xFF) {
      appendUtf8(piece, cp);
    } else {
      continue;  // control characters, emoji, scripts the font can't draw
    }

    if (pendingSpace && !out.empty() && out.back() != '\n') out += ' ';
    pendingSpace = false;
    out += piece;
  }
  trimTrailingSpace(out);
  return out;
}

std::vector<std::string> wrap(const std::string& s, int maxWidth, const MeasureFn& measure) {
  std::vector<std::string> lines;
  size_t start = 0;
  while (start < s.size()) {
    size_t nl = s.find('\n', start);
    if (nl == std::string::npos) nl = s.size();
    wrapParagraph(s.substr(start, nl - start), maxWidth, measure, lines);
    start = nl + 1;
  }
  return lines;
}

size_t length(const std::string& s) {
  size_t count = 0;
  size_t i = 0;
  while (i < s.size()) {
    uint32_t cp;
    if (decodeNext(s, i, cp)) ++count;
  }
  return count;
}

}  // namespace text
