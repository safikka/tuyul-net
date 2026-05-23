#!/bin/bash

# Terminate script execution immediately if any command encounters an error
set -e

# Define ANSI Color Codes for universal terminal coloring
COLOR_RESET="\033[0m"
COLOR_GREEN="\033[32m"
COLOR_RED="\033[31m"
COLOR_YELLOW="\033[33m"
COLOR_CYAN="\033[36m"

# Define operational paths
BUILD_DIR="build"
PACKAGE_DIR="package"
VERSION_FILE="VERSION"
DEP_FILE="dependencies.txt"
COVERAGE_OUT_DIR="coverage_html"
SUFFIX_CACHE="${BUILD_DIR}/.package_suffix"

# Custom error handler to catch pipeline failures and print a loud alert
error_trap() {
    echo -e "\n=========================================="
    echo -e "${COLOR_RED}[BUILD FAILED] An error occurred during configuration, compilation, testing, or coverage reporting!${COLOR_RESET}"
    echo "=========================================="
}
# Link the error handler to the native shell exit signal on failure
trap error_trap ERR

# Read global version string from file, fallback to 0.1.0 if missing
if [ -f "$VERSION_FILE" ]; then
    VERSION_STR=$(tr -d '\r\n' < "$VERSION_FILE")
else
    VERSION_STR="0.1.0"
fi

# Function: Display automated help instructions
show_help() {
    echo "Usage: ./build.sh [options]"
    echo "Options:"
    echo "  -c <suffix>  Configure the CMake environment (Optional: custom identifier, e.g., -c testing)"
    echo "  -b           Build, compile, and automatically execute all unit tests"
    echo "  -V           Generate localized Code Coverage HTML reports and launch browser view"
    echo "  -C           Clean all build artifacts, coverage data, and generated packages"
    echo "  -i           Install and generate the release package (.tar.gz)"
    echo "  -h           Display this help interface menu"
    exit 0
}

# Function: Verify presence of core system binaries
validate_environment() {
    if [ ! -f "$DEP_FILE" ]; then
        echo -e "${COLOR_RED}[ERROR] Configuration file '$DEP_FILE' is missing from the root directory!${COLOR_RESET}"
        exit 1
    fi

    echo -e "${COLOR_CYAN}[TUYUL] Validating system environment...${COLOR_RESET}"
    while IFS= read -r line || [ -n "$line" ]; do
        clean_line=$(echo "$line" | tr -d '\r')
        if [[ -z "$clean_line" || "$clean_line" =~ ^# ]]; then
            continue
        fi
        if ! command -v "$clean_line" &> /dev/null; then
            echo -e "${COLOR_RED}[ERROR] Required tool '$clean_line' was not found on your system!${COLOR_RESET}"
            exit 1
        fi
        echo -e "${COLOR_GREEN}[OK] System dependency '$clean_line' is verified.${COLOR_RESET}"
    done < "$DEP_FILE"

    # Verify GCC compiler version for modern C++17 layout rules
    GCC_VERSION=$(g++ -dumpversion | cut -d. -f1)
    if [ "$GCC_VERSION" -lt 7 ]; then
        echo -e "${COLOR_RED}[ERROR] Your g++ compiler version is too outdated (Requires version 7+ for C++17)!${COLOR_RESET}"
        exit 1
    fi
    echo -e "${COLOR_GREEN}[SUCCESS] Environment lookup passed successfully.${COLOR_RESET}"
}

# Function: Execute clean-up operational sequence
run_clean() {
    # Temporarily remove trap during explicit manual cleans
    trap - ERR
    echo -e "${COLOR_CYAN}[TUYUL] Sweeping away all runtime compilation, coverage, and packaging artifacts...${COLOR_RESET}"
    rm -rf "$BUILD_DIR"
    rm -rf "$PACKAGE_DIR"
    rm -rf "$COVERAGE_OUT_DIR"
    rm -f coverage.info coverage_clean.info
    echo -e "${COLOR_GREEN}[SUCCESS] Workspace is completely clean.${COLOR_RESET}"
}

# Function: Execute project environment configuration
run_configure() {
    local suffix="$1"
    validate_environment
    
    echo -e "\n${COLOR_CYAN}[TUYUL] Initializing CMake build framework configuration...${COLOR_RESET}"
    if [ ! -d "$BUILD_DIR" ]; then
        mkdir "$BUILD_DIR"
    fi
    
    if [ -n "$suffix" ]; then
        echo "$suffix" > "$SUFFIX_CACHE"
        echo -e "${COLOR_GREEN}[OK] Custom naming override set to: 'tuyul-net-${suffix}'${COLOR_RESET}"
    else
        rm -f "$SUFFIX_CACHE"
        echo -e "${COLOR_GREEN}[OK] Using default name convention (tuyul-net-${VERSION_STR}).${COLOR_RESET}"
    fi

    cd "$BUILD_DIR"
    cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
    cd ..
    echo -e "${COLOR_GREEN}[SUCCESS] Configuration sequence finished.${COLOR_RESET}"
}

# Function: Execute system compilation pipeline and trigger unit testing automatically
run_build() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo -e "${COLOR_YELLOW}[WARN] Build directory missing. Auto-triggering configuration sequence...${COLOR_RESET}"
        run_configure ""
    fi
    echo -e "\n${COLOR_CYAN}[TUYUL] Triggering native engine compilation pipeline...${COLOR_RESET}"
    cd "$BUILD_DIR"
    cmake --build .
    
    echo -e "\n${COLOR_CYAN}[TUYUL] Executing automated unit test framework via CTest...${COLOR_RESET}"
    ctest --output-on-failure
    
    cd ..
    echo -e "${COLOR_GREEN}[SUCCESS] Build and all localized unit tests passed perfectly!${COLOR_RESET}"
}

# Function: Harvest code coverage counters, extract HTML reports, and fire up native browser layout
run_coverage_view() {
    # Ensure raw binary objects and recent profiling counters exist by triggering a build sequence first
    run_build

    echo -e "\n${COLOR_CYAN}[TUYUL] Processing code coverage metric footprints via lcov...${COLOR_RESET}"
    
    # 1. Extract raw instrumentation data counters from target build spaces
    # Added --ignore-errors mismatch,unused to bypass strict modern lcov 2.x validation checks on GTest macros
    lcov --capture --directory "$BUILD_DIR" --output-file coverage.info --quiet --ignore-errors mismatch,unused

    # 2. Filter out systemic system architecture scopes and vendor testing dependencies
    lcov --remove coverage.info '/usr/*' '*/tuyul-lib/*' '*/tests/*' --output-file coverage_clean.info --quiet --ignore-errors mismatch,unused

    # 3. Transform filtered analytics trace summaries into standard static HTML layouts
    echo -e "${COLOR_CYAN}-> Compiling structured coverage metrics into localized HTML sheets...${COLOR_RESET}"
    rm -rf "$COVERAGE_OUT_DIR"
    genhtml coverage_clean.info --output-directory "$COVERAGE_OUT_DIR" --quiet --ignore-errors mismatch,unused

    # 4. Automatically identify target operating system browser hook points to launch report views
    echo -e "${COLOR_GREEN}[SUCCESS] Report pipeline completed! Opening localized view inside web browser...${COLOR_RESET}"
    local index_path="${COVERAGE_OUT_DIR}/index.html"
    
    if command -v xdg-open &> /dev/null; then
        xdg-open "$index_path"    # Standard Linux desktop environment wrapper hook
    elif command -v open &> /dev/null; then
        open "$index_path"        # macOS desktop platform native invocation hook
    else
        echo -e "${COLOR_YELLOW}[WARN] Desktop visualization hook missing. Please manually load: ./${index_path}${COLOR_RESET}"
    fi
}

# Function: Prepare final production-ready directory archive layout
run_install() {
    echo -e "\n${COLOR_CYAN}[TUYUL] Launching release package distribution pipeline...${COLOR_RESET}"
    
    run_build

    local current_suffix=""
    if [ -f "$SUFFIX_CACHE" ]; then
        current_suffix=$(cat "$SUFFIX_CACHE" | tr -d '\r\n ')
    fi

    local staging_name="tuyul-net-${VERSION_STR}"
    if [ -n "$current_suffix" ]; then
        staging_name="tuyul-net-${current_suffix}"
    fi
    
    local staging_dir="${PACKAGE_DIR}/${staging_name}"

    echo -e "${COLOR_CYAN}-> Flushing previous packaging space...${COLOR_RESET}"
    rm -rf "$PACKAGE_DIR"
    mkdir -p "$staging_dir/include/tuyul"
    mkdir -p "$staging_dir/lib"

    echo -e "${COLOR_CYAN}-> [Staging] Creating provisional blueprint markers inside packaging space...${COLOR_RESET}"
    echo "${staging_name} production layout placeholder" > "$staging_dir/RELEASE_NOTE.txt"

    echo -e "${COLOR_CYAN}-> Compressing staging directory into standard production archive...${COLOR_RESET}"
    cd "$PACKAGE_DIR"
    tar -czf "${staging_name}.tar.gz" "$staging_name"
    
    rm -rf "$staging_name"
    cd ..

    echo "=========================================="
    echo -e "${COLOR_GREEN}[SUCCESS] Installation archive successfully prepared!${COLOR_RESET}"
    echo -e "📦 Package: ${COLOR_YELLOW}./${PACKAGE_DIR}/${staging_name}.tar.gz${COLOR_RESET}"
    echo "=========================================="
}

# Parse command line flags using native Bash getopts loop
if [ $# -eq 0 ]; then
    show_help
fi

# Registered capital 'V' for localized Code Coverage execution pipelines
while getopts "bVCihc:" opt; do
    case "$opt" in
        c) run_configure "$OPTARG" ;;
        b) run_build ;;
        V) run_coverage_view ;;
        C) run_clean ;;
        i) run_install ;;
        h) show_help ;;
        *) show_help ;;
    esac
done
