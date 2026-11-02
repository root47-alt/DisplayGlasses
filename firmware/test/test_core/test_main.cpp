// Unit tests for lib/core. Run on your computer with:  pio test -e native
#include <unity.h>

#include <string>
#include <vector>

#include "ancs.h"
#include "clock.h"
#include "notice.h"
#include "text.h"

void setUp() {}
void tearDown() {}

// Fake font: every character is 6 px wide, like u8g2_font_6x10.
static int measure6(const std::string& s) { return static_cast<int>(text::length(s)) * 6; }

static void assertLines(const std::vector<std::string>& expected, const std::vector<std::string>& actual) {
  TEST_ASSERT_EQUAL_UINT32(expected.size(), actual.size());
  for (size_t i = 0; i < expected.size(); ++i) TEST_ASSERT_EQUAL_STRING(expected[i].c_str(), actual[i].c_str());
}

// ---------- text ----------

void test_sanitize_keeps_latin1_accents() {
  TEST_ASSERT_EQUAL_STRING("Caf\xC3\xA9 con ni\xC3\xB1os", text::sanitize("Caf\xC3\xA9 con ni\xC3\xB1os").c_str());
}

void test_sanitize_maps_smart_punctuation() {
  // I’m “here” — ok…
  const std::string in = "I\xE2\x80\x99m \xE2\x80\x9Chere\xE2\x80\x9D \xE2\x80\x94 ok\xE2\x80\xA6";
  TEST_ASSERT_EQUAL_STRING("I'm \"here\" - ok...", text::sanitize(in).c_str());
}

void test_sanitize_drops_emoji_and_collapses_spaces() {
  // "Hi 👋  there\t!" plus a zero-width joiner and an invalid byte
  const std::string in = "  Hi \xF0\x9F\x91\x8B  there\t\xE2\x80\x8D!\xFF ";
  TEST_ASSERT_EQUAL_STRING("Hi there !", text::sanitize(in).c_str());
}

void test_sanitize_collapses_blank_lines() {
  TEST_ASSERT_EQUAL_STRING("a\nb\nc", text::sanitize("a \n\n\nb\r\n c\n").c_str());
}

void test_wrap_words() {
  assertLines({"hello", "world"}, text::wrap("hello world", 30, measure6));
  assertLines({"a bc", "def"}, text::wrap("a bc def", 30, measure6));
}

void test_wrap_splits_long_words() {
  assertLines({"abcde", "fghij", "kl"}, text::wrap("abcdefghijkl", 30, measure6));
  assertLines({"hi", "abcde", "fg ok"}, text::wrap("hi abcdefg ok", 30, measure6));
}

void test_wrap_counts_code_points_not_bytes() {
  const std::string w = "\xC3\xB1" "and\xC3\xBA";  // ñandú: 5 characters, 7 bytes
  assertLines({w, w}, text::wrap(w + " " + w, 30, measure6));
}

void test_wrap_honors_newlines() {
  assertLines({"ab", "cd"}, text::wrap("ab\ncd", 60, measure6));
  TEST_ASSERT_EQUAL_UINT32(0, text::wrap("", 60, measure6).size());
}

// ---------- clock ----------

void test_epoch_known_values() {
  TEST_ASSERT_EQUAL_INT64(0, clockutil::toEpoch(1970, 1, 1, 0, 0, 0));
  TEST_ASSERT_EQUAL_INT64(951868800, clockutil::toEpoch(2000, 3, 1, 0, 0, 0));
}

void test_epoch_round_trip_and_weekday() {
  const int64_t e = clockutil::toEpoch(2026, 11, 2, 21, 5, 9);
  const clockutil::DateTime t = clockutil::fromEpoch(e);
  TEST_ASSERT_EQUAL_INT(2026, t.year);
  TEST_ASSERT_EQUAL_INT(11, t.month);
  TEST_ASSERT_EQUAL_INT(2, t.day);
  TEST_ASSERT_EQUAL_INT(21, t.hour);
  TEST_ASSERT_EQUAL_INT(5, t.minute);
  TEST_ASSERT_EQUAL_INT(9, t.second);
  TEST_ASSERT_EQUAL_INT(1, t.weekday);  // Monday

  const clockutil::DateTime leap = clockutil::fromEpoch(clockutil::toEpoch(2028, 2, 29, 23, 59, 59) + 1);
  TEST_ASSERT_EQUAL_INT(3, leap.month);
  TEST_ASSERT_EQUAL_INT(1, leap.day);
}

void test_format_time_and_date() {
  clockutil::DateTime t = clockutil::fromEpoch(clockutil::toEpoch(2026, 11, 2, 0, 5, 0));
  TEST_ASSERT_EQUAL_STRING("12:05", clockutil::formatTime(t, false).c_str());
  TEST_ASSERT_EQUAL_STRING("AM", clockutil::amPm(t, false));
  t = clockutil::fromEpoch(clockutil::toEpoch(2026, 11, 2, 13, 30, 0));
  TEST_ASSERT_EQUAL_STRING("1:30", clockutil::formatTime(t, false).c_str());
  TEST_ASSERT_EQUAL_STRING("PM", clockutil::amPm(t, false));
  TEST_ASSERT_EQUAL_STRING("13:30", clockutil::formatTime(t, true).c_str());
  TEST_ASSERT_EQUAL_STRING("", clockutil::amPm(t, true));
  TEST_ASSERT_EQUAL_STRING("Mon 2 Nov", clockutil::formatDate(t).c_str());
}

void test_clock_counts_forward() {
  clockutil::Clock clock;
  TEST_ASSERT_FALSE(clock.valid());
  clockutil::DateTime start = clockutil::fromEpoch(clockutil::toEpoch(2026, 11, 2, 9, 41, 0));
  clock.set(start, 4294960000u);  // just before millis() wraps
  const clockutil::DateTime later = clock.now(65000u);  // 72.3 s later, after the wrap
  TEST_ASSERT_TRUE(clock.valid());
  TEST_ASSERT_EQUAL_INT(9, later.hour);
  TEST_ASSERT_EQUAL_INT(42, later.minute);
  TEST_ASSERT_EQUAL_INT(12, later.second);
}

// ---------- ANCS ----------

void test_parse_source_event() {
  const uint8_t packet[] = {0x00, 0x02, 0x04, 0x01, 0x78, 0x56, 0x34, 0x12};
  ancs::SourceEvent ev;
  TEST_ASSERT_TRUE(ancs::parseSourceEvent(packet, sizeof packet, ev));
  TEST_ASSERT_EQUAL_UINT8(ancs::kAdded, ev.eventId);
  TEST_ASSERT_EQUAL_UINT8(ancs::kFlagImportant, ev.flags);
  TEST_ASSERT_EQUAL_UINT8(ancs::kSocial, ev.category);
  TEST_ASSERT_EQUAL_UINT32(0x12345678u, ev.uid);
  TEST_ASSERT_FALSE(ancs::parseSourceEvent(packet, 7, ev));
}

void test_build_get_attributes() {
  const std::vector<uint8_t> cmd = ancs::buildGetAttributes(0x01020304, 64, 32, 1000);
  const std::vector<uint8_t> expected = {0x00, 0x04, 0x03, 0x02, 0x01, 0x00, 0x01, 64, 0,
                                         0x02, 32,   0,    0x03, 0xE8, 0x03};
  TEST_ASSERT_EQUAL_UINT32(expected.size(), cmd.size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.data(), cmd.data(), expected.size());
}

static std::vector<uint8_t> makeResponse(uint32_t uid, const std::string& app, const std::string& title,
                                         const std::string& subtitle, const std::string& message) {
  std::vector<uint8_t> r = {0x00};
  for (int i = 0; i < 4; ++i) r.push_back(static_cast<uint8_t>((uid >> (8 * i)) & 0xFF));
  const std::string* values[] = {&app, &title, &subtitle, &message};
  for (uint8_t id = 0; id < 4; ++id) {
    r.push_back(id);
    r.push_back(static_cast<uint8_t>(values[id]->size() & 0xFF));
    r.push_back(static_cast<uint8_t>(values[id]->size() >> 8));
    r.insert(r.end(), values[id]->begin(), values[id]->end());
  }
  return r;
}

void test_assembler_joins_fragments() {
  const std::vector<uint8_t> resp = makeResponse(7, "com.apple.MobileSMS", "Mom", "", "Dinner at 8?");
  ancs::ResponseAssembler a;
  a.expect(7);
  TEST_ASSERT_FALSE(a.feed(resp.data(), 3));
  TEST_ASSERT_FALSE(a.feed(resp.data() + 3, 20));
  TEST_ASSERT_TRUE(a.feed(resp.data() + 23, resp.size() - 23));
  TEST_ASSERT_FALSE(a.waiting());
  TEST_ASSERT_EQUAL_UINT32(7, a.attributes().uid);
  TEST_ASSERT_EQUAL_STRING("com.apple.MobileSMS", a.attributes().appId.c_str());
  TEST_ASSERT_EQUAL_STRING("Mom", a.attributes().title.c_str());
  TEST_ASSERT_EQUAL_STRING("", a.attributes().subtitle.c_str());
  TEST_ASSERT_EQUAL_STRING("Dinner at 8?", a.attributes().message.c_str());
}

void test_assembler_skips_stale_responses() {
  std::vector<uint8_t> both = makeResponse(5, "a", "old", "", "stale");
  const std::vector<uint8_t> fresh = makeResponse(6, "b", "new", "", "fresh");
  both.insert(both.end(), fresh.begin(), fresh.end());
  ancs::ResponseAssembler a;
  a.expect(6);
  TEST_ASSERT_TRUE(a.feed(both.data(), both.size()));
  TEST_ASSERT_EQUAL_STRING("fresh", a.attributes().message.c_str());
}

void test_assembler_ignores_packets_when_idle() {
  const std::vector<uint8_t> resp = makeResponse(1, "a", "b", "", "c");
  ancs::ResponseAssembler a;
  TEST_ASSERT_FALSE(a.feed(resp.data(), resp.size()));
}

// ---------- routing ----------

void test_app_names() {
  TEST_ASSERT_EQUAL_STRING("Messages", notice::appName("com.apple.MobileSMS").c_str());
  TEST_ASSERT_EQUAL_STRING("Foo", notice::appName("com.foo.client").c_str());
  TEST_ASSERT_EQUAL_STRING("Coolapp", notice::appName("com.example.coolapp").c_str());
  TEST_ASSERT_EQUAL_STRING("Notification", notice::appName("").c_str());
}

void test_route_weather_from_shortcuts() {
  const notice::Routed r = notice::route("com.apple.shortcuts", 0, "Weather", "", "24\xC2\xB0" "C Sunny");
  TEST_ASSERT_EQUAL_INT(static_cast<int>(notice::Route::Weather), static_cast<int>(r.route));
  TEST_ASSERT_EQUAL_STRING("24\xC2\xB0" "C Sunny", r.weather.c_str());

  // Another app's "Weather" notification is just a notification.
  const notice::Routed other = notice::route("com.example.weatherapp", 0, "Weather", "", "Storm warning");
  TEST_ASSERT_EQUAL_INT(static_cast<int>(notice::Route::Show), static_cast<int>(other.route));
}

void test_route_claude_answer() {
  const notice::Routed r = notice::route("com.apple.shortcuts", 0, "Claude", "", "About 384,400 km.");
  TEST_ASSERT_EQUAL_INT(static_cast<int>(notice::Route::Show), static_cast<int>(r.route));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(NoticeKind::Claude), static_cast<int>(r.notice.kind));
  TEST_ASSERT_EQUAL_STRING("Claude", r.notice.app.c_str());
  TEST_ASSERT_EQUAL_STRING("About 384,400 km.", r.notice.body.c_str());
}

void test_route_generic_and_calls() {
  notice::Routed r = notice::route("com.apple.mobilemail", ancs::kEmail, "Ana", "Lab report", "See attached");
  TEST_ASSERT_EQUAL_STRING("Mail", r.notice.app.c_str());
  TEST_ASSERT_EQUAL_STRING("Ana", r.notice.title.c_str());
  TEST_ASSERT_EQUAL_STRING("Lab report\nSee attached", r.notice.body.c_str());

  r = notice::route("com.apple.mobilephone", ancs::kIncomingCall, "Mom", "", "Incoming Call");
  TEST_ASSERT_EQUAL_INT(static_cast<int>(NoticeKind::Call), static_cast<int>(r.notice.kind));
  TEST_ASSERT_EQUAL_STRING("Call", r.notice.app.c_str());

  r = notice::route("com.apple.MobileSMS", 0, " ", "", "\xF0\x9F\x91\x8D");  // only an emoji
  TEST_ASSERT_EQUAL_INT(static_cast<int>(notice::Route::Ignore), static_cast<int>(r.route));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_sanitize_keeps_latin1_accents);
  RUN_TEST(test_sanitize_maps_smart_punctuation);
  RUN_TEST(test_sanitize_drops_emoji_and_collapses_spaces);
  RUN_TEST(test_sanitize_collapses_blank_lines);
  RUN_TEST(test_wrap_words);
  RUN_TEST(test_wrap_splits_long_words);
  RUN_TEST(test_wrap_counts_code_points_not_bytes);
  RUN_TEST(test_wrap_honors_newlines);
  RUN_TEST(test_epoch_known_values);
  RUN_TEST(test_epoch_round_trip_and_weekday);
  RUN_TEST(test_format_time_and_date);
  RUN_TEST(test_clock_counts_forward);
  RUN_TEST(test_parse_source_event);
  RUN_TEST(test_build_get_attributes);
  RUN_TEST(test_assembler_joins_fragments);
  RUN_TEST(test_assembler_skips_stale_responses);
  RUN_TEST(test_assembler_ignores_packets_when_idle);
  RUN_TEST(test_app_names);
  RUN_TEST(test_route_weather_from_shortcuts);
  RUN_TEST(test_route_claude_answer);
  RUN_TEST(test_route_generic_and_calls);
  return UNITY_END();
}
