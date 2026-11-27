#include "camera_page.h"

#include <TJpg_Decoder.h>

#if defined(CYD_BIG)
#include <Arduino_GFX_Library.h>

#include "../big_display.h"
#endif

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

#if defined(CYD_BIG)
// Full-screen path for the 480x272 panel. Frames are 640x360 (16:9), so shrink
// 4:3 by dropping every 4th row/column -- blocks arrive at multiples of 8, so
// the mapping stays consistent across block boundaries. Anything already
// <= 480 wide is drawn 1:1.
bool g_shrink = false;
int g_destX = 0;
int g_destY = 0;

bool bigOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    Arduino_GFX *cv = BigDisplay::canvas();
    if (cv == nullptr) {
        return false;
    }
    if (!g_shrink) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(w) * h; i++) {
            bitmap[i] = enhance(bitmap[i]);
        }
        cv->draw16bitBeRGBBitmap(x + g_destX, y + g_destY, bitmap, w, h);
        return true;
    }
    static uint16_t out[16 * 16];
    int ow = 0, oh = 0;
    for (int sy = 0; sy < h; sy++) {
        if (((y + sy) & 3) == 3) {
            continue;
        }
        ow = 0;
        for (int sx = 0; sx < w; sx++) {
            if (((x + sx) & 3) == 3) {
                continue;
            }
            out[oh * 12 + ow] = enhance(bitmap[sy * w + sx]);
            ow++;
        }
        oh++;
    }
    // draw row by row since `out` is packed with a fixed 12-px stride
    for (int r = 0; r < oh; r++) {
        cv->draw16bitBeRGBBitmap(x * 3 / 4 + g_destX, y * 3 / 4 + r + g_destY, &out[r * 12], ow, 1);
    }
    return true;
}
#endif
}  // namespace

void CameraPage::begin(TFT_eSPI &tftRef, UnifiCameraManager &camRef) {
    tft = &tftRef;
    cam = &camRef;
    g_tft = &tftRef;
#if defined(CYD_BIG)
    TJpgDec.setCallback(bigOutput);
#else
    TJpgDec.setCallback(tftOutput);
#endif
    TJpgDec.setSwapBytes(true);  // TFT_eSPI expects byte-swapped RGB565 from pushImage
}

void CameraPage::onShow() {
#if defined(CYD_BIG)
    BigDisplay::clear();
    Arduino_GFX *cv = BigDisplay::canvas();
    cv->setTextColor(0xFFFF, 0x0000);
    cv->setTextSize(3);
    cv->setCursor(150, 125);
    cv->print("Connecting...");
    BigDisplay::flush();
    lastSeq = 0;
    gotFrame = false;
    lastStatusDrawn = "";
    cam->markViewed(camIndex);
    return;
#endif
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
#if defined(CYD_BIG)
    if (s == lastStatusDrawn) {
        return;
    }
    lastStatusDrawn = s;
    Arduino_GFX *cv = BigDisplay::canvas();
    cv->fillRect(0, 256, BigDisplay::PANEL_W, 16, 0x0000);
    cv->setTextColor(0x7BEF, 0x0000);
    cv->setTextSize(2);
    cv->setCursor(4, 257);
    cv->print(String(UnifiCameraManager::cameraName(camIndex)) + "  " + s);
    BigDisplay::flush();
    return;
#endif
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
#if defined(CYD_BIG)
    {
        // Smallest power-of-two JPEG shrink that is <= 640 wide, then 3/4 if
        // still wider than the 480px panel.
        int sc = 1;
        while (sc < 8 && w / sc > 640) {
            sc *= 2;
        }
        TJpgDec.setJpgScale(sc);
        int dw = w / sc, dh = h / sc;
        g_shrink = dw > BigDisplay::PANEL_W;
        int fw = g_shrink ? dw * 3 / 4 : dw;
        int fh = g_shrink ? dh * 3 / 4 : dh;
        g_destX = (BigDisplay::PANEL_W - fw) / 2;
        g_destY = (BigDisplay::PANEL_H - fh) / 2;
        if (g_destX < 0) g_destX = 0;
        if (g_destY < 0) g_destY = 0;
        if (!gotFrame) {
            BigDisplay::clear();
            lastStatusDrawn = "";
            gotFrame = true;
        }
        TJpgDec.drawJpg(0, 0, jpeg, jpegLen);
        drawStatus(cam->lastStatus());  // overlay after the image
        lastStatusDrawn = "";            // force re-draw next frame (image covered it)
        BigDisplay::flush();
        return;
    }
#endif
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
