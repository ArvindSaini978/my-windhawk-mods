// ==WindhawkMod==
// @id              winrar-tweaks
// @name            WinRAR Tweaks
// @description     Auto-dismiss the trial reminder popup, convert file sizes into readable units (KB, MB, GB), and quickly close WinRAR with the Esc key.
// @version         1.1.0
// @author          ArvindSaini978
// @github          https://github.com/ArvindSaini978
// @homepage        https://github.com/ArvindSaini978/my-windhawk-mods
// @license         MIT
// @include         WinRAR.exe
// @compilerOptions -lcomctl32 -luser32
// ==/WindhawkMod==

// clang-format off

// ==WindhawkModReadme==
/*
# WinRAR Tweaks

Simple quality-of-life improvements that make **WinRAR** cleaner, easier to read, and faster to use—without changing how your archives work.

---

## ✨ Features

* 🚫 **Auto-Dismiss Trial Popup:** Automatically blocks the *"Please purchase WinRAR license"* and *"Trial Expired"* popups on startup before they even appear on screen.
* 📊 **Readable File Sizes (All Languages):** Converts long, hard-to-read byte numbers (like `121,815,400`) in the **Size** and **Packed** columns into clean units (`B`, `KB`, `MB`, `GB`, `TB`). Works automatically in all WinRAR display languages.
* 🏷️ **Optional `KiB` / `MiB` Labels:** Prefer technical binary units? Flip a single switch in Settings to display `KiB`, `MiB`, `GiB`, and `TiB` instead of `KB`, `MB`, `GB`, and `TB`.
* ⌨️ **Smart Close on `Esc`:** Press `Escape` to quickly close WinRAR just like modern apps. Smart enough to close open popups, menus, dropdowns, or file-rename boxes first before closing the main window.
* ⚙️ **Customizable Settings:** Enable or disable any feature independently at any time in the Settings tab.

---

## 🖼️ Previews

### Human-Readable File Sizes
![WinRAR File Size Before and After Preview](https://raw.githubusercontent.com/ArvindSaini978/my-windhawk-mods/master/assets/wt_preview_1.png)

### Smart Close on Esc
![WinRAR Close on Esc Demo Webp](https://raw.githubusercontent.com/ArvindSaini978/my-windhawk-mods/master/assets/wt_preview_2.webp)

---

## 🛠️ How It Works

* **Instant Popup Blocking:** Detects WinRAR's startup trial reminder in the background and dismisses it before the window is drawn, so you never see a flicker. All other WinRAR dialogs (Extract, Add, Password, Info, etc.) work normally.
* **Smart Size Formatting:** Formats raw byte numbers on the fly as you browse folders inside an archive, supporting every language automatically.
* **Safe `Esc` Handling:** Checks what you are doing before closing—so pressing `Esc` while renaming a file, dragging items, or browsing a right-click menu won't accidentally close your window.

---

## 💡 Good to Know

* **Sorting Still Works Perfectly:** Clicking the **Size** or **Packed** column headers still sorts your files by their exact byte size.
* **Toggling Settings Live:** Switching between `KB` and `KiB` updates open WinRAR windows right away. If you turn off *Human-Readable File Sizes* completely while WinRAR is open, press **`F5`** (or open another folder) to switch the numbers back to raw bytes.

---

### 🧩 More from this author
* **[Drag to Scroll](https://windhawk.net/mods/drag-to-scroll)** – Hold `Ctrl + Right-Click` and drag to scroll smoothly across desktop applications.
* **[Explorer Custom Shortcuts](https://windhawk.net/mods/explorer-custom-shortcuts)** – Customizable keyboard shortcuts for File Explorer: open terminal, copy paths, create files/folders, toggle hidden items, and launch apps with hotkeys.

### Author & Attribution
* **Created and maintained by ArvindSaini978 ([GitHub](https://github.com/ArvindSaini978)).**
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- close_on_esc: true
  $name: "Close on Esc"
  $description: "Pressing Escape closes the active WinRAR dialog or main window while respecting menus, dropdowns, and inline text editing."
- readable_file_sizes: true
  $name: "Human-Readable File Sizes"
  $description: "Formats raw byte values in the Size and Packed columns into readable units (KB, MB, GB, TB) across all languages."
- use_iec_terms: false
  $name: "Use IEC terms"
  $description: "Use the International Electrotechnical Commission terms, such as KiB instead of KB."
- auto_dismiss_eval_dialog: true
  $name: "Auto-Dismiss Trial Popup"
  $description: "Automatically blocks the 'Please purchase WinRAR license' and 'Trial Expired' popups on startup before they appear on screen."
*/
// ==/WindhawkModSettings==

// clang-format on

#include <commctrl.h>
#include <windhawk_utils.h>
#include <climits>
#include <cstdio>
#include <cwchar>
#include <cwctype>
#include <mutex>
#include <unordered_set>
#include <vector>

// Holds runtime configuration state synced with Windhawk settings
struct ModSettings {
    bool close_on_esc = true;
    bool readable_file_sizes = true;
    bool use_iec_terms = false;
    bool auto_dismiss_eval_dialog = true;
};

static ModSettings g_settings;
static std::mutex g_listViewMutex;
static std::unordered_set<HWND> g_subclassedListViews;

// Loads user preferences from Windhawk storage into memory
static void LoadSettings() {
    g_settings.close_on_esc = Wh_GetIntSetting(L"close_on_esc") != 0;
    g_settings.readable_file_sizes =
        Wh_GetIntSetting(L"readable_file_sizes") != 0;
    g_settings.use_iec_terms = Wh_GetIntSetting(L"use_iec_terms") != 0;
    g_settings.auto_dismiss_eval_dialog =
        Wh_GetIntSetting(L"auto_dismiss_eval_dialog") != 0;
}

// Performs a case-insensitive substring search on wide strings
static bool ContainsCaseInsensitive(const WCHAR* haystack,
                                    const WCHAR* needle) {
    if (!haystack || !needle || !*needle)
        return false;

    size_t needleLen = wcslen(needle);
    for (const WCHAR* p = haystack; *p; ++p) {
        if (_wcsnicmp(p, needle, needleLen) == 0)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// 1. Smart Esc-to-Close Logic
// ---------------------------------------------------------------------------

// Checks if keyboard focus is currently inside a text input or combobox
static bool IsFocusInEditableControl(HWND hFocus) {
    if (!hFocus)
        return false;

    WCHAR cls[64] = {};
    if (!GetClassNameW(hFocus, cls, ARRAYSIZE(cls)))
        return false;

    return (_wcsicmp(cls, L"Edit") == 0 || _wcsicmp(cls, L"ComboBox") == 0 ||
            _wcsicmp(cls, L"ComboBoxEx32") == 0 ||
            wcsstr(cls, L"RichEdit") != nullptr ||
            wcsstr(cls, L"RICHEDIT") != nullptr);
}

// Intercepts Escape keydown to close the active WinRAR dialog or main window
static bool ProcessEscapeKey(MSG* pMsg) {
    if (!g_settings.close_on_esc || !pMsg)
        return false;

    if (pMsg->message != WM_KEYDOWN || pMsg->wParam != VK_ESCAPE)
        return false;

    // Ignore auto-repeat keydown events when Escape is held down
    if ((pMsg->lParam & (1LL << 30)) != 0)
        return false;

    GUITHREADINFO gti = {};
    gti.cbSize = sizeof(gti);
    if (GetGUIThreadInfo(GetCurrentThreadId(), &gti)) {
        constexpr DWORD kModalFlags = GUI_INMENUMODE | GUI_POPUPMENUMODE |
                                      GUI_SYSTEMMENUMODE | GUI_INMOVESIZE;
        // Do not close the window if a menu, drag, or mouse capture is active
        if ((gti.flags & kModalFlags) != 0 || gti.hwndCapture != nullptr ||
            gti.hwndMenuOwner != nullptr) {
            return false;
        }

        // Do not close the window while the user is renaming a file inline
        if (IsFocusInEditableControl(gti.hwndFocus))
            return false;
    } else if (GetCapture() != nullptr) {
        return false;
    }

    HWND hActive = gti.hwndActive ? gti.hwndActive : GetActiveWindow();
    if (!hActive)
        return false;

    DWORD pid = 0;
    GetWindowThreadProcessId(hActive, &pid);
    if (pid != GetCurrentProcessId())
        return false;

    HWND hTarget = GetAncestor(hActive, GA_ROOT);
    if (!hTarget)
        hTarget = hActive;

    PostMessageW(hTarget, WM_CLOSE, 0, 0);
    pMsg->message = WM_NULL;
    return true;
}

using GetMessageW_t = decltype(&GetMessageW);
static GetMessageW_t pfnGetMessageW_Original = nullptr;

// Hooks GetMessageW to process Escape key presses on the main message loop
BOOL WINAPI GetMessageW_Hook(LPMSG lpMsg,
                             HWND hWnd,
                             UINT wMsgFilterMin,
                             UINT wMsgFilterMax) {
    BOOL res =
        pfnGetMessageW_Original(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    if (res > 0 && lpMsg) {
        ProcessEscapeKey(lpMsg);
    }
    return res;
}

using PeekMessageW_t = decltype(&PeekMessageW);
static PeekMessageW_t pfnPeekMessageW_Original = nullptr;

// Hooks PeekMessageW to process Escape key presses in modal dialog loops
BOOL WINAPI PeekMessageW_Hook(LPMSG lpMsg,
                              HWND hWnd,
                              UINT wMsgFilterMin,
                              UINT wMsgFilterMax,
                              UINT wRemoveMsg) {
    BOOL res = pfnPeekMessageW_Original(lpMsg, hWnd, wMsgFilterMin,
                                        wMsgFilterMax, wRemoveMsg);
    if (res && lpMsg && (wRemoveMsg & PM_REMOVE)) {
        ProcessEscapeKey(lpMsg);
    }
    return res;
}

// ---------------------------------------------------------------------------
// 2. Human-Readable File Size Logic (Semantic All-Language Detection)
// ---------------------------------------------------------------------------

// Formats a raw byte value into Base-2 (1024) with KB/MB or KiB/MiB units
static void FormatBytesReadable(ULONGLONG bytes,
                                WCHAR* outBuf,
                                size_t outSize) {
    constexpr double KB = 1024.0;
    constexpr double MB = KB * 1024.0;
    constexpr double GB = MB * 1024.0;
    constexpr double TB = GB * 1024.0;

    if (bytes < 1024ULL) {
        swprintf_s(outBuf, outSize, L"%llu B", bytes);
        return;
    }

    const WCHAR* unitK = g_settings.use_iec_terms ? L"KiB" : L"KB";
    const WCHAR* unitM = g_settings.use_iec_terms ? L"MiB" : L"MB";
    const WCHAR* unitG = g_settings.use_iec_terms ? L"GiB" : L"GB";
    const WCHAR* unitT = g_settings.use_iec_terms ? L"TiB" : L"TB";

    double dBytes = static_cast<double>(bytes);

    if (dBytes >= GB * 1023.995) {
        swprintf_s(outBuf, outSize, L"%.2f %s", dBytes / TB, unitT);
    } else if (dBytes >= MB * 1023.995) {
        swprintf_s(outBuf, outSize, L"%.2f %s", dBytes / GB, unitG);
    } else if (dBytes >= KB * 1023.995) {
        swprintf_s(outBuf, outSize, L"%.2f %s", dBytes / MB, unitM);
    } else {
        swprintf_s(outBuf, outSize, L"%.2f %s", dBytes / KB, unitK);
    }
}

// Converts an already-formatted cell between KB/MB/GB/TB and KiB/MiB/GiB/TiB
static bool ConvertFormattedUnitLabels(const WCHAR* str,
                                       WCHAR* outBuf,
                                       size_t outSize) {
    if (!str || !*str)
        return false;

    const WCHAR* space = wcsrchr(str, L' ');
    if (!space || space == str)
        return false;

    // Ensure the prefix before the unit is strictly a decimal number
    for (const WCHAR* p = str; p < space; ++p) {
        if ((*p < L'0' || *p > L'9') && *p != L'.')
            return false;
    }

    const WCHAR* unit = space + 1;
    const WCHAR* newUnit = nullptr;

    if (g_settings.use_iec_terms) {
        if (wcscmp(unit, L"KB") == 0)
            newUnit = L"KiB";
        else if (wcscmp(unit, L"MB") == 0)
            newUnit = L"MiB";
        else if (wcscmp(unit, L"GB") == 0)
            newUnit = L"GiB";
        else if (wcscmp(unit, L"TB") == 0)
            newUnit = L"TiB";
    } else {
        if (wcscmp(unit, L"KiB") == 0)
            newUnit = L"KB";
        else if (wcscmp(unit, L"MiB") == 0)
            newUnit = L"MB";
        else if (wcscmp(unit, L"GiB") == 0)
            newUnit = L"GB";
        else if (wcscmp(unit, L"TiB") == 0)
            newUnit = L"TB";
    }

    if (!newUnit)
        return false;

    int numLen = static_cast<int>(space - str);
    swprintf_s(outBuf, outSize, L"%.*s %s", numLen, str, newUnit);
    return true;
}

// Checks if the ListView column header is CRC32 to avoid formatting 8-digit hex
static bool IsCrcColumn(HWND hListView, int subItem) {
    if (subItem <= 0)
        return false;

    HWND hHeader = ListView_GetHeader(hListView);
    if (!hHeader)
        return false;

    WCHAR textBuf[64] = {};
    HDITEMW hdi = {};
    hdi.mask = HDI_TEXT;
    hdi.pszText = textBuf;
    hdi.cchTextMax = ARRAYSIZE(textBuf);

    if (!SendMessageW(hHeader, HDM_GETITEMW, static_cast<WPARAM>(subItem),
                      reinterpret_cast<LPARAM>(&hdi))) {
        return false;
    }

    return ContainsCaseInsensitive(textBuf, L"CRC");
}

// Excludes the Name column (subItem 0) and CRC32 column across all languages
static bool IsTargetSizeColumn(HWND hListView, int subItem) {
    if (subItem <= 0)
        return false;

    return !IsCrcColumn(hListView, subItem);
}

// Parses a locale-formatted byte string while rejecting dates, ratios, and CRC
static bool ParseNumericBytes(const WCHAR* str, ULONGLONG* outBytes) {
    if (!str || str == LPSTR_TEXTCALLBACKW || !*str || !outBytes)
        return false;

    ULONGLONG val = 0;
    int digitCount = 0;
    int sepCount = 0;

    for (const WCHAR* p = str; *p; ++p) {
        WCHAR ch = *p;

        // Skip locale thousand separators and invisible formatting marks
        if (iswspace(ch) || ch == L',' || ch == L'.' || ch == L'\'' ||
            ch == 0x00A0 || ch == 0x202F || ch == 0x200E || ch == 0x200F) {
            sepCount++;
            continue;
        }

        if (ch >= L'0' && ch <= L'9') {
            ULONGLONG digit = static_cast<ULONGLONG>(ch - L'0');
            if (val > (ULLONG_MAX - digit) / 10ULL)
                return false;
            val = (val * 10ULL) + digit;
            digitCount++;
        } else {
            return false;
        }
    }

    if (digitCount == 0)
        return false;

    // Reject ungrouped 8-digit numbers or leading-zero numbers (CRC32 hashes)
    if (sepCount == 0 &&
        (digitCount == 8 || (digitCount > 1 && str[0] == L'0'))) {
        return false;
    }

    *outBytes = val;
    return true;
}

// Formats raw byte cells or updates unit labels in an already-open ListView
static void FormatExistingListViewItems(HWND hListView) {
    if (!g_settings.readable_file_sizes || !IsWindow(hListView))
        return;

    if (GetWindowLongPtrW(hListView, GWL_STYLE) & LVS_OWNERDATA)
        return;

    HWND hHeader = ListView_GetHeader(hListView);
    if (!hHeader)
        return;

    int colCount =
        static_cast<int>(SendMessageW(hHeader, HDM_GETITEMCOUNT, 0, 0));
    int itemCount = ListView_GetItemCount(hListView);
    if (colCount <= 1 || itemCount <= 0)
        return;

    for (int subItem = 1; subItem < colCount; ++subItem) {
        if (!IsTargetSizeColumn(hListView, subItem))
            continue;

        for (int i = 0; i < itemCount; ++i) {
            WCHAR rawText[64] = {};
            LVITEMW lvi = {};
            lvi.iSubItem = subItem;
            lvi.pszText = rawText;
            lvi.cchTextMax = ARRAYSIZE(rawText);

            if (SendMessageW(hListView, LVM_GETITEMTEXTW,
                             static_cast<WPARAM>(i),
                             reinterpret_cast<LPARAM>(&lvi)) <= 0) {
                continue;
            }

            WCHAR formatted[64] = {};
            ULONGLONG bytes = 0;
            if (ParseNumericBytes(rawText, &bytes)) {
                FormatBytesReadable(bytes, formatted, ARRAYSIZE(formatted));
                lvi.pszText = formatted;
                SendMessageW(hListView, LVM_SETITEMTEXTW,
                             static_cast<WPARAM>(i),
                             reinterpret_cast<LPARAM>(&lvi));
            } else if (ConvertFormattedUnitLabels(rawText, formatted,
                                                  ARRAYSIZE(formatted))) {
                lvi.pszText = formatted;
                SendMessageW(hListView, LVM_SETITEMTEXTW,
                             static_cast<WPARAM>(i),
                             reinterpret_cast<LPARAM>(&lvi));
            }
        }
    }
}

// Intercepts ListView item insertions and updates to format byte counts inline
static LRESULT CALLBACK ListViewSubclassProc(HWND hWnd,
                                             UINT uMsg,
                                             WPARAM wParam,
                                             LPARAM lParam,
                                             DWORD_PTR /*dwRefData*/) {
    if (g_settings.readable_file_sizes && lParam != 0) {
        if (uMsg == LVM_SETITEMTEXTW || uMsg == LVM_SETITEMW ||
            uMsg == LVM_INSERTITEMW) {
            LVITEMW* pItem = reinterpret_cast<LVITEMW*>(lParam);
            bool hasText =
                (uMsg == LVM_SETITEMTEXTW) || ((pItem->mask & LVIF_TEXT) != 0);

            if (hasText && pItem->iSubItem > 0 && pItem->pszText &&
                pItem->pszText != LPSTR_TEXTCALLBACKW) {
                ULONGLONG byteCount = 0;
                if (ParseNumericBytes(pItem->pszText, &byteCount) &&
                    IsTargetSizeColumn(hWnd, pItem->iSubItem)) {
                    WCHAR szFormatted[64] = {};
                    FormatBytesReadable(byteCount, szFormatted,
                                        ARRAYSIZE(szFormatted));

                    LPWSTR pOld = pItem->pszText;
                    pItem->pszText = szFormatted;
                    LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);
                    pItem->pszText = pOld;
                    return res;
                }
            }
        }
    }

    // Remove the window handle from the tracking set when it is destroyed
    if (uMsg == WM_NCDESTROY) {
        std::lock_guard<std::mutex> lock(g_listViewMutex);
        g_subclassedListViews.erase(hWnd);
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// Attaches our subclass procedure to a SysListView32 control
static void AttachListViewSubclass(HWND hWnd, bool formatExisting) {
    if (!hWnd || !IsWindow(hWnd))
        return;

    WCHAR cls[64] = {};
    if (!GetClassNameW(hWnd, cls, ARRAYSIZE(cls)) ||
        _wcsicmp(cls, WC_LISTVIEWW) != 0) {
        return;
    }

    bool alreadySubclassed = false;
    {
        std::lock_guard<std::mutex> lock(g_listViewMutex);
        alreadySubclassed =
            (g_subclassedListViews.find(hWnd) != g_subclassedListViews.end());
    }

    if (!alreadySubclassed) {
        if (WindhawkUtils::SetWindowSubclassFromAnyThread(
                hWnd, ListViewSubclassProc, 0)) {
            std::lock_guard<std::mutex> lock(g_listViewMutex);
            g_subclassedListViews.insert(hWnd);
            alreadySubclassed = true;
        }
    }

    if (alreadySubclassed && formatExisting) {
        FormatExistingListViewItems(hWnd);
    }
}

// ---------------------------------------------------------------------------
// 3. Zero-Flicker Evaluation Popup Blocker
// ---------------------------------------------------------------------------

// Checks if a dialog template name matches WinRAR's trial reminder resources
static bool IsEvaluationTemplateName(LPCWSTR lpTemplateName) {
    if (!lpTemplateName || IS_INTRESOURCE(lpTemplateName))
        return false;

    return (_wcsicmp(lpTemplateName, L"REMINDER") == 0 ||
            _wcsicmp(lpTemplateName, L"RARREMINDER") == 0 ||
            _wcsicmp(lpTemplateName, L"WEBREMINDER") == 0 ||
            _wcsicmp(lpTemplateName, L"RARWEBREMINDER") == 0);
}

// Matches only the classic license dialog title or the web expired popup title
static bool IsEvaluationWindowTitle(const WCHAR* title) {
    if (!title || !*title)
        return false;

    if (ContainsCaseInsensitive(title, L"Expired Notification") &&
        ContainsCaseInsensitive(title, L"free trial period has ended")) {
        return true;
    }

    if (_wcsicmp(title, L"Please purchase WinRAR license") == 0) {
        return true;
    }

    return false;
}

// Verifies the trial dialog by checking its child controls or embedded browser
static bool HasEvaluationDialogSignature(HWND hDlg) {
    if (!hDlg || !IsWindow(hDlg))
        return false;

    bool hasBuyOnlineBtn = false;
    bool hasHowToBuyBtn = false;
    bool hasTrialNote = false;

    for (HWND hChild = GetWindow(hDlg, GW_CHILD); hChild != nullptr;
         hChild = GetWindow(hChild, GW_HWNDNEXT)) {
        WCHAR cls[64] = {};
        if (GetClassNameW(hChild, cls, ARRAYSIZE(cls))) {
            // The web-based Trial Expired popup hosts an embedded browser
            // control ("Shell Embedding" / "WebView2") inside a dialog window
            if (_wcsicmp(cls, L"Shell Embedding") == 0 ||
                _wcsicmp(cls, L"Internet Explorer_Server") == 0 ||
                ContainsCaseInsensitive(cls, L"WebView")) {
                return true;
            }
        }

        WCHAR text[256] = {};
        if (GetWindowTextW(hChild, text, ARRAYSIZE(text)) <= 0)
            continue;

        if (_wcsicmp(text, L"Buy online") == 0) {
            hasBuyOnlineBtn = true;
        } else if (_wcsicmp(text, L"How to buy") == 0) {
            hasHowToBuyBtn = true;
        } else if (ContainsCaseInsensitive(text, L"40 day trial period") ||
                   ContainsCaseInsensitive(text,
                                           L"WinRAR is not free software")) {
            hasTrialNote = true;
        }
    }

    return (hasBuyOnlineBtn && hasHowToBuyBtn) ||
           (hasBuyOnlineBtn && hasTrialNote);
}

// Determines if a top-level window is one of the two evaluation popups
static bool IsEvaluationPopupWindow(HWND hWnd) {
    if (!hWnd || !IsWindow(hWnd))
        return false;

    // Never match child controls such as buttons or static text labels
    if ((GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_CHILD) != 0)
        return false;

    WCHAR title[256] = {};
    if (GetWindowTextW(hWnd, title, ARRAYSIZE(title)) > 0) {
        if (IsEvaluationWindowTitle(title))
            return true;
    }

    WCHAR cls[64] = {};
    if (GetClassNameW(hWnd, cls, ARRAYSIZE(cls))) {
        if (_wcsicmp(cls, L"RarReminder") == 0 ||
            _wcsicmp(cls, L"RarWebReminder") == 0) {
            return true;
        }

        if (_wcsicmp(cls, L"#32770") == 0 &&
            HasEvaluationDialogSignature(hWnd)) {
            return true;
        }
    }

    return false;
}

// Checks whether a window creation call is creating the web reminder host
// (which starts with the title "WinRAR" over an already-open main window)
static bool IsWebReminderCreation(LPCWSTR lpClassName,
                                  LPCWSTR lpWindowName,
                                  DWORD dwStyle,
                                  HWND hWndParent) {
    if ((dwStyle & WS_CHILD) != 0)
        return false;

    WCHAR clsName[128] = {};
    if (lpClassName) {
        if (IS_INTRESOURCE(lpClassName)) {
            GlobalGetAtomNameW(LOWORD(reinterpret_cast<ULONG_PTR>(lpClassName)),
                               clsName, ARRAYSIZE(clsName));
        } else {
            wcsncpy_s(clsName, lpClassName, _TRUNCATE);
        }
    }

    // Direct match on WinRAR's reminder window class
    if (_wcsicmp(clsName, L"RarReminder") == 0 ||
        _wcsicmp(clsName, L"RarWebReminder") == 0) {
        return true;
    }

    // When WinRAR spawns the promotional web window, it creates a popup/owned
    // window with the initial title "WinRAR" parented to the main WinRAR window
    if (hWndParent != nullptr && lpWindowName != nullptr &&
        _wcsicmp(lpWindowName, L"WinRAR") == 0) {
        return true;
    }

    return false;
}

// Thread-local pointer to pass the original DLGPROC into our wrapper
static thread_local DLGPROC g_tlsOriginalDlgProc = nullptr;

// Intercepts WM_INITDIALOG before Windows makes the modal dialog visible
static INT_PTR CALLBACK ModalDialogInterceptorProc(HWND hDlg,
                                                   UINT uMsg,
                                                   WPARAM wParam,
                                                   LPARAM lParam) {
    DLGPROC origProc =
        reinterpret_cast<DLGPROC>(GetPropW(hDlg, L"WhOrigDlgProc"));
    if (!origProc && g_tlsOriginalDlgProc) {
        origProc = g_tlsOriginalDlgProc;
        SetPropW(hDlg, L"WhOrigDlgProc", reinterpret_cast<HANDLE>(origProc));
    }

    INT_PTR res = origProc ? origProc(hDlg, uMsg, wParam, lParam) : FALSE;

    if (uMsg == WM_INITDIALOG && g_settings.auto_dismiss_eval_dialog) {
        // After WinRAR's WM_INITDIALOG finishes populating child controls and
        // title text (but BEFORE Windows shows the dialog on screen), check if
        // this dialog is the evaluation reminder and abort it immediately.
        if (IsEvaluationPopupWindow(hDlg)) {
            Wh_Log(L"Aborted evaluation dialog inside WM_INITDIALOG");
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
    } else if (uMsg == WM_NCDESTROY) {
        RemovePropW(hDlg, L"WhOrigDlgProc");
    }

    return res;
}

using DialogBoxParamW_t = decltype(&DialogBoxParamW);
static DialogBoxParamW_t pfnDialogBoxParamW_Original = nullptr;

// Hooks modal dialog creation to block evaluation dialogs before they render
INT_PTR WINAPI DialogBoxParamW_Hook(HINSTANCE hInstance,
                                    LPCWSTR lpTemplateName,
                                    HWND hWndParent,
                                    DLGPROC lpDialogFunc,
                                    LPARAM dwInitParam) {
    if (g_settings.auto_dismiss_eval_dialog) {
        if (IsEvaluationTemplateName(lpTemplateName)) {
            Wh_Log(L"Blocked modal evaluation dialog template: %s",
                   lpTemplateName);
            return IDCANCEL;
        }

        if (lpDialogFunc) {
            DLGPROC prevProc = g_tlsOriginalDlgProc;
            g_tlsOriginalDlgProc = lpDialogFunc;
            INT_PTR ret = pfnDialogBoxParamW_Original(
                hInstance, lpTemplateName, hWndParent,
                ModalDialogInterceptorProc, dwInitParam);
            g_tlsOriginalDlgProc = prevProc;
            return ret;
        }
    }

    return pfnDialogBoxParamW_Original(hInstance, lpTemplateName, hWndParent,
                                       lpDialogFunc, dwInitParam);
}

using CreateDialogParamW_t = decltype(&CreateDialogParamW);
static CreateDialogParamW_t pfnCreateDialogParamW_Original = nullptr;

// Blocks modeless evaluation dialogs before they can be shown
HWND WINAPI CreateDialogParamW_Hook(HINSTANCE hInstance,
                                    LPCWSTR lpTemplateName,
                                    HWND hWndParent,
                                    DLGPROC lpDialogFunc,
                                    LPARAM dwInitParam) {
    if (g_settings.auto_dismiss_eval_dialog &&
        IsEvaluationTemplateName(lpTemplateName)) {
        Wh_Log(L"Blocked modeless evaluation dialog template: %s",
               lpTemplateName);
        return nullptr;
    }

    HWND hDlg = pfnCreateDialogParamW_Original(
        hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);

    if (hDlg && g_settings.auto_dismiss_eval_dialog &&
        IsEvaluationPopupWindow(hDlg)) {
        Wh_Log(L"Destroying evaluation dialog before render");
        DestroyWindow(hDlg);
        return nullptr;
    }

    return hDlg;
}

using SetWindowTextW_t = decltype(&SetWindowTextW);
static SetWindowTextW_t pfnSetWindowTextW_Original = nullptr;

// Catches evaluation dialogs when WinRAR sets their title dynamically
BOOL WINAPI SetWindowTextW_Hook(HWND hWnd, LPCWSTR lpString) {
    BOOL res = pfnSetWindowTextW_Original(hWnd, lpString);

    if (g_settings.auto_dismiss_eval_dialog && hWnd && lpString &&
        (GetWindowLongPtrW(hWnd, GWL_STYLE) & WS_CHILD) == 0 &&
        IsEvaluationWindowTitle(lpString)) {
        Wh_Log(L"Dismissing evaluation popup on title set: %s", lpString);
        if (!EndDialog(hWnd, IDCANCEL)) {
            DestroyWindow(hWnd);
        }
    }

    return res;
}

using ShowWindow_t = decltype(&ShowWindow);
static ShowWindow_t pfnShowWindow_Original = nullptr;

// Prevents any remaining evaluation popup from becoming visible on screen
BOOL WINAPI ShowWindow_Hook(HWND hWnd, int nCmdShow) {
    if (g_settings.auto_dismiss_eval_dialog && nCmdShow != SW_HIDE && hWnd &&
        IsEvaluationPopupWindow(hWnd)) {
        Wh_Log(L"Prevented evaluation popup from showing");
        if (!EndDialog(hWnd, IDCANCEL)) {
            PostMessageW(hWnd, WM_CLOSE, 0, 0);
        }
        return FALSE;
    }

    return pfnShowWindow_Original(hWnd, nCmdShow);
}

// ---------------------------------------------------------------------------
// 4. Window Creation Hook & Windhawk Lifecycle
// ---------------------------------------------------------------------------

using CreateWindowExW_t = decltype(&CreateWindowExW);
static CreateWindowExW_t pfnCreateWindowExW_Original = nullptr;

// Hooks window creation to block evaluation windows and subclass new ListViews
HWND WINAPI CreateWindowExW_Hook(DWORD dwExStyle,
                                 LPCWSTR lpClassName,
                                 LPCWSTR lpWindowName,
                                 DWORD dwStyle,
                                 int X,
                                 int Y,
                                 int nWidth,
                                 int nHeight,
                                 HWND hWndParent,
                                 HMENU hMenu,
                                 HINSTANCE hInstance,
                                 LPVOID lpParam) {
    if (g_settings.auto_dismiss_eval_dialog && !(dwStyle & WS_CHILD)) {
        if ((lpWindowName && IsEvaluationWindowTitle(lpWindowName)) ||
            IsWebReminderCreation(lpClassName, lpWindowName, dwStyle,
                                  hWndParent)) {
            Wh_Log(L"Blocked evaluation window creation in CreateWindowExW");
            return nullptr;
        }
    }

    HWND hWnd = pfnCreateWindowExW_Original(
        dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight,
        hWndParent, hMenu, hInstance, lpParam);

    if (hWnd) {
        // If an embedded browser control ("Shell Embedding" / WebView) is
        // created inside a dialog popup, hide and close the parent popup
        // immediately before it paints on screen.
        if (g_settings.auto_dismiss_eval_dialog && (dwStyle & WS_CHILD) &&
            hWndParent) {
            WCHAR cls[64] = {};
            if (GetClassNameW(hWnd, cls, ARRAYSIZE(cls)) &&
                (_wcsicmp(cls, L"Shell Embedding") == 0 ||
                 ContainsCaseInsensitive(cls, L"WebView"))) {
                HWND hRoot = GetAncestor(hWndParent, GA_ROOT);
                if (hRoot && GetMenu(hRoot) == nullptr) {
                    Wh_Log(L"Aborted web reminder host on browser creation");
                    ShowWindow(hRoot, SW_HIDE);
                    if (!EndDialog(hRoot, IDCANCEL)) {
                        PostMessageW(hRoot, WM_CLOSE, 0, 0);
                    }
                }
            }
        }

        AttachListViewSubclass(hWnd, false);
    }

    return hWnd;
}

// Enumerates child windows to find and subclass existing SysListView32 controls
static BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM /*lParam*/) {
    AttachListViewSubclass(hWnd, true);
    return TRUE;
}

// Enumerates top-level windows belonging to the current WinRAR process
static BOOL CALLBACK EnumTopWindowsProc(HWND hWnd, LPARAM /*lParam*/) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if (pid == GetCurrentProcessId()) {
        if (g_settings.auto_dismiss_eval_dialog &&
            IsEvaluationPopupWindow(hWnd)) {
            if (!EndDialog(hWnd, IDCANCEL)) {
                PostMessageW(hWnd, WM_CLOSE, 0, 0);
            }
            return TRUE;
        }

        AttachListViewSubclass(hWnd, true);
        EnumChildWindows(hWnd, EnumChildProc, 0);
    }
    return TRUE;
}

// Initializes the mod and installs Win32 API hooks
BOOL Wh_ModInit() {
    Wh_Log(L"Initializing WinRAR Tweaks");
    LoadSettings();

    WindhawkUtils::SetFunctionHook(GetMessageW, GetMessageW_Hook,
                                   &pfnGetMessageW_Original);
    WindhawkUtils::SetFunctionHook(PeekMessageW, PeekMessageW_Hook,
                                   &pfnPeekMessageW_Original);
    WindhawkUtils::SetFunctionHook(CreateWindowExW, CreateWindowExW_Hook,
                                   &pfnCreateWindowExW_Original);
    WindhawkUtils::SetFunctionHook(DialogBoxParamW, DialogBoxParamW_Hook,
                                   &pfnDialogBoxParamW_Original);
    WindhawkUtils::SetFunctionHook(CreateDialogParamW, CreateDialogParamW_Hook,
                                   &pfnCreateDialogParamW_Original);
    WindhawkUtils::SetFunctionHook(SetWindowTextW, SetWindowTextW_Hook,
                                   &pfnSetWindowTextW_Original);
    WindhawkUtils::SetFunctionHook(ShowWindow, ShowWindow_Hook,
                                   &pfnShowWindow_Original);

    return TRUE;
}

// Processes any WinRAR windows that were already open before mod initialization
void Wh_ModAfterInit() {
    EnumWindows(EnumTopWindowsProc, 0);
}

// Reloads settings and refreshes open windows when preferences change
void Wh_ModSettingsChanged() {
    LoadSettings();
    if (g_settings.readable_file_sizes || g_settings.auto_dismiss_eval_dialog) {
        EnumWindows(EnumTopWindowsProc, 0);
    }
}

// Safely removes all window subclasses when the mod is unloaded
void Wh_ModUninit() {
    Wh_Log(L"Unloading WinRAR Tweaks");

    std::vector<HWND> listViews;
    {
        std::lock_guard<std::mutex> lock(g_listViewMutex);
        listViews.assign(g_subclassedListViews.begin(),
                         g_subclassedListViews.end());
        g_subclassedListViews.clear();
    }

    for (HWND hWnd : listViews) {
        if (IsWindow(hWnd)) {
            WindhawkUtils::RemoveWindowSubclassFromAnyThread(
                hWnd, ListViewSubclassProc);
        }
    }
}
