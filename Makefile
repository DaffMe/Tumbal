# ============================================================================
# Cinderwake — build dengan MSYS2 UCRT64 g++ + mingw32-make
#
#   mingw32-make            build game  → bin/cinderwake.exe
#   mingw32-make test       build + jalankan unit test
#   mingw32-make run        build + jalankan game
#   mingw32-make clean
# ============================================================================

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc
# raylib MSYS2 di-build dengan GLFW versi DLL → link dinamis, lalu DLL-nya
# disalin ke bin/ agar folder bin/ bisa dijalankan di komputer lain.
LDLIBS   := -lraylib
LDFLAGS  := -static-libgcc -static-libstdc++
DLLS     := bin/libraylib.dll bin/glfw3.dll

# Lokasi folder bin toolchain (tempat DLL raylib berada). `where` tersedia
# baik dari cmd/PowerShell maupun dari bash di Windows.
MSYS_BIN := $(subst \,/,$(dir $(firstword $(shell where g++))))

# mingw32-make memakai cmd.exe jika dijalankan dari PowerShell/cmd, dan sh
# jika dari bash. Trik: cmd mengekspansi %OS%, sh tidak.
ifeq ($(shell echo %OS%),Windows_NT)
  CP = copy /Y "$(subst /,\,$(1))" "$(subst /,\,$(2))" >NUL
  RM = del /Q bin\*.exe bin\*.dll 2>NUL
  RUN_PREFIX :=
else
  CP = cp "$(1)" "$(2)"
  RM = rm -f bin/*.exe bin/*.dll
  RUN_PREFIX := ./
endif

CORE_HDR := $(wildcard src/core/*/*.hpp)
CORE_SRC := $(wildcard src/core/*/*.cpp)
GUI_SRC  := $(wildcard src/gui/*.cpp)
CON_SRC  := $(wildcard src/console/*.cpp)
TEST_SRC := $(wildcard tests/*.cpp)

GAME := bin/cinderwake.exe
TEST := bin/cinderwake_tests.exe

.PHONY: all test run clean

all: $(GAME) $(DLLS)

$(GAME): src/main.cpp $(CORE_SRC) $(GUI_SRC) $(CON_SRC) $(CORE_HDR)
	$(CXX) $(CXXFLAGS) src/main.cpp $(CORE_SRC) $(GUI_SRC) $(CON_SRC) -o $@ $(LDFLAGS) $(LDLIBS)

bin/%.dll:
	$(call CP,$(MSYS_BIN)$*.dll,$@)

# Test hanya memakai core → tidak di-link dengan raylib
$(TEST): $(TEST_SRC) $(CORE_SRC) $(CORE_HDR) tests/TestFramework.hpp
	$(CXX) $(CXXFLAGS) $(TEST_SRC) $(CORE_SRC) -o $@ -static-libgcc -static-libstdc++

test: $(TEST)
	$(RUN_PREFIX)$(subst /,$(if $(RUN_PREFIX),/,\),$(TEST))

run: all
	$(RUN_PREFIX)$(subst /,$(if $(RUN_PREFIX),/,\),$(GAME))

clean:
	$(RM)
