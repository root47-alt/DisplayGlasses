#include "ancs.h"

namespace ancs {
namespace {

constexpr uint8_t kCommandGetNotificationAttributes = 0;
constexpr size_t kRequestedAttributes = 4;  // app id, title, subtitle, message
constexpr size_t kMaxBuffer = 4096;         // drop garbage instead of growing forever

uint32_t readU32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

uint16_t readU16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

void putU16(std::vector<uint8_t>& out, uint16_t v) {
  out.push_back(static_cast<uint8_t>(v & 0xFF));
  out.push_back(static_cast<uint8_t>(v >> 8));
}

}  // namespace

bool parseSourceEvent(const uint8_t* data, size_t len, SourceEvent& out) {
  if (data == nullptr || len < 8) return false;
  out.eventId = data[0];
  out.flags = data[1];
  out.category = data[2];
  out.categoryCount = data[3];
  out.uid = readU32(data + 4);
  return out.eventId <= kRemoved;
}

std::vector<uint8_t> buildGetAttributes(uint32_t uid, uint16_t maxTitle, uint16_t maxSubtitle,
                                        uint16_t maxMessage) {
  std::vector<uint8_t> out;
  out.reserve(17);
  out.push_back(kCommandGetNotificationAttributes);
  for (int i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>((uid >> (8 * i)) & 0xFF));
  out.push_back(kAppIdentifier);  // the app id takes no length limit
  out.push_back(kTitle);
  putU16(out, maxTitle);
  out.push_back(kSubtitle);
  putU16(out, maxSubtitle);
  out.push_back(kMessage);
  putU16(out, maxMessage);
  return out;
}

void ResponseAssembler::expect(uint32_t uid) {
  buf_.clear();
  attrs_ = Attributes{};
  uid_ = uid;
  waiting_ = true;
}

void ResponseAssembler::reset() {
  buf_.clear();
  waiting_ = false;
}

bool ResponseAssembler::feed(const uint8_t* data, size_t len) {
  if (!waiting_ || data == nullptr || len == 0) return false;
  buf_.insert(buf_.end(), data, data + len);
  if (buf_.size() > kMaxBuffer) {
    buf_.clear();
    return false;
  }

  while (!buf_.empty()) {
    if (buf_[0] != kCommandGetNotificationAttributes) {
      buf_.clear();  // not the start of a response; wait for a clean one
      return false;
    }
    Attributes parsed;
    const size_t used = parse(parsed);
    if (used == 0) return false;  // need more packets
    buf_.erase(buf_.begin(), buf_.begin() + static_cast<std::ptrdiff_t>(used));
    if (parsed.uid == uid_) {
      attrs_ = parsed;
      waiting_ = false;
      buf_.clear();
      return true;
    }
    // A late answer to an older request: skip it and keep waiting.
  }
  return false;
}

size_t ResponseAssembler::parse(Attributes& out) const {
  if (buf_.size() < 5) return 0;
  out.uid = readU32(&buf_[1]);
  size_t pos = 5;
  for (size_t n = 0; n < kRequestedAttributes; ++n) {
    if (pos + 3 > buf_.size()) return 0;
    const uint8_t id = buf_[pos];
    const uint16_t len = readU16(&buf_[pos + 1]);
    if (pos + 3 + len > buf_.size()) return 0;
    std::string value(reinterpret_cast<const char*>(&buf_[pos + 3]), len);
    switch (id) {
      case kAppIdentifier: out.appId = std::move(value); break;
      case kTitle: out.title = std::move(value); break;
      case kSubtitle: out.subtitle = std::move(value); break;
      case kMessage: out.message = std::move(value); break;
      default: break;
    }
    pos += 3 + len;
  }
  return pos;
}

}  // namespace ancs
