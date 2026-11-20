// ============================================================================
// TUMBAL — entry point.
//   tumbal.exe            → game (GUI raylib)
//   tumbal.exe --console  → mode console (logika & struktur data yang sama)
//   tumbal.exe --smoke    → buka jendela sebentar lalu keluar (cek build)
//   tumbal.exe --shots    → ambil screenshot otomatis beberapa scene
//   tumbal.exe --balance  → simulasi battle otomatis untuk menyetel kesulitan
// ============================================================================

#include <cstring>
#include "console/Balance.hpp"
#include "console/ConsoleGame.hpp"
#include "gui/App.hpp"

int main(int argc, char** argv) {
#ifdef TUMBAL_CONSOLE_DEFAULT
    // TUMBAL-console.exe (rilis): langsung mode console tanpa perlu argumen
    if (argc < 2) return runConsole(argv[0]);
#endif
    bool smoke = false, shots = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--console") == 0) return runConsole(argv[0]);
        if (std::strcmp(argv[i], "--balance") == 0) return runBalance();
        if (std::strcmp(argv[i], "--smoke") == 0) smoke = true;
        if (std::strcmp(argv[i], "--shots") == 0) shots = true;
    }
    App app;
    return app.run(smoke, shots);
}
