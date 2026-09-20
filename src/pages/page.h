#pragma once

// Common lifecycle for a swipeable full-screen page.
class Page {
public:
    virtual ~Page() = default;

    // Called once whenever this page becomes the visible page. Should clear
    // the screen and draw the page's static layout.
    virtual void onShow() = 0;

    // Called every main-loop iteration while this page is visible.
    virtual void loop() = 0;

    // Called when a tap lands outside the top/bottom page-navigation edge
    // strips. Default: ignored. Override to build in-page interaction
    // (e.g. tapping a row to open detail/control view for it).
    virtual void onTap(int /*x*/, int /*y*/) {}
};
