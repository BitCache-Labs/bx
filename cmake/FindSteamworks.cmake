# Locate Steamworks library
# This module defines:
#   STEAMWORKS_LIBRARY       - the library to link against
#   STEAMWORKS_INCLUDE_DIR   - the directory containing steam/steam_api.h
#   STEAMWORKS_FOUND         - whether Steamworks was found
#   A target `steamworks` that can be linked directly via target_link_libraries

# NOTE:
#   - On Windows, ensure the SteamworksDIR environment variable is set to the root of the Steamworks SDK 
#     (e.g., C:/Path/To/SteamworksSDK).
#   - The expected structure for the Steamworks SDK directory is:
#       SteamworksSDK/
#       ├── glmgr
#       ├── public
#       ├── redistributable_bin
#       ├── steamworksexample
#       ├── tools
#       └── readme.txt

# Base search paths for Steamworks
set(STEAMWORKS_SEARCH_PATHS
    ~/Library/Frameworks             # macOS Frameworks
    /Library/Frameworks              # macOS Frameworks
    /usr/local                       # Common UNIX installation paths
    /usr
    /sw                              # Fink
    /opt/local                       # DarwinPorts (macOS)
    /opt/csw                         # Blastwave (UNIX)
    /opt                             # Generic UNIX installations
)

# Prefixes for searching the library files
set(STEAMWORKS_SEARCH_PREFIXES
    lib                              # General library directories
    lib64                            # 64-bit libraries
)

# Consolidated OS-specific handling
if (APPLE)
    # macOS-specific directories
    list(APPEND STEAMWORKS_SEARCH_PREFIXES
        redistributable_bin/osx32      # Legacy macOS 32-bit binaries
        redistributable_bin/osx        # macOS binaries
    )
elseif (UNIX AND NOT APPLE)
    # UNIX/Linux-specific directories
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "i.86")
        set(build_arch "32")           # 32-bit architecture
    elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(build_arch "64")           # 64-bit architecture
    endif()
        list(APPEND STEAMWORKS_SEARCH_PREFIXES
            redistributable_bin/linux${build_arch} # Linux binaries (32-bit/64-bit)
        )
elseif (WIN32)
    # Windows-specific directories
    list(APPEND STEAMWORKS_SEARCH_PREFIXES
        redistributable_bin/win64      # Windows 64-bit binaries

        # TODO: Add conditional to only search here if not win64
        # Windows 32-bit binaries
        #redistributable_bin
        #redistributable_bin/win32
    )
endif()

# Add the SteamworksDIR environment variable (useful for all platforms)
if (DEFINED ENV{SteamworksDIR})
    list(APPEND STEAMWORKS_SEARCH_PATHS $ENV{SteamworksDIR})
endif()

# Find include directory
find_path(
    STEAMWORKS_INCLUDE_DIR
    steam/steam_api.h
    PATHS ${STEAMWORKS_SEARCH_PATHS}
    PATH_SUFFIXES public include Include
)

# Find library
find_library(STEAMWORKS_LIBRARY
    NAMES steam_api steam_api64
    PATHS ${STEAMWORKS_SEARCH_PATHS}
    PATH_SUFFIXES ${STEAMWORKS_SEARCH_PREFIXES}
)

if (NOT STEAMWORKS_INCLUDE_DIR)
    message(WARNING "steam_api.h not found. Please check your SteamworksDIR.")
endif()

if (NOT STEAMWORKS_LIBRARY)
  message(WARNING "steam_api library not found. Please check your SteamworksDIR.")
endif()

# Check for the DLL if on Windows
if (WIN32 AND STEAMWORKS_LIBRARY)
    # Get the library name (without path)
    get_filename_component(STEAMWORKS_LIBRARY_NAME ${STEAMWORKS_LIBRARY} NAME_WE)

    # Look for corresponding DLL in the same folder as the library
    find_file(STEAMWORKS_DLL
        NAMES ${STEAMWORKS_LIBRARY_NAME}.dll
        PATHS ${STEAMWORKS_SEARCH_PATHS}
        PATH_SUFFIXES ${STEAMWORKS_SEARCH_PREFIXES}
    )

    if (STEAMWORKS_DLL)
        # Set the path for the DLL
        set(STEAMWORKS_DLL_PATH ${STEAMWORKS_DLL} CACHE PATH "Path to the Steamworks DLL")
    else()
        message(WARNING "Steamworks DLL not found. Make sure the DLL is in the correct path.")
    endif()
endif()

# Generate a target for the Steamworks library
if (STEAMWORKS_INCLUDE_DIR AND STEAMWORKS_LIBRARY)
    add_library(steamworks STATIC IMPORTED GLOBAL)
    set_target_properties(steamworks PROPERTIES
        IMPORTED_LOCATION "${STEAMWORKS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${STEAMWORKS_INCLUDE_DIR}"
    )
    set(STEAMWORKS_FOUND TRUE CACHE INTERNAL "Steamworks found flag")
else()
    set(STEAMWORKS_FOUND FALSE CACHE INTERNAL "Steamworks not found flag")
endif()

# Standard package handling for Steamworks
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Steamworks REQUIRED_VARS STEAMWORKS_LIBRARY STEAMWORKS_INCLUDE_DIR)