#include "notice.h"

#include <cctype>

#include "ancs.h"
#include "text.h"

namespace notice {
namespace {

struct KnownApp {
  const char* bundleId;
  const char* name;
};

const KnownApp kKnownApps[] = {
    {"com.apple.MobileSMS", "Messages"},
    {"com.apple.mobilephone", "Phone"},
    {"com.apple.mobilemail", "Mail"},
    {"com.apple.mobilecal", "Calendar"},
    {"com.apple.reminders", "Reminders"},
    {"com.apple.shortcuts", "Shortcuts"},
    {"com.apple.facetime", "FaceTime"},
    {"net.whatsapp.WhatsApp", "WhatsApp"},
    {"com.burbn.instagram", "Instagram"},
    {"com.hammerandchisel.discord", "Discord"},
    {"com.atebits.Tweetie2", "X"},
    {"com.toyopagroup.picaboo", "Snapchat"},
    {"com.google.Gmail", "Gmail"},
    {"com.spotify.client", "Spotify"},
    {"ph.telegra.Telegraph", "Telegram"},
    {"com.tinyspeck.chatlyio", "Slack"},
    {"com.microsoft.skype.teams", "Teams"},
};

std::string lower(const std::string& s) {
  std::string out = s;
  for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return out;
}

bool startsWithWord(const std::string& s, const char* prefix) {
  const std::string l = lower(s);
  const std::string p = prefix;
  if (l.compare(0, p.size(), p) != 0) return false;
  // "Weather" and "Weather:" match, "Weatherman" doesn't.
  return l.size() == p.size() || !std::isalnum(static_cast<unsigned char>(l[p.size()]));
}

// Last part of the app id, skipping generic endings: "com.spotify.client" -> "spotify".
std::string guessName(const std::string& bundleId) {
  static const char* const kGeneric[] = {"client", "app", "ios", "iphone", "mobile", "main"};
  std::string last;
  std::string previous;
  size_t start = 0;
  while (start <= bundleId.size()) {
    size_t dot = bundleId.find('.', start);
    if (dot == std::string::npos) dot = bundleId.size();
    previous = last;
    last = bundleId.substr(start, dot - start);
    start = dot + 1;
  }
  for (const char* g : kGeneric) {
    if (lower(last) == g && !previous.empty()) {
      last = previous;
      break;
    }
  }
  if (!last.empty()) last[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(last[0])));
  return last;
}

}  // namespace

std::string appName(const std::string& bundleId) {
  for (const KnownApp& app : kKnownApps) {
    if (bundleId == app.bundleId) return app.name;
  }
  const std::string guess = guessName(bundleId);
  return guess.empty() ? "Notification" : guess;
}

Routed route(const std::string& bundleId, uint8_t category, const std::string& rawTitle,
             const std::string& rawSubtitle, const std::string& rawMessage) {
  const std::string title = text::sanitize(rawTitle);
  const std::string subtitle = text::sanitize(rawSubtitle);
  const std::string message = text::sanitize(rawMessage);

  Routed r;
  if (title.empty() && subtitle.empty() && message.empty()) return r;  // Ignore

  const bool fromShortcuts = lower(bundleId).find("shortcuts") != std::string::npos;
  if (fromShortcuts && startsWithWord(title, "weather") && !message.empty()) {
    r.route = Route::Weather;
    r.weather = message;
    return r;
  }

  r.route = Route::Show;
  Notice& n = r.notice;
  if (fromShortcuts && startsWithWord(title, "claude")) {
    n.kind = NoticeKind::Claude;
    n.app = "Claude";
    n.body = message;
    return r;
  }

  n.kind = category == ancs::kIncomingCall ? NoticeKind::Call : NoticeKind::Generic;
  n.app = n.kind == NoticeKind::Call ? "Call" : appName(bundleId);
  n.title = title;
  n.body = subtitle.empty() ? message : (message.empty() ? subtitle : subtitle + "\n" + message);
  return r;
}

}  // namespace notice
