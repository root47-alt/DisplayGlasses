#include "iphone_link.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

#include <algorithm>
#include <cstring>
#include <deque>
#include <mutex>
#include <vector>

#include "ancs.h"

namespace iphone {
namespace {

constexpr uint16_t kAppearanceEyeglasses = 0x01C0;  // Bluetooth SIG "Generic Eye-glasses"
constexpr uint16_t kGattService = 0x1801;           // Generic Attribute service
constexpr uint16_t kServiceChanged = 0x2A05;
constexpr uint16_t kCtsService = 0x1805;            // Current Time Service
constexpr uint16_t kCtsCurrentTime = 0x2A2B;

constexpr uint16_t kMtu = 247;
constexpr size_t kMaxPacket = kMtu - 3;
constexpr uint16_t kMaxTitle = 64;
constexpr uint16_t kMaxSubtitle = 64;
constexpr uint16_t kMaxMessage = 1000;  // room for a full Claude answer
constexpr uint32_t kResponseTimeoutMs = 4000;
constexpr uint32_t kSetupRetryMs = 3000;
constexpr size_t kMaxPending = 16;
constexpr size_t kMaxEvents = 32;

// ANCS service UUID, least significant byte first, for the advertising packet.
// "Soliciting" it is what makes iOS list the glasses in Settings > Bluetooth
// and offer to share notifications.
const uint8_t kAncsSolicitation[] = {
    17,    // length of this AD structure
    0x15,  // AD type: list of 128-bit service solicitation UUIDs
    0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4, 0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79,
};

struct Packet {
  uint16_t len;
  uint8_t bytes[kMaxPacket];
};

// What the Bluetooth worker hands to loop() on the Arduino task.
struct Event {
  enum Type : uint8_t { Notification, Removed, Time, Link };
  Type type = Notification;
  uint32_t uid = 0;
  uint8_t category = 0;
  bool connected = false;
  bool ready = false;
  clockutil::DateTime time;
  std::string appId, title, subtitle, message;
};

Handlers g_handlers;
NimBLEServer* g_server = nullptr;

// ---- Written by the NimBLE host task (callbacks), read by the worker.
QueueHandle_t g_sourceQ = nullptr;  // ancs::SourceEvent
QueueHandle_t g_dataQ = nullptr;    // Packet from the ANCS Data Source
QueueHandle_t g_timeQ = nullptr;    // Packet from the Current Time characteristic
volatile bool g_connected = false;
volatile bool g_encrypted = false;
volatile uint16_t g_connHandle = BLE_HS_CONN_HANDLE_NONE;
volatile uint32_t g_connCount = 0;        // goes up on every new connection
volatile bool g_servicesChanged = false;  // the phone added or removed a service (e.g. ANCS)

// ---- Worker -> loop().
std::mutex g_eventsMutex;
std::deque<Event> g_events;

// ---- Only touched by the worker task.
enum class Stage : uint8_t { Idle, Setup, Ready };
Stage g_stage = Stage::Idle;
uint32_t g_session = 0;  // g_connCount of the connection being served
uint32_t g_retryAt = 0;
NimBLEClient* g_client = nullptr;
NimBLERemoteCharacteristic* g_controlPoint = nullptr;
std::deque<ancs::SourceEvent> g_pending;
ancs::ResponseAssembler g_assembler;
ancs::SourceEvent g_inFlight;
uint32_t g_sentAt = 0;
bool g_reportedConnected = false;
bool g_reportedReady = false;

void emit(Event&& e) {
  std::lock_guard<std::mutex> lock(g_eventsMutex);
  if (g_events.size() >= kMaxEvents) g_events.pop_front();
  g_events.push_back(std::move(e));
}

// ---------------------------------------------------------------- callbacks
// These run on the NimBLE host task: copy the data and get out.

void queuePacket(QueueHandle_t queue, const uint8_t* data, size_t len) {
  Packet p;
  p.len = static_cast<uint16_t>(std::min(len, kMaxPacket));
  std::memcpy(p.bytes, data, p.len);
  xQueueSend(queue, &p, 0);  // drop it if the worker has fallen far behind
}

void onNotificationSource(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
  ancs::SourceEvent ev;
  if (ancs::parseSourceEvent(data, len, ev)) xQueueSend(g_sourceQ, &ev, 0);
}

void onDataSource(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
  queuePacket(g_dataQ, data, len);
}

void onCurrentTime(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
  queuePacket(g_timeQ, data, len);
}

void onServiceChanged(NimBLERemoteCharacteristic*, uint8_t*, size_t, bool) { g_servicesChanged = true; }

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*, NimBLEConnInfo& info) override {
    g_connHandle = info.getConnHandle();
    g_encrypted = info.isEncrypted();
    g_connCount = g_connCount + 1;
    g_connected = true;
    // Pair (first time) or re-encrypt with the saved keys (every time after).
    NimBLEDevice::startSecurity(info.getConnHandle());
  }

  void onDisconnect(NimBLEServer*, NimBLEConnInfo& info, int reason) override {
    if (info.getConnHandle() == g_connHandle) {
      g_connected = false;
      g_encrypted = false;
      g_connHandle = BLE_HS_CONN_HANDLE_NONE;
    }
    Serial.printf("[iphone] disconnected (reason 0x%x)\n", reason);
  }

  void onAuthenticationComplete(NimBLEConnInfo& info) override {
    if (info.getConnHandle() != g_connHandle) return;
    g_encrypted = info.isEncrypted();
    if (g_encrypted) {
      Serial.printf("[iphone] link encrypted, bonded=%d\n", info.isBonded());
    } else {
      Serial.println("[iphone] pairing failed or was cancelled; disconnecting");
      g_server->disconnect(info.getConnHandle());
    }
  }
};

// ------------------------------------------------------------------- worker

void report(bool connected, bool ready) {
  if (connected == g_reportedConnected && ready == g_reportedReady) return;
  g_reportedConnected = connected;
  g_reportedReady = ready;
  Event e;
  e.type = Event::Link;
  e.connected = connected;
  e.ready = ready;
  emit(std::move(e));
}

void parseTime(const uint8_t* d, size_t len) {
  if (len < 7) return;
  Event e;
  e.type = Event::Time;
  clockutil::DateTime& t = e.time;
  t.year = d[0] | (d[1] << 8);
  t.month = d[2];
  t.day = d[3];
  t.hour = d[4];
  t.minute = d[5];
  t.second = d[6];
  if (t.year < 2000 || t.month < 1 || t.month > 12 || t.day < 1 || t.day > 31 || t.hour > 23 ||
      t.minute > 59 || t.second > 59) {
    return;
  }
  emit(std::move(e));
}

// Forget everything about the current session (not the pairing).
void resetSession() {
  g_stage = Stage::Idle;
  g_client = nullptr;
  g_controlPoint = nullptr;
  g_pending.clear();
  g_assembler.reset();
  g_servicesChanged = false;
  xQueueReset(g_sourceQ);
  xQueueReset(g_dataQ);
  xQueueReset(g_timeQ);
}

// Finds ANCS (and the clock) on the phone and subscribes.
bool setupServices() {
  // getClient() clears the list of the phone's services, so only call it once
  // per session; later retries reuse it and look up what's still missing.
  if (g_client == nullptr) g_client = g_server->getClient(g_connHandle);
  if (g_client == nullptr) return false;

  NimBLERemoteService* ancsService = g_client->getService(ancs::kServiceUuid);
  if (ancsService == nullptr) {
    Serial.println("[iphone] ANCS not found yet (is this an iPhone, and is it paired?)");
    return false;
  }
  NimBLERemoteCharacteristic* source = ancsService->getCharacteristic(ancs::kNotificationSourceUuid);
  NimBLERemoteCharacteristic* control = ancsService->getCharacteristic(ancs::kControlPointUuid);
  NimBLERemoteCharacteristic* data = ancsService->getCharacteristic(ancs::kDataSourceUuid);
  if (source == nullptr || control == nullptr || data == nullptr) return false;

  // Data Source first, so no answer is missed once notifications start flowing.
  // iOS asks "Allow DisplayGlasses to receive your notifications?" around here.
  if (!data->subscribe(true, onDataSource)) return false;
  if (!source->subscribe(true, onNotificationSource)) return false;
  g_controlPoint = control;

  // iOS signals "Service Changed" when ANCS is turned off or on again.
  NimBLERemoteService* gatt = g_client->getService(NimBLEUUID(kGattService));
  NimBLERemoteCharacteristic* changed = gatt ? gatt->getCharacteristic(NimBLEUUID(kServiceChanged)) : nullptr;
  if (changed != nullptr && changed->canIndicate()) changed->subscribe(false, onServiceChanged);

  NimBLERemoteService* cts = g_client->getService(NimBLEUUID(kCtsService));
  NimBLERemoteCharacteristic* now = cts ? cts->getCharacteristic(NimBLEUUID(kCtsCurrentTime)) : nullptr;
  if (now != nullptr) {
    const NimBLEAttValue value = now->readValue();
    parseTime(value.data(), value.size());
    if (now->canNotify()) now->subscribe(true, onCurrentTime);
  } else {
    Serial.println("[iphone] phone has no Current Time Service; clock stays blank");
  }
  return true;
}

void requestNext(uint32_t now) {
  if (g_assembler.waiting() || g_pending.empty() || g_controlPoint == nullptr) return;
  g_inFlight = g_pending.front();
  g_pending.pop_front();
  const std::vector<uint8_t> cmd = ancs::buildGetAttributes(g_inFlight.uid, kMaxTitle, kMaxSubtitle, kMaxMessage);
  g_assembler.expect(g_inFlight.uid);
  g_sentAt = now;
  if (!g_controlPoint->writeValue(cmd.data(), cmd.size(), true)) {
    g_assembler.reset();  // usually: the notification was already gone
  }
}

void work() {
  const uint32_t now = millis();
  const bool connected = g_connected;
  const uint32_t session = g_connCount;

  // A disconnect, or a new connection since last time (even a quick
  // disconnect + reconnect), starts over.
  if (!connected || session != g_session) {
    if (g_stage != Stage::Idle) resetSession();
    if (!connected) {
      report(false, false);
      return;
    }
    g_session = session;
  }

  if (g_stage == Stage::Idle) {
    g_stage = Stage::Setup;
    g_retryAt = now + 500;
  }
  if (g_servicesChanged && g_stage == Stage::Ready) {
    Serial.println("[iphone] phone's services changed; setting up again");
    g_servicesChanged = false;
    g_stage = Stage::Setup;
    g_client = nullptr;  // rediscover everything
    g_controlPoint = nullptr;
    g_pending.clear();
    g_assembler.reset();
    g_retryAt = now + 500;
  }
  report(true, g_stage == Stage::Ready);

  if (g_stage == Stage::Setup) {
    if (!g_encrypted || static_cast<int32_t>(now - g_retryAt) < 0) return;
    if (setupServices()) {
      g_stage = Stage::Ready;
      Serial.println("[iphone] ready: receiving notifications");
      report(true, true);
    } else {
      g_retryAt = now + kSetupRetryMs;
    }
    return;
  }

  Packet p;
  while (xQueueReceive(g_timeQ, &p, 0) == pdTRUE) parseTime(p.bytes, p.len);

  ancs::SourceEvent ev;
  while (xQueueReceive(g_sourceQ, &ev, 0) == pdTRUE) {
    if (ev.eventId == ancs::kAdded && !(ev.flags & ancs::kFlagPreExisting)) {
      if (g_pending.size() >= kMaxPending) g_pending.pop_front();  // drop the oldest, like the screen does
      g_pending.push_back(ev);
    } else if (ev.eventId == ancs::kRemoved) {
      // Read, cleared, or a call that ended. Skip it if we haven't asked yet,
      // stop waiting if we're asking right now, and tell the screen.
      g_pending.erase(std::remove_if(g_pending.begin(), g_pending.end(),
                                     [&](const ancs::SourceEvent& e) { return e.uid == ev.uid; }),
                      g_pending.end());
      if (g_assembler.waiting() && g_inFlight.uid == ev.uid) g_assembler.reset();
      Event e;
      e.type = Event::Removed;
      e.uid = ev.uid;
      emit(std::move(e));
    }
  }

  while (xQueueReceive(g_dataQ, &p, 0) == pdTRUE) {
    if (g_assembler.feed(p.bytes, p.len)) {
      const ancs::Attributes& a = g_assembler.attributes();
      Event e;
      e.type = Event::Notification;
      e.uid = a.uid;
      e.category = g_inFlight.category;
      e.appId = a.appId;
      e.title = a.title;
      e.subtitle = a.subtitle;
      e.message = a.message;
      emit(std::move(e));
    }
  }

  if (g_assembler.waiting() && now - g_sentAt > kResponseTimeoutMs) g_assembler.reset();
  requestNext(now);
}

void workerTask(void*) {
  for (;;) {
    work();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

}  // namespace

void begin(const char* name, const Handlers& handlers) {
  g_handlers = handlers;
  g_sourceQ = xQueueCreate(16, sizeof(ancs::SourceEvent));
  g_dataQ = xQueueCreate(24, sizeof(Packet));
  g_timeQ = xQueueCreate(2, sizeof(Packet));

  NimBLEDevice::init(name);
  NimBLEDevice::setSecurityAuth(true, false, true);  // bond, no PIN, LE Secure Connections
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  NimBLEDevice::setMTU(kMtu);

  g_server = NimBLEDevice::createServer();
  g_server->setCallbacks(new ServerCallbacks());
  g_server->advertiseOnDisconnect(true);
  g_server->start();

  NimBLEAdvertisementData adv;
  adv.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
  adv.setAppearance(kAppearanceEyeglasses);
  adv.addData(kAncsSolicitation, sizeof kAncsSolicitation);
  NimBLEAdvertisementData scan;
  scan.setName(name);

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->setAdvertisementData(adv);
  advertising->setScanResponseData(scan);
  advertising->enableScanResponse(true);  // keeps the name after a Bluetooth stack reset
  advertising->start();
  Serial.printf("[iphone] advertising as \"%s\" (%d saved pairing(s))\n", name, NimBLEDevice::getNumBonds());

  xTaskCreatePinnedToCore(workerTask, "iphone", 8192, nullptr, 1, nullptr, ARDUINO_RUNNING_CORE);
}

void loop() {
  std::deque<Event> events;
  {
    std::lock_guard<std::mutex> lock(g_eventsMutex);
    events.swap(g_events);
  }
  for (const Event& e : events) {
    switch (e.type) {
      case Event::Notification:
        if (g_handlers.onNotification) g_handlers.onNotification(e.uid, e.appId, e.category, e.title, e.subtitle, e.message);
        break;
      case Event::Removed:
        if (g_handlers.onRemoved) g_handlers.onRemoved(e.uid);
        break;
      case Event::Time:
        if (g_handlers.onTime) g_handlers.onTime(e.time);
        break;
      case Event::Link:
        if (g_handlers.onLinkChange) g_handlers.onLinkChange(e.connected, e.ready);
        break;
    }
  }
}

void forget() { NimBLEDevice::deleteAllBonds(); }

}  // namespace iphone
