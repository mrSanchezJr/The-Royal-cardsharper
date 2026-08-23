cmake_minimum_required(VERSION 3.14)
project(TheRoyalCardshaper C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optimization flags for GCC/Clang
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # Optimize for size, strip symbols, disable exceptions/RTTI (C++ mostly but good practice)
    add_compile_options(-Os -s -ffunction-sections -fdata-sections -Wall)
    add_link_options(-static -s -Wl,--gc-sections)
endif()

# Link against the same prebuilt raylib used by build.bat (w64devkit bundle).
# Do NOT build C:/raylib/raylib sources: that tree is raylib 6.0 and its
# window message loop hangs on some systems; the w64devkit lib is stable.
add_library(raylib STATIC IMPORTED)
set_target_properties(raylib PROPERTIES
    IMPORTED_LOCATION "C:/raylib/w64devkit/lib/libraylib.a"
    INTERFACE_INCLUDE_DIRECTORIES "C:/raylib/w64devkit/include"
)

# Gather source files
file(GLOB SOURCES "src/*.c")

# Add executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib gdi32 winmm)

# Setup UPX download if it doesn't exist
set(UPX_URL "https://github.com/upx/upx/releases/download/v4.2.4/upx-4.2.4-win64.zip")
set(UPX_ARCHIVE "${CMAKE_BINARY_DIR}/upx.zip")
set(UPX_DIR "${CMAKE_BINARY_DIR}/upx-4.2.4-win64")
set(UPX_EXE "${UPX_DIR}/upx.exe")

if(NOT EXISTS ${UPX_EXE})
    message(STATUS "Downloading UPX...")
    file(DOWNLOAD ${UPX_URL} ${UPX_ARCHIVE} STATUS DOWNLOAD_STATUS)
    message(STATUS "Extracting UPX...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${UPX_ARCHIVE}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# Add custom command to run UPX after build
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${UPX_EXE} --best --lzma $<TARGET_FILE:${PROJECT_NAME}>
#     COMMENT "Compressing executable with UPX..."
# )
