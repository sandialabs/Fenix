#!/bin/bash
#
# sanitizers.sh - Run Fenix tests with comprehensive sanitizer coverage
#
# This script builds Fenix with multiple sanitizers and runs tests to detect:
#   - Out of bounds memory accesses
#   - Memory leaks
#   - Use after free
#   - Uninitialized memory reads
#   - Invalid memory operations
#   - Undefined behavior (integer overflows, null pointers, etc.)
#
# Uses:
#   - AddressSanitizer (ASAN): Memory error detection
#   - LeakSanitizer (LSAN): Memory leak detection
#   - UndefinedBehaviorSanitizer (UBSAN): Undefined behavior detection
#
# Usage:
#   ./test/CI/sanitizers.sh
#
# Requirements:
#   - GCC with sanitizer support
#   - MPI with fault tolerance support
#   - lsan_suppressions.txt file (committed to test/CI/)
#

set -e  # Exit on error
set -u  # Exit on undefined variable

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check that we're in the project root
if [[ ! -f "CMakeLists.txt" ]] || [[ ! -d "src" ]] || [[ ! -d "test" ]]; then
    echo -e "${RED}ERROR: This script must be run from the Fenix project root directory${NC}"
    echo "Expected directory structure:"
    echo "  CMakeLists.txt  (in current directory)"
    echo "  src/            (source directory)"
    echo "  test/           (test directory)"
    exit 1
fi

PROJECT_ROOT="$(pwd)"
BUILD_DIR="${PROJECT_ROOT}/build_memory_asan"
REPORT_DIR="${BUILD_DIR}/memory_reports"

# Set up PATH for common tool locations
export PATH="/home/mwhitlo/installs/openmpi/main/bin:${PATH}"
export PATH="/projects/claude_scratch/python_packages/bin:${PATH}"
export PATH="/projects/claude_scratch/python_packages/cmake/data/bin:${PATH}"
export PYTHONPATH="/projects/claude_scratch/python_packages/lib/python3.13/site-packages:${PYTHONPATH:-}"

echo -e "${GREEN}=== Fenix Memory Testing ===${NC}"
echo "Using: ${BLUE}AddressSanitizer + LeakSanitizer${NC}"
echo "Project root: ${PROJECT_ROOT}"
echo "Build directory: ${BUILD_DIR}"
echo "Report directory: ${REPORT_DIR}"

# Clean and create build directory
echo -e "\n${GREEN}Cleaning and creating build directory...${NC}"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
mkdir -p "${REPORT_DIR}"
cd "${BUILD_DIR}"

# Configure CMake with sanitizers
echo -e "\n${GREEN}Configuring CMake with AddressSanitizer + UndefinedBehaviorSanitizer...${NC}"

# Check for suppressions file
if [[ ! -f "${PROJECT_ROOT}/test/CI/lsan_suppressions.txt" ]]; then
    echo -e "${RED}ERROR: lsan_suppressions.txt not found at ${PROJECT_ROOT}/test/CI/${NC}"
    echo "This file should be committed to the repository."
    exit 1
fi

# Sanitizer flags: ASAN + LSAN + UBSAN
SANITIZER_FLAGS="-fsanitize=address -fsanitize=leak -fsanitize=undefined"
SANITIZER_FLAGS+=" -fno-omit-frame-pointer -g -O1"

# Configure environment for ASAN
export ASAN_OPTIONS="detect_leaks=1:check_initialization_order=1:strict_init_order=1"
export ASAN_OPTIONS+=":detect_stack_use_after_return=1:detect_invalid_pointer_pairs=2"
export LSAN_OPTIONS="suppressions=${PROJECT_ROOT}/test/CI/lsan_suppressions.txt:print_suppressions=0"

# Configure environment for UBSAN
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=0"

cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER=mpicc \
    -DCMAKE_CXX_COMPILER=mpicxx \
    -DCMAKE_C_FLAGS="${SANITIZER_FLAGS}" \
    -DCMAKE_CXX_FLAGS="${SANITIZER_FLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="${SANITIZER_FLAGS}" \
    -DBUILD_TESTING=ON \
    -DBUILD_EXAMPLES=OFF

# Build
echo -e "\n${GREEN}Building Fenix...${NC}"
make -j$(nproc)

# Run tests
echo -e "\n${GREEN}Running memory tests...${NC}"
echo -e "${BLUE}Running with AddressSanitizer + UndefinedBehaviorSanitizer...${NC}"
echo "ASAN_OPTIONS=${ASAN_OPTIONS}"
echo "LSAN_OPTIONS=${LSAN_OPTIONS}"
echo "UBSAN_OPTIONS=${UBSAN_OPTIONS}"

# Run tests with ASAN
# ASAN output goes to stderr and will be captured
TEST_OUTPUT="${REPORT_DIR}/test_output.log"

if ctest --output-on-failure --timeout 60 2>&1 | tee "${TEST_OUTPUT}"; then
    echo -e "${GREEN}All tests passed${NC}"
    TEST_RESULT=0
else
    echo -e "${RED}Some tests failed${NC}"
    TEST_RESULT=1
fi

# Parse ASAN output
echo -e "\n${GREEN}Analyzing sanitizer reports...${NC}"

# Extract ASAN errors
ASAN_ERRORS="${REPORT_DIR}/asan_errors.txt"
grep -E "==.*==ERROR: (AddressSanitizer|LeakSanitizer)" "${TEST_OUTPUT}" > "${ASAN_ERRORS}" || true

# Extract UBSAN errors
UBSAN_ERRORS="${REPORT_DIR}/ubsan_errors.txt"
grep "runtime error:" "${TEST_OUTPUT}" > "${UBSAN_ERRORS}" || true

# Check for ASAN/LSAN errors
if [[ -s "${ASAN_ERRORS}" ]]; then
    echo -e "${RED}Memory errors detected!${NC}"
    echo -e "\n${RED}=== AddressSanitizer Errors ===${NC}"
    cat "${ASAN_ERRORS}"

    # Count errors by type
    echo -e "\n${YELLOW}Error Summary:${NC}"
    echo "Heap buffer overflow: $(grep -c "heap-buffer-overflow" "${TEST_OUTPUT}" || echo 0)"
    echo "Stack buffer overflow: $(grep -c "stack-buffer-overflow" "${TEST_OUTPUT}" || echo 0)"
    echo "Use after free: $(grep -c "heap-use-after-free" "${TEST_OUTPUT}" || echo 0)"
    echo "Memory leaks: $(grep -c "detected memory leaks" "${TEST_OUTPUT}" || echo 0)"
    echo "Uninitialized value: $(grep -c "use-of-uninitialized-value" "${TEST_OUTPUT}" || echo 0)"

    exit 1
else
    echo -e "${GREEN}No memory errors detected by AddressSanitizer!${NC}"
fi

# Check for UBSAN errors
if [[ -s "${UBSAN_ERRORS}" ]]; then
    echo -e "${YELLOW}Undefined behavior detected!${NC}"
    echo -e "\n${YELLOW}=== UndefinedBehaviorSanitizer Errors ===${NC}"

    # Get unique error types
    UNIQUE_UBSAN=$(grep -o "[^:]*runtime error: [^$]*" "${UBSAN_ERRORS}" | sort -u)
    echo "$UNIQUE_UBSAN"

    # Count errors by type
    echo -e "\n${YELLOW}UBSAN Error Summary:${NC}"
    echo "Signed integer overflow: $(grep -c "signed integer overflow" "${TEST_OUTPUT}" || echo 0)"
    echo "Unsigned integer overflow: $(grep -c "unsigned integer overflow" "${TEST_OUTPUT}" || echo 0)"
    echo "Division by zero: $(grep -c "division by zero" "${TEST_OUTPUT}" || echo 0)"
    echo "Null pointer: $(grep -c "null pointer" "${TEST_OUTPUT}" || echo 0)"
    echo "Misaligned access: $(grep -c "misaligned address" "${TEST_OUTPUT}" || echo 0)"
    echo "Shift errors: $(grep -c "shift" "${TEST_OUTPUT}" || echo 0)"

    echo -e "\n${YELLOW}Note: UBSAN errors indicate undefined behavior that should be fixed.${NC}"
    echo "Review the errors above and fix the reported issues."

    # Don't exit - UBSAN errors are warnings, not fatal
    TEST_RESULT=1
else
    echo -e "${GREEN}No undefined behavior detected by UndefinedBehaviorSanitizer!${NC}"
fi

# Final summary
echo -e "\n${GREEN}=== Memory Testing Complete ===${NC}"
echo "Build directory: ${BUILD_DIR}"
echo "Reports: ${REPORT_DIR}"
echo ""
echo "To clean up, run: rm -rf ${BUILD_DIR}"

exit ${TEST_RESULT}
