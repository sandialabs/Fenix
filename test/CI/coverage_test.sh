#!/bin/bash
#
# coverage_test.sh - Build Fenix with coverage enabled and generate reports
#
# This script must be run from the root directory of the Fenix project.
# It creates a build_coverage/ subdirectory and leaves no artifacts elsewhere.
#
# Usage:
#   ./test/CI/coverage_test.sh
#
# Requirements:
#   - gcov (part of GCC)
#   - lcov (install via: apt-get install lcov / brew install lcov)
#   - MPI with fault tolerance support
#

set -e  # Exit on error
set -u  # Exit on undefined variable

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
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
BUILD_DIR="${PROJECT_ROOT}/build_coverage"

echo -e "${GREEN}=== Fenix Code Coverage Test ===${NC}"
echo "Project root: ${PROJECT_ROOT}"
echo "Build directory: ${BUILD_DIR}"

# Check for required tools
if ! command -v gcov &> /dev/null; then
    echo -e "${RED}ERROR: gcov not found. Install GCC or use Clang with llvm-cov${NC}"
    exit 1
fi

if ! command -v gcovr &> /dev/null; then
    echo -e "${YELLOW}WARNING: gcovr not found. Will use gcov for basic reporting only${NC}"
    echo "Install gcovr for HTML reports: pip install gcovr"
    HAVE_GCOVR=0
else
    HAVE_GCOVR=1
    echo "Using gcovr version: $(gcovr --version | head -1)"
fi

# Clean and create build directory
echo -e "\n${GREEN}Cleaning and creating build directory...${NC}"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure with coverage enabled
echo -e "\n${GREEN}Configuring CMake with coverage enabled...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_COVERAGE=ON \
    -DBUILD_TESTING=ON \
    -DBUILD_EXAMPLES=OFF \
    -DCMAKE_C_COMPILER=mpicc \
    -DCMAKE_CXX_COMPILER=mpicxx

# Build
echo -e "\n${GREEN}Building Fenix with coverage instrumentation...${NC}"
make -j$(nproc)

# Run tests
echo -e "\n${GREEN}Running tests...${NC}"

# Run ctest with verbose output on failure
if ctest --output-on-failure --timeout 30; then
    echo -e "${GREEN}All tests passed${NC}"
else
    echo -e "${RED}Some tests failed - coverage data may be incomplete${NC}"
    exit 1
fi

# Process coverage data
echo -e "\n${GREEN}Processing coverage data...${NC}"

if [[ ${HAVE_GCOVR} -eq 1 ]]; then
    # Use gcovr for comprehensive reporting
    echo "Generating coverage reports with gcovr..."

    # Generate text summary
    echo -e "\n${GREEN}=== Coverage Summary ===${NC}"
    gcovr --root "${PROJECT_ROOT}" \
          --filter "${PROJECT_ROOT}/src/" \
          --exclude "${PROJECT_ROOT}/test/" \
          --print-summary

    # Generate detailed text report
    echo -e "\n${GREEN}=== Detailed Coverage by File ===${NC}"
    gcovr --root "${PROJECT_ROOT}" \
          --filter "${PROJECT_ROOT}/src/" \
          --exclude "${PROJECT_ROOT}/test/"

    # Generate HTML report
    echo -e "\n${GREEN}Generating HTML coverage report...${NC}"
    mkdir -p coverage_html
    gcovr --root "${PROJECT_ROOT}" \
          --filter "${PROJECT_ROOT}/src/" \
          --exclude "${PROJECT_ROOT}/test/" \
          --html-details coverage_html/index.html

    echo -e "${GREEN}HTML coverage report generated in:${NC}"
    echo "  ${BUILD_DIR}/coverage_html/index.html"
    echo -e "  ${YELLOW}Open with: firefox ${BUILD_DIR}/coverage_html/index.html${NC}"

    # Also generate Cobertura XML for CI systems
    gcovr --root "${PROJECT_ROOT}" \
          --filter "${PROJECT_ROOT}/src/" \
          --exclude "${PROJECT_ROOT}/test/" \
          --xml coverage.xml

    echo -e "${GREEN}Cobertura XML report generated:${NC} coverage.xml"

else
    # Basic gcov reporting
    echo "Using basic gcov reporting (install gcovr for detailed HTML reports: pip install gcovr)..."

    # Find all .gcda files and run gcov on them
    find . -name "*.gcda" -type f | while read gcda_file; do
        dir=$(dirname "$gcda_file")
        gcov -p "${gcda_file}" > /dev/null 2>&1 || true
    done

    # Generate basic summary
    echo -e "\n${GREEN}=== Coverage Summary (gcov) ===${NC}"
    echo "Lines coverage by file:"
    find . -name "*.gcov" -type f | \
        grep -v "/usr/" | \
        grep -v "/test/" | \
        while read gcov_file; do
            lines_exec=$(grep -c "^    [0-9]" "$gcov_file" || true)
            lines_total=$(grep -c "^" "$gcov_file" || true)
            if [[ $lines_total -gt 0 ]]; then
                percent=$((lines_exec * 100 / lines_total))
                basename=$(basename "$gcov_file" .gcov)
                printf "  %-50s %3d%% (%d/%d lines)\n" \
                    "$basename" "$percent" "$lines_exec" "$lines_total"
            fi
        done | sort -t'%' -k2 -n -r
fi

# Summary
echo -e "\n${GREEN}=== Coverage Test Complete ===${NC}"
echo "All artifacts are contained in: ${BUILD_DIR}"
echo "To clean up, run: rm -rf ${BUILD_DIR}"

exit 0
