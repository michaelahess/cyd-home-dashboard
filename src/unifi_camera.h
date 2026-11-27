#pragma once

#include <Arduino.h>

#include "secrets.h"  // before the defaults below so real values win without macro redefinition

// Optional UniFi Protect camera snapshots. Configure in secrets.h (all three
// must be set, otherwise the Camera page is simply not added):
//   #define UNIFI_HOST       "https://192.168.1.1"   // Protect console, no trailing slash
//   #define UNIFI_API_KEY    "..."                   // Protect > Settings > Control Plane > Integrations
//   #define UNIFI_CAMERA_IDS   {"<camera id>"}       // from GET /proxy/protect/integration/v1/cameras
//   #define UNIFI_CAMERA_NAMES {"Front door"}        // same length as the ID list
//
// Uses Protect's Integration API snapshot endpoint (JPEG, X-API-KEY header).
// Frames are fetched on the network task ONLY while the Camera page is
// visible (the page calls markViewed() every loop), so it costs nothing when
// you're looking at another page.
#ifndef UNIFI_HOST
#define UNIFI_HOST ""
#endif
#ifndef UNIFI_API_KEY
#define UNIFI_API_KEY ""
#endif
#ifndef UNIFI_CAMERA_IDS
#define UNIFI_CAMERA_IDS {}
#endif
#ifndef UNIFI_CAMERA_NAMES
#define UNIFI_CAMERA_NAMES {}
#endif
#ifndef UNIFI_NUM_CAMERAS
#define UNIFI_NUM_CAMERAS 0
#endif

class NetworkClientSecure;

class UnifiCameraManager {
public:
    static constexpr size_t MAX_JPEG_BYTES = 72 * 1024;  // CYD has no PSRAM; refuse anything bigger

    static bool configured();
    static int numCameras();
    static const char *cameraName(int index);

    // Called by the page every loop while visible; gates network fetching.
    void markViewed(int cameraIndex);
    // Runs on the network task.
    void loop();

    // Main-loop side: returns true (and copies the frame into `out`) if a
    // frame newer than `lastSeq` is ready. Caller owns/resizes `out`.
    bool takeFrame(uint32_t &lastSeq, uint8_t *&buf, size_t &len);
    String lastStatus() const;

private:
    volatile uint32_t viewedMs = 0;
    volatile int wantCamera = 0;
    uint8_t *frame = nullptr;
    size_t frameLen = 0;
    uint32_t frameSeq = 0;
    unsigned long lastFetchMs = 0;
    NetworkClientSecure *client = nullptr;  // kept open while viewing -> no TLS handshake per frame
    uint32_t lastFetchDurMs = 0;
    String status = "idle";

    bool fetchNow(int cameraIndex);
};
