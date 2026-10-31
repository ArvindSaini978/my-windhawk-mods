# Drag to Scroll

Hold **`Ctrl` + Right-Click** and drag your mouse to scroll naturally and effortlessly across desktop apps—including web browsers (Chrome, Edge, Firefox), documents (Word, PDFs), code editors (VS Code), and File Explorer.

## Vertical & Horizontal Scrolling
![Drag to Scroll Demo 1](https://raw.githubusercontent.com/ArvindSaini978/my-windhawk-mods/refs/heads/master/assets/dts_preview_1.webp)


## Hands-Free Continuous Cruising
![Drag to Scroll Demo 2](https://raw.githubusercontent.com/ArvindSaini978/my-windhawk-mods/refs/heads/master/assets/dts_preview_2.webp)

---

## ⚡ Quick Start (How to Use)

* **To Scroll:** Hold down **`Ctrl`**, hold down the **Right Mouse Button**, and move your mouse up, down, left, or right.
* **To Stop:** Release the Right Mouse Button.
* **Normal Right-Click:** Regular Right-Click is completely untouched—click anywhere like you normally do to open context menus.
* **Need an App's Own Ctrl + Right-Click?** Just tap `Ctrl + Right-Click` in place without moving your mouse—it triggers the app's native shortcut as usual.
* **Hands-Free Cruising (Optional):** Turn on *Continuous Cruising* in settings. Drag to start moving, let go of the mouse button while still holding `Ctrl`, and the page will glide continuously on its own. Tap any key or click to stop.

---

## ✨ Everyday Highlights (Why You'll Love It)

* 🎯 **Smart Straight-Line Panning:** No more accidental diagonal wobbling when reading documents or scanning spreadsheets. Move up/down or left/right, and the mod keeps your movement locked straight on track.
* 👁️ **High-Contrast, All-Surface Cursors:** Custom drag cursors with protective contours are built directly into the mod. Whether you use pure Dark Mode, bright white documents, or customized themes, the cursor stays crisp, sharp, and easy to follow.
* 🔍 **Zero Accidental Zooming:** Holding `Ctrl` while scrolling in web browsers usually messes up your page zoom. This mod automatically shields your browser during both the drag and the gentle stop, so you never accidentally blow up text or shrink your screen.
* 🚀 **Flick & Glide Dynamic Speed:** Drag slowly for comfortable, line-by-line reading, or flick your mouse quickly to fly across lengthy web pages and documents.
* 📱 **Touchscreen or Traditional Feel:** Choose between natural pull-to-scroll (moving down drags content down, just like a phone or tablet) or classic wheel style in settings.

---

## 🛠️ Detailed Features

* **Directional Axis Locking & 2D Horizontal Panning:** Full sideways and vertical scrolling with smart threshold checks to lock your movement to the primary direction.
* **Silky Smooth Motion Filtering:** Low-pass input smoothing eliminates hand tremors and mouse sensor jitter, providing a fluid, trackpad-like glide.
* **Sub-Notch High-Resolution Micro-Scrolling:** Emits fractional wheel increments to modern web browsers and editors for pixel-level precision instead of jagged notch jumps.
* **Browser Compositor Drain Masking:** Keeps `Ctrl` and modifier flags suppressed during both active drags and kinetic momentum drain, preventing compositor zoom interference in Chromium and Gecko engines.
* **Embedded High-DPI Dual-Contour Cursors:** Custom self-contained cursor assets automatically scale to your system DPI display metrics (`SM_CXCURSOR`) with centered hotspots and zero external file requirements.
* **Legacy Control Detection:** Detects classic Win32 controls (e.g., File Explorer list views, TreeViews, text fields) and automatically switches to discrete line steps (`WM_VSCROLL` / `WM_HSCROLL`) for native stability.
* **Accidental Click Protection:** Tapping `Ctrl + Right-Click` within the deadzone seamlessly passes through as a standard `Ctrl + Right-Click` action.

---

## ⚠️ Compatibility & Limitations

* **Microsoft Store & Modern UWP Apps (Not Supported):** Modern packaged applications installed from the Microsoft Store (such as Calculator, Weather, Microsoft To Do) as well as Windows system surfaces (Start menu, Action Center, Windows Settings) handle input through isolated modern pointer pipelines. Because they bypass classic Win32 mouse messages, the drag gesture cannot be detected over these windows.
* **Windows File Explorer Folder Views:** Modern Windows 11 File Explorer views scroll using standard wheel notches configured by system mouse settings (*Settings > Bluetooth & devices > Mouse > Lines to scroll at a time*), rather than fractional micro-scrolling.
* **Portable Windhawk & Elevation:** The mod works in elevated applications when using the standard Windhawk service. If you are using portable Windhawk without running it as Administrator, elevated application windows cannot be targeted.
* **Older Desktop Utilities (Setting Adjustment):** While modern browsers and code editors glide effortlessly with micro-scrolling, some legacy Win32 programs may stutter when receiving fractional wheel deltas. If scrolling feels jumpy in an older program, disable **Ultra-Smooth Scrolling** in settings to use standard wheel notches instead.
* **3D & Creative Suites (Custom Shortcuts):** Software with native `Ctrl + Right-Click` dragging shortcuts (such as Blender, AutoCAD, or video games) will conflict with this gesture. You can exclude those specific programs under the mod's **Advanced** tab in Windhawk.

---

### 🧩 More from this author
* **[Explorer Custom Shortcuts](https://windhawk.net/mods/explorer-custom-shortcuts)** – Customizable keyboard shortcuts for File Explorer: open terminal, copy paths, create files/folders, toggle hidden items, and launch apps with hotkeys.

### Author & Attribution
* **Created and maintained by ArvindSaini978 ([GitHub](https://github.com/ArvindSaini978)).**
