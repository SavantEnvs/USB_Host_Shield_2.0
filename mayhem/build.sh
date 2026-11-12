#!/usr/bin/env bash
#
# mayhem/build.sh — build the fuzz harnesses (sanitized + standalone reproducers) and the known-answer
# probe that mayhem/test.sh checks. Runs inside the commit image as `mayhem` in /mayhem, offline.
#
# USB Host Shield 2.0 is an Arduino library. It is compiled here on a PC against the tiny Arduino API
# shim in mayhem/shim (the library's own generic-board path, selected with -DESP32): no hardware is
# touched, only the descriptor and report parsers and the drivers' endpoint extractors run.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

TARGETS="fuzz_config_desc fuzz_report_desc fuzz_gamepad_report fuzz_boot_report"
LIB_CPP="$(ls "$SRC"/*.cpp | sort)"
ARDUINO_FLAGS="-std=gnu++17 -DESP32 -DARDUINO=10819 -I$SRC/mayhem/shim -I$SRC -w"

# `build/` is gitignored upstream; everything generated below is recreated on every run.
OBJ_FUZZ="$SRC/build/mayhem-fuzz"
OBJ_PROBE="$SRC/build/mayhem-probe"
rm -rf "$OBJ_FUZZ" "$OBJ_PROBE"
mkdir -p "$OBJ_FUZZ" "$OBJ_PROBE"

compile_all() { # <objdir> <flags...>
  local dir="$1"; shift
  printf '%s\n' $LIB_CPP "$SRC/mayhem/shim/shim.cpp" | xargs -P "$MAYHEM_JOBS" -I{} sh -c \
    '"$0" "$@" -c "{}" -o "'"$dir"'/$(basename "{}").o"' "$CXX" "$@"
}

# 1) The library, sanitized, with SanCov on the library code too (fuzzer-no-link) and DWARF <= 3.
FUZZ_LIB_FLAGS="$ARDUINO_FLAGS -O1 $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link"
compile_all "$OBJ_FUZZ" $FUZZ_LIB_FLAGS
LIB_OBJS="$(ls "$OBJ_FUZZ"/*.o)"

$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$OBJ_FUZZ/lsan_off.o"

# 2) One fuzz binary + one standalone (run-once, no libFuzzer runtime) reproducer per harness.
# The run-once driver is C: compile it as C so its LLVMFuzzerTestOneInput reference keeps C linkage.
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OBJ_FUZZ/standalone_main.o"
for t in $TARGETS; do
  $CXX $FUZZ_LIB_FLAGS "$SRC/mayhem/harness/$t.cpp" $LIB_OBJS "$OBJ_FUZZ/lsan_off.o" \
    $LIB_FUZZING_ENGINE -o "/mayhem/$t"
  $CXX $FUZZ_LIB_FLAGS "$SRC/mayhem/harness/$t.cpp" $LIB_OBJS "$OBJ_FUZZ/lsan_off.o" \
    "$OBJ_FUZZ/standalone_main.o" -o "/mayhem/$t-standalone"
done

# 3) The known-answer probe for mayhem/test.sh: the project's NORMAL flags (no sanitizers, no SanCov),
# dynamically linked so the oracle's sabotage check can neuter it.
compile_all "$OBJ_PROBE" $ARDUINO_FLAGS -O1 $COVERAGE_FLAGS
$CXX $ARDUINO_FLAGS -O1 $COVERAGE_FLAGS "$SRC/mayhem/probe/kat_probe.cpp" "$OBJ_PROBE"/*.o -o "$SRC/build/mayhem-kat-probe"
file "$SRC/build/mayhem-kat-probe" | grep -q 'dynamically linked'
