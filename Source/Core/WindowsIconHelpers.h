#pragma once

#if JUCE_WINDOWS
#include <JuceHeader.h>
#include <windows.h>

// ============================================================================
// WindowsIconHelper
// Responsibility: Set the native Win32 taskbar & window icon from a juce::Image.
// Uses raw GDI (CreateDIBSection) — does NOT depend on JUCE internal APIs that
// were removed in JUCE 8 (e.g. juce::IconConverters::createHBITMAPFromImage).
// ============================================================================
namespace WindowsIconHelper {

    inline void setTaskbarIcon(juce::Component* component, const juce::Image& image) {
        if (!image.isValid() || component == nullptr)
            return;

        // Work with an ARGB copy at a good icon size
        const int iconSize = 256;
        juce::Image argb = image.convertedToFormat(juce::Image::ARGB)
                                .rescaled(iconSize, iconSize, juce::Graphics::highResamplingQuality);

        // Create a DIB section (top-down, 32-bit BGRA) for the colour bitmap
        BITMAPV5HEADER bi {};
        bi.bV5Size        = sizeof(bi);
        bi.bV5Width       = iconSize;
        bi.bV5Height      = -iconSize; // negative = top-down
        bi.bV5Planes      = 1;
        bi.bV5BitCount    = 32;
        bi.bV5Compression = BI_BITFIELDS;
        bi.bV5RedMask     = 0x00FF0000;
        bi.bV5GreenMask   = 0x0000FF00;
        bi.bV5BlueMask    = 0x000000FF;
        bi.bV5AlphaMask   = 0xFF000000;

        void* pBits = nullptr;
        HDC   hdc   = GetDC(nullptr);
        HBITMAP hColor = CreateDIBSection(hdc, reinterpret_cast<BITMAPINFO*>(&bi),
                                          DIB_RGB_COLORS, &pBits, nullptr, 0);
        ReleaseDC(nullptr, hdc);

        if (!hColor || !pBits)
            return;

        // Copy pixel data: juce::Image is ARGB (B,G,R,A in memory on little-endian)
        juce::Image::BitmapData bmp(argb, juce::Image::BitmapData::readOnly);
        auto* dst = reinterpret_cast<DWORD*>(pBits);
        for (int y = 0; y < iconSize; ++y) {
            for (int x = 0; x < iconSize; ++x) {
                juce::Colour c = argb.getPixelAt(x, y);
                // Pack as BGRA (Win32 native order) with pre-multiplied alpha
                BYTE a = c.getAlpha();
                dst[y * iconSize + x] = ((DWORD)a << 24)
                                      | ((DWORD)c.getRed()   * a / 255 << 16)
                                      | ((DWORD)c.getGreen() * a / 255 <<  8)
                                      | ((DWORD)c.getBlue()  * a / 255);
            }
        }

        // Mono mask — all zeros means "use alpha channel"
        HBITMAP hMask = CreateBitmap(iconSize, iconSize, 1, 1, nullptr);

        ICONINFO ii {};
        ii.fIcon    = TRUE;
        ii.hbmColor = hColor;
        ii.hbmMask  = hMask;

        if (HICON hIcon = CreateIconIndirect(&ii)) {
            if (auto* hwnd = reinterpret_cast<HWND>(component->getWindowHandle())) {
                SendMessage(hwnd, WM_SETICON, ICON_BIG,   reinterpret_cast<LPARAM>(hIcon));
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
            }
            // The OS takes ownership of hIcon; do not call DestroyIcon here.
        }

        DeleteObject(hMask);
        DeleteObject(hColor);
    }

} // namespace WindowsIconHelper
#endif
