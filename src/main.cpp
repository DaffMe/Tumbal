// ============================================================================
// Cinderwake — entry point.
//   cinderwake.exe            → game (GUI raylib)
//   cinderwake.exe --console  → mode console (logika & struktur data yang sama)
//   cinderwake.exe --smoke    → buka jendela sebentar lalu keluar (cek build)
//   cinderwake.exe --shots    → ambil screenshot otomatis beberapa scene
//   cinderwake.exe --gallery  → tampilkan & screenshot semua aset
// ============================================================================

#include <cstring>
#include "console/ConsoleGame.hpp"
#include "gui/App.hpp"
#include "gui/Gallery.hpp"

int main(int argc, char** argv) {
    bool smoke = false, shots = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--console") == 0) return runConsole();
        if (std::strcmp(argv[i], "--gallery") == 0) return runGallery();
        if (std::strcmp(argv[i], "--smoke") == 0) smoke = true;
        if (std::strcmp(argv[i], "--shots") == 0) shots = true;
    }
    App app;
    return app.run(smoke, shots);
}
