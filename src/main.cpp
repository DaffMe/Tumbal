// ============================================================================
// TUMBAL — entry point.
//   tumbal.exe            → game (GUI raylib)
//   tumbal.exe --console  → mode console (logika & struktur data yang sama)
//   tumbal.exe --smoke    → buka jendela sebentar lalu keluar (cek build)
//   tumbal.exe --shots    → ambil screenshot otomatis beberapa scene
// ============================================================================

#include <cstring>
#include "console/ConsoleGame.hpp"
#include "gui/App.hpp"

int main(int argc, char** argv) {
    bool smoke = false, shots = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--console") == 0) return runConsole();
        if (std::strcmp(argv[i], "--smoke") == 0) smoke = true;
        if (std::strcmp(argv[i], "--shots") == 0) shots = true;
    }
    App app;
    return app.run(smoke, shots);
}
