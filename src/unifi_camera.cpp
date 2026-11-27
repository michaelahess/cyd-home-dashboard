#include "unifi_camera.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "data_mutex.h"
#include "secrets.h"

namespace {
constexpr uint32_t VIEW_TIMEOUT_MS = 3000;   // stop fetching if the page hasn't been drawn lately
constexpr uint32_t FETCH_INTERVAL_MS = 100;  // min gap between fetches; the fetch itself is the real cadence

#if UNIFI_NUM_CAMERAS > 0
const char *const CAMERA_IDS[] = UNIFI_CAMERA_IDS;
const char *const CAMERA_NAMES[] = UNIFI_CAMERA_NAMES;
#endif
}  // namespace

bool UnifiCameraManager::configured() {
    return UNIFI_NUM_CAMERAS > 0 && UNIFI_HOST[0] != '\0' && UNIFI_API_KEY[0] != '\0';
}

int UnifiCameraManager::numCameras() {
    return configured() ? UNIFI_NUM_CAMERAS : 0;
}

const char *UnifiCameraManager::cameraName(int index) {
#if UNIFI_NUM_CAMERAS > 0
    if (index >= 0 && index < UNIFI_NUM_CAMERAS) {
        return CAMERA_NAMES[index];
    }
#endif
    return "";
}

void UnifiCameraManager::markViewed(int cameraIndex) {
    viewedMs = millis();
    wantCamera = cameraIndex;
}

String UnifiCameraManager::lastStatus() const {
    DataLock lock;
    return status;
}

bool UnifiCameraManager::takeFrame(uint32_t &lastSeq, uint8_t *&buf, size_t &len) {
    DataLock lock;
    if (frameSeq == lastSeq || frame == nullptr) {
        return false;
    }
    // Hand the buffer over (no copy -- RAM is tight with no PSRAM). The
    // caller frees whatever it passed in and must free() the new buffer.
    free(buf);
    buf = frame;
    len = frameLen;
    frame = nullptr;
    frameLen = 0;
    lastSeq = frameSeq;
    return true;
}

void UnifiCameraManager::loop() {
    if (!configured() || WiFi.status() != WL_CONNECTED) {
        return;
    }
    uint32_t now = millis();
    if (now - viewedMs > VIEW_TIMEOUT_MS) {
        // page not visible: drop the TLS session and its RAM
        if (client != nullptr) {
            client->stop();
            delete client;
            client = nullptr;
        }
        return;
    }
    if (lastFetchMs != 0 && now - lastFetchMs < FETCH_INTERVAL_MS) {
        return;
    }
    lastFetchMs = now;
    fetchNow(wantCamera);
}

bool UnifiCameraManager::fetchNow(int cameraIndex) {
#if UNIFI_NUM_CAMERAS > 0
    if (cameraIndex < 0 || cameraIndex >= UNIFI_NUM_CAMERAS) {
        return false;
    }
    if (client == nullptr) {
        client = new NetworkClientSecure();
        client->setInsecure();  // local console with a self-signed cert, same tradeoff as the HA client
    }
    uint32_t t0 = millis();
    HTTPClient http;
    http.setReuse(true);
    String url = String(UNIFI_HOST) + "/proxy/protect/integration/v1/cameras/" + CAMERA_IDS[cameraIndex] +
                 "/snapshot?highQuality=false";
    http.begin(*client, url);
    http.addHeader("X-API-KEY", UNIFI_API_KEY);
    http.setTimeout(5000);

    int code = http.GET();
    String st;
    if (code != 200) {
        client->stop();  // reset the connection after any error
        st = code < 0 ? String("conn ") + HTTPClient::errorToString(code) : String("HTTP ") + code;
        http.end();
        DataLock lock;
        status = st;
        return false;
    }

    int len = http.getSize();
    if (len <= 0 || static_cast<size_t>(len) > MAX_JPEG_BYTES) {
        http.end();
        DataLock lock;
        status = len <= 0 ? "no length" : String("frame too big ") + (len / 1024) + "k";
        return false;
    }

    uint8_t *tmp = static_cast<uint8_t *>(malloc(len));
    if (tmp == nullptr) {
        http.end();
        DataLock lock;
        status = String("low mem ") + (len / 1024) + "k heap " + (ESP.getFreeHeap() / 1024) + "k";
        return false;
    }
    WiFiClient *stream = http.getStreamPtr();
    int got = 0;
    uint32_t startMs = millis();
    while (got < len && millis() - startMs < 8000) {
        int avail = stream->available();
        if (avail > 0) {
            int n = stream->read(tmp + got, min(avail, len - got));
            if (n > 0) {
                got += n;
            }
        } else if (!stream->connected()) {
            break;
        } else {
            delay(2);
        }
    }
    http.end();

    DataLock lock;
    if (got != len) {
        client->stop();
        free(tmp);
        status = String("short ") + (got / 1024) + "/" + (len / 1024) + "k heap " + (ESP.getFreeHeap() / 1024) + "k";
        return false;
    }
    free(frame);
    frame = tmp;
    frameLen = len;
    frameSeq++;
    lastFetchDurMs = millis() - t0;
    status = String("OK ") + (len / 1024) + "k " + lastFetchDurMs + "ms heap " + (ESP.getFreeHeap() / 1024) + "k";
    return true;
#else
    (void)cameraIndex;
    return false;
#endif
}
