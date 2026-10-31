// ==WindhawkMod==
// @id              test-mod
// @name            Test Mod Sample
// @description     A temporary mod to test docs automation.
// @version         1.0.0
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Test Mod Sample

This documentation text should be extracted into `mods/test-mod.md`.

* Feature 1
* Feature 2
*/
// ==/WindhawkModReadme==

#include <windows.h>

// This C++ code should NOT appear in the documentation or table
BOOL Wh_ModInit() {
    int secret_variable = 42;
    return TRUE;
}

void Wh_ModUninit() {
    // cleanup
}
