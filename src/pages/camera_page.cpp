#include "camera_page.h"

#include <TJpg_Decoder.h>

namespace {
TFT_eSPI *g_tft = nullptr;
int g_offsetX = 0;
int g_offsetY = 0;

// Contrast / saturation boost -- the camera snapshots are flat and this TN
// panel washes them out further. x100 values: 100 = unchanged.
constexpr int CONTRAST_X100 = 140;
constexpr int SATURATION_X100 = 140;

inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Operates on 8-bit channels; pixel arrives byte-swapped (TJpgDec swapBytes).
inline uint16_t enhance(uint16_t swapped) {
    uint16_t p = (swapped >> 8) | (swapped << 8);
    int r = ((p >> 11) & 31) * 255 / 31;
    int g = ((p >> 5) & 63) * 255 / 63;
    int b = (p & 31) * 255 / 31;
    int y = (r * 77 + g * 150 + b * 29) >> 8;  // luma
    r = y + (r - y) * SATURATION_X100 / 100;
    g = y + (g - y) * SATURATION_X100 / 100;
    b = y + (b - y) * SATURATION_X100 / 100;
    r = clampi((r - 128) * CONTRAST_X100 / 100 + 128, 0, 255);
    g = clampi((g - 128) * CONTRAST_X100 / 100 + 128, 0, 255);
    b = clampi((b - 128) * CONTRAST_X100 / 100 + 128, 0, 255);
    uint16_t out = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    return (out >> 8) | (out << 8);
}

// TJpg_Decoder pushes decoded blocks through this callback.
bool tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    if (g_tft == nullptr) {
        return false;
    }
    for (uint32_t i = 0; i < static_cast<uint32_t>(w) * h; i++) {
        bitmap[i] = enhance(bitmap[i]);
    }
    g_tft->pushImage(x + g_offsetX, y + g_offsetY, w, h, bitmap);
    return true;
}
}  // namespace

void CameraPage::begin(TFT_eSPI &tftRef, UnifiCameraManager &camRef) {
    tft = &tftRef;
    cam = &camRef;
    g_tft = &tftRef;
    TJpgDec.setCallback(tftOutput);
    TJpgDec.setSwapBytes(true);  // TFT_eSPI expects byte-swapped RGB565 from pushImage
}

void CameraPage::onShow() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->setTextDatum(MC_DATUM);
    tft->drawString("Connecting...", 160, 120, 4);
    tft->setTextDatum(TL_DATUM);
    lastSeq = 0;
    gotFrame = false;
    lastStatusDrawn = "";
    cam->markViewed(camIndex);
}

void CameraPage::onTap(int /*x*/, int /*y*/) {
    int n = UnifiCameraManager::numCameras();
    if (n > 1) {
        camIndex = (camIndex + 1) % n;
        onShow();
    }
}

void CameraPage::drawStatus(const String &s) {
    // Small strip at the bottom: camera name + fetch status, so a failing
    // fetch is visible on the screen instead of a silently frozen image.
    if (s == lastStatusDrawn) {
        return;
    }
    lastStatusDrawn = s;
    tft->fillRect(0, 224, 320, 16, TFT_BLACK);
    tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft->setTextDatum(TL_DATUM);
    String line = String(UnifiCameraManager::cameraName(camIndex)) + "  " + s;
    tft->drawString(line, 4, 226, 2);
}

void CameraPage::drawFrame() {
    uint16_t w = 0, h = 0;
    if (TJpgDec.getJpgSize(&w, &h, jpeg, jpegLen) != JDR_OK || w == 0 || h == 0) {
        drawStatus("bad image");
        return;
    }
    // Smallest power-of-two shrink that fits the usable area (320x224,
    // leaving a 16px strip for the status line).
    int scale = 1;
    while (scale < 8 && (w / scale > 320 || h / scale > 224)) {
        scale *= 2;
    }
    TJpgDec.setJpgScale(scale);
    int outW = w / scale, outH = h / scale;
    if (outW > 320) outW = 320;
    g_offsetX = (320 - outW) / 2;
    g_offsetY = (224 - outH) / 2;
    if (g_offsetY < 0) g_offsetY = 0;
    if (!gotFrame) {
        tft->fillScreen(TFT_BLACK);  // clear the "Connecting..." text once
        lastStatusDrawn = "";
        gotFrame = true;
    }
    TJpgDec.drawJpg(0, 0, jpeg, jpegLen);
}


void CameraPage::loop() {
    cam->markViewed(camIndex);
    if (cam->takeFrame(lastSeq, jpeg, jpegLen)) {
        drawFrame();
        free(jpeg);  // free right away so the next fetch has the RAM
        jpeg = nullptr;
        jpegLen = 0;
    }
    drawStatus(cam->lastStatus());
}
