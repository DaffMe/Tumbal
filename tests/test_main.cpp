// Runner semua unit test. Jalankan: mingw32-make test
#include "TestFramework.hpp"

int main() {
    runAdtTests();
    runWorldTests();
    std::printf("\n%d lulus, %d gagal\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
