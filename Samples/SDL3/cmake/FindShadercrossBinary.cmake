# FindShadercrossBinary.cmake
#
# Downloads pre-built shadercross binaries from GitHub Releases
# via FetchContent.
#
# Usage:
#   set(SHADERCROSS_VERSION "<upstream-commit-sha>")
#   include(cmake/FindShadercrossBinary.cmake)
#   # SHADERCROSS_EXECUTABLE is now set
#
# Required variables:
#   SHADERCROSS_VERSION  - Upstream (libsdl-org/SDL_shadercross) commit SHA
#
# Optional variables:
#   SHADERCROSS_GITHUB_REPO - GitHub repo for releases (default: wamsoft/SDL_shadercross)
#
# Output variables:
#   SHADERCROSS_EXECUTABLE - Full path to the shadercross CLI binary

include(FetchContent)

if(NOT DEFINED SHADERCROSS_VERSION)
    message(FATAL_ERROR "SHADERCROSS_VERSION must be set to the upstream SDL_shadercross commit SHA")
endif()

if(NOT DEFINED SHADERCROSS_GITHUB_REPO)
    set(SHADERCROSS_GITHUB_REPO "wamsoft/SDL_shadercross")
endif()

# Platform detection
if(WIN32)
    set(_SHADERCROSS_PLATFORM "windows-x64")
    set(_SHADERCROSS_EXT "zip")
elseif(APPLE)
    set(_SHADERCROSS_PLATFORM "macos")
    set(_SHADERCROSS_EXT "tar.gz")
else()
    set(_SHADERCROSS_PLATFORM "linux-x64")
    set(_SHADERCROSS_EXT "tar.gz")
endif()

set(_SHADERCROSS_URL "https://github.com/${SHADERCROSS_GITHUB_REPO}/releases/download/upstream-${SHADERCROSS_VERSION}/shadercross-${_SHADERCROSS_PLATFORM}.${_SHADERCROSS_EXT}")

message(STATUS "Fetching shadercross binary from: ${_SHADERCROSS_URL}")

FetchContent_Declare(
    shadercross_bin
    URL "${_SHADERCROSS_URL}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(shadercross_bin)

find_program(SHADERCROSS_EXECUTABLE
    NAMES shadercross
    PATHS "${shadercross_bin_SOURCE_DIR}/bin"
    NO_DEFAULT_PATH
    REQUIRED
)

message(STATUS "Found shadercross: ${SHADERCROSS_EXECUTABLE}")
