#pragma once
// Apple Notification Center Service (ANCS): the Bluetooth protocol an iPhone
// uses to share its notifications with accessories such as smartwatches.
// This file only builds and parses the byte packets, so it can be unit-tested
// on a computer. The Bluetooth connection lives in src/iphone_link.cpp.
//
// Spec: Apple "ANCS Specification" (developer.apple.com).

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ancs {

constexpr const char* kServiceUuid = "7905F431-B5CE-4E99-A40F-4B1E122D00D0";
constexpr const char* kNotificationSourceUuid = "9FBF120D-6301-42D9-8C58-25E699A21DBD";
constexpr const char* kControlPointUuid = "69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9";
constexpr const char* kDataSourceUuid = "22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB";

enum EventId : uint8_t { kAdded = 0, kModified = 1, kRemoved = 2 };

enum EventFlag : uint8_t {
  kFlagSilent = 1 << 0,
  kFlagImportant = 1 << 1,
  kFlagPreExisting = 1 << 2,  // was already on the phone when we connected
  kFlagPositiveAction = 1 << 3,
  kFlagNegativeAction = 1 << 4,
};

enum Category : uint8_t {
  kOther = 0,
  kIncomingCall = 1,
  kMissedCall = 2,
  kVoicemail = 3,
  kSocial = 4,
  kSchedule = 5,
  kEmail = 6,
  kNews = 7,
  kHealthAndFitness = 8,
  kBusinessAndFinance = 9,
  kLocation = 10,
  kEntertainment = 11,
};

enum AttributeId : uint8_t {
  kAppIdentifier = 0,
  kTitle = 1,
  kSubtitle = 2,
  kMessage = 3,
};

// One packet from the Notification Source characteristic: "notification
// <uid> was added/modified/removed".
struct SourceEvent {
  uint8_t eventId = 0;
  uint8_t flags = 0;
  uint8_t category = 0;
  uint8_t categoryCount = 0;
  uint32_t uid = 0;
};

// Parses an 8-byte Notification Source packet. Returns false if malformed.
bool parseSourceEvent(const uint8_t* data, size_t len, SourceEvent& out);

// Builds the Control Point command that asks for a notification's app id,
// title, subtitle and message (each text capped at the given byte lengths).
std::vector<uint8_t> buildGetAttributes(uint32_t uid, uint16_t maxTitle, uint16_t maxSubtitle,
                                        uint16_t maxMessage);

struct Attributes {
  uint32_t uid = 0;
  std::string appId;
  std::string title;
  std::string subtitle;
  std::string message;
};

// Collects the Data Source response to one buildGetAttributes() request.
// iOS may split a response over several Bluetooth packets, so feed() them in
// order until it returns true.
class ResponseAssembler {
 public:
  // Starts waiting for the response about `uid`.
  void expect(uint32_t uid);
  // Feeds one Data Source packet. Returns true when the full response for the
  // expected uid has arrived; read it with attributes().
  bool feed(const uint8_t* data, size_t len);
  bool waiting() const { return waiting_; }
  const Attributes& attributes() const { return attrs_; }
  void reset();

 private:
  // Parses one complete response from the front of buf_. Returns the bytes it
  // used, or 0 if more data is needed.
  size_t parse(Attributes& out) const;

  std::vector<uint8_t> buf_;
  Attributes attrs_;
  uint32_t uid_ = 0;
  bool waiting_ = false;
};

}  // namespace ancs
