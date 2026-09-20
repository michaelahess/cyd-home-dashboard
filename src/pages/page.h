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

    // True to disable the top/bottom page-navigation edge strips entirely,
    // sending every tap to onTap(x, y) instead -- even ones in what's
    // normally the nav band. Meant for a screen dense enough with real
    // controls (e.g. HVAC's per-zone control view) that an accidental tap
    // near the top/bottom edge should never silently change pages instead
    // of hitting the intended button; that screen should offer its own
    // explicit way back (e.g. a Back button) when this is true.
    virtual bool blocksPageNav() const { return false; }
};
