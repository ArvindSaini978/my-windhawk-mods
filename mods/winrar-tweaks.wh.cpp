// ==WindhawkMod==
// @id              winrar-tweaks
// @name            WinRAR Tweaks (Close on Esc & Dynamic Size)
// @description     Closes WinRAR on Esc (modal-aware) and converts file sizes in Size and Packed columns to KB, MB, GB, TB.
// @version         1.0.0
// @author          Custom
// @include         WinRAR.exe
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
All-in-one quality-of-life tweaks for WinRAR:
1. Pressing Escape closes WinRAR, respecting open dialogs, dropdowns, and menus.
2. In-place conversion of raw bytes to human-readable units (B, KB, MB, GB, TB) for Size and Packed columns.
*/
// ==/WindhawkModReadme==

#include <windows.h>
#include <commctrl.h>
#include <cwctype>
#include <cstdio>

// ---------------------------------------------------------------------------
// 1. Esc-to-Close State & Logic
// ---------------------------------------------------------------------------

static HHOOK g_hKeyboardHook = NULL;
static bool g_inMenuLoop = false;

using PeekMessageW_t = decltype(&PeekMessageW);
PeekMessageW_t pOriginalPeekMessageW = NULL;
BOOL WINAPI Hook_PeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
    BOOL res = pOriginalPeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    if (res && lpMsg) {
        if (lpMsg->message == WM_ENTERMENULOOP) {
            g_inMenuLoop = true;
        } else if (lpMsg->message == WM_EXITMENULOOP) {
            g_inMenuLoop = false;
        }
    }
    return res;
}

using GetMessageW_t = decltype(&GetMessageW);
GetMessageW_t pOriginalGetMessageW = NULL;
BOOL WINAPI Hook_GetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
    BOOL res = pOriginalGetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    if (res > 0 && lpMsg) {
        if (lpMsg->message == WM_ENTERMENULOOP) {
            g_inMenuLoop = true;
        } else if (lpMsg->message == WM_EXITMENULOOP) {
            g_inMenuLoop = false;
        }
    }
    return res;
}

static LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0 && wParam == PM_REMOVE) {
        MSG* pMsg = reinterpret_cast<MSG*>(lParam);

        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) {
            // Respect active popup menus or combo box dropdowns
            if (g_inMenuLoop || (GetCapture() != NULL)) {
                return CallNextHookEx(g_hKeyboardHook, code, wParam, lParam);
            }

            HWND hActive = GetActiveWindow();
            if (!hActive) {
                hActive = GetForegroundWindow();
            }

            if (hActive) {
                HWND hOwner = GetWindow(hActive, GW_OWNER);
                HWND hMain = GetAncestor(hActive, GA_ROOT);

                // If a dialog window is active, close only that dialog
                if (hOwner != NULL && hActive != hMain) {
                    PostMessage(hActive, WM_CLOSE, 0, 0);
                    pMsg->message = WM_NULL;
                    return 0;
                }

                // If an owned popup dialog exists, close it first
                HWND hPopup = GetLastActivePopup(hMain);
                if (hPopup && hPopup != hMain && IsWindowVisible(hPopup)) {
                    PostMessage(hPopup, WM_CLOSE, 0, 0);
                    pMsg->message = WM_NULL;
                    return 0;
                }

                // Close main WinRAR window
                PostMessage(hMain, WM_CLOSE, 0, 0);
                pMsg->message = WM_NULL;
                return 0;
            }
        }
    }
    return CallNextHookEx(g_hKeyboardHook, code, wParam, lParam);
}

// ---------------------------------------------------------------------------
// 2. In-Place Dynamic Size Logic
// ---------------------------------------------------------------------------

static HWND g_hListView = NULL;

static void FormatBytesReadable(ULONGLONG bytes, WCHAR* outBuf, size_t outSize) {
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    const double TB = GB * 1024.0;

    double dBytes = static_cast<double>(bytes);

    if (dBytes >= TB) {
        swprintf_s(outBuf, outSize, L"%.2f TB", dBytes / TB);
    } else if (dBytes >= GB) {
        swprintf_s(outBuf, outSize, L"%.2f GB", dBytes / GB);
    } else if (dBytes >= MB) {
        swprintf_s(outBuf, outSize, L"%.2f MB", dBytes / MB);
    } else if (dBytes >= KB) {
        swprintf_s(outBuf, outSize, L"%.2f KB", dBytes / KB);
    } else {
        swprintf_s(outBuf, outSize, L"%llu B", bytes);
    }
}

static bool ParseNumericBytes(const WCHAR* str, ULONGLONG* outBytes) {
    if (!str || !*str) return false;

    for (const WCHAR* p = str; *p; ++p) {
        if (*p == L':' || *p == L'-' || *p == L'/' || iswalpha(*p)) {
            return false;
        }
    }

    ULONGLONG val = 0;
    bool hasDigits = false;

    for (const WCHAR* p = str; *p; ++p) {
        if (*p == L' ' || *p == L',' || *p == L'.' || *p == L'\'' || *p == 0x00A0) {
            continue;
        }
        if (iswdigit(*p)) {
            val = (val * 10) + (*p - L'0');
            hasDigits = true;
        } else {
            return false;
        }
    }

    if (hasDigits && outBytes) {
        *outBytes = val;
        return true;
    }
    return false;
}

static bool IsTargetSizeColumn(HWND hListView, int subItem) {
    HWND hHeader = ListView_GetHeader(hListView);
    if (!hHeader) return false;

    WCHAR textBuf[64] = { 0 };
    HDITEMW hdi = { 0 };
    hdi.mask = HDI_TEXT;
    hdi.pszText = textBuf;
    hdi.cchTextMax = ARRAYSIZE(textBuf);

    if (SendMessageW(hHeader, HDM_GETITEMW, (WPARAM)subItem, (LPARAM)&hdi)) {
        if (wcsstr(textBuf, L"Size") != NULL || 
            wcsstr(textBuf, L"Packed") != NULL ||
            wcsstr(textBuf, L"Сжатый") != NULL ||
            wcsstr(textBuf, L"Размер") != NULL) {
            return true;
        }
    }
    return false;
}

static LRESULT CALLBACK ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOrig = (WNDPROC)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    if (uMsg == LVM_SETITEMTEXTW && lParam != 0) {
        LVITEMW* pItem = reinterpret_cast<LVITEMW*>(lParam);

        if (pItem->pszText && IsTargetSizeColumn(hWnd, pItem->iSubItem)) {
            ULONGLONG byteCount = 0;
            if (ParseNumericBytes(pItem->pszText, &byteCount)) {
                WCHAR szFormatted[64] = { 0 };
                FormatBytesReadable(byteCount, szFormatted, ARRAYSIZE(szFormatted));

                LPWSTR pOld = pItem->pszText;
                pItem->pszText = szFormatted;
                LRESULT res = CallWindowProcW(pOrig, hWnd, uMsg, wParam, lParam);
                pItem->pszText = pOld;
                return res;
            }
        }
    }

    if (uMsg == WM_NCDESTROY) {
        SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)pOrig);
    }

    return CallWindowProcW(pOrig, hWnd, uMsg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// 3. Window Hooking & Lifecycle
// ---------------------------------------------------------------------------

using CreateWindowExW_t = decltype(&CreateWindowExW);
CreateWindowExW_t pOriginalCreateWindowExW = NULL;

HWND WINAPI Hook_CreateWindowExW(
    DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
    int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
    HINSTANCE hInstance, LPVOID lpParam
) {
    HWND hWnd = pOriginalCreateWindowExW(
        dwExStyle, lpClassName, lpWindowName, dwStyle,
        X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam
    );

    if (hWnd && lpClassName && !((ULONG_PTR)lpClassName & ~(ULONG_PTR)0xffff)) {
        return hWnd;
    }

    if (hWnd && lpClassName && wcscmp(lpClassName, WC_LISTVIEWW) == 0) {
        g_hListView = hWnd;
        WNDPROC pOrig = (WNDPROC)GetWindowLongPtrW(hWnd, GWLP_WNDPROC);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pOrig);
        SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)ListViewSubclassProc);
    }

    return hWnd;
}

BOOL Wh_ModInit() {
    Wh_Log(L"WinRAR Tweaks (All-In-One) loaded");

    // Message loop hooks for modal menu checks
    Wh_SetFunctionHook((void*)PeekMessageW, (void*)Hook_PeekMessageW, (void**)&pOriginalPeekMessageW);
    Wh_SetFunctionHook((void*)GetMessageW, (void*)Hook_GetMessageW, (void**)&pOriginalGetMessageW);

    // ESC key hook
    g_hKeyboardHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, NULL, GetCurrentThreadId());

    // ListView detection hook
    Wh_SetFunctionHook((void*)CreateWindowExW, (void*)Hook_CreateWindowExW, (void**)&pOriginalCreateWindowExW);

    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"WinRAR Tweaks (All-In-One) unloaded");

    if (g_hKeyboardHook) {
        UnhookWindowsHookEx(g_hKeyboardHook);
        g_hKeyboardHook = NULL;
    }

    if (g_hListView) {
        WNDPROC pOrig = (WNDPROC)GetWindowLongPtrW(g_hListView, GWLP_USERDATA);
        if (pOrig) {
            SetWindowLongPtrW(g_hListView, GWLP_WNDPROC, (LONG_PTR)pOrig);
        }
    }
}
