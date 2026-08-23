cmake_minimum_required(VERSION 3.21)

# Set your project name. This will be the name of your SKSE .dll file.
project(HelloWorld VERSION 0.3.0 LANGUAGES CXX)

set(MOD_CONTENTS "${CMAKE_SOURCE_DIR}/build/ModContents")

find_program(POWERSHELL_EXECUTABLE NAMES pwsh powershell REQUIRED)
find_program(
        DOTNET_EXECUTABLE
        NAMES dotnet
        HINTS "${CMAKE_SOURCE_DIR}/build/tools/dotnet"
        REQUIRED
)
execute_process(
        COMMAND "${DOTNET_EXECUTABLE}" --list-sdks
        OUTPUT_VARIABLE DOTNET_SDKS
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(DOTNET_SDKS STREQUAL "")
    message(FATAL_ERROR "Spriggit requires a .NET SDK. Install .NET 9 SDK or set DOTNET_EXECUTABLE.")
endif()

set(MOD_PLUGIN "${MOD_CONTENTS}/HelloWorld.esp")
set(MOD_PEX "${MOD_CONTENTS}/Scripts/HelloWorldMCM.pex")
set(MOD_MCM_CONFIG "${MOD_CONTENTS}/MCM/Config/HelloWorld/config.json")
set(MOD_MCM_DEFAULTS "${MOD_CONTENTS}/MCM/Config/HelloWorld/settings.ini")

add_custom_command(
        OUTPUT
        "${MOD_PLUGIN}"
        "${MOD_PEX}"
        "${MOD_MCM_CONFIG}"
        "${MOD_MCM_DEFAULTS}"
        COMMAND
        "${POWERSHELL_EXECUTABLE}"
        -NoProfile
        -ExecutionPolicy Bypass
        -File "${CMAKE_SOURCE_DIR}/cmake/BuildModAssets.ps1"
        -SourceDirectory "${CMAKE_SOURCE_DIR}"
        -BinaryDirectory "${CMAKE_BINARY_DIR}"
        -ModContentsDirectory "${MOD_CONTENTS}"
        -DotnetExecutable "${DOTNET_EXECUTABLE}"
        DEPENDS
        "${CMAKE_SOURCE_DIR}/cmake/BuildModAssets.ps1"
        "${CMAKE_SOURCE_DIR}/scripts/Source/HelloWorldMCM.psc"
        "${CMAKE_SOURCE_DIR}/scripts/SkyrimPapyrusFlags.flg"
        "${CMAKE_SOURCE_DIR}/plugin/HelloWorld/spriggit-meta.json"
        "${CMAKE_SOURCE_DIR}/plugin/HelloWorld/RecordData.yaml"
        "${CMAKE_SOURCE_DIR}/plugin/HelloWorld/Quests/HelloWorld MCM Quest - 000800_HelloWorld.esp.yaml"
        "${CMAKE_SOURCE_DIR}/mcm/Config/HelloWorld/config.json"
        "${CMAKE_SOURCE_DIR}/mcm/Config/HelloWorld/settings.ini"
        COMMENT "Building Creation-Kit-free Papyrus and plugin assets"
        VERBATIM
)
add_custom_target(HelloWorldAssets DEPENDS
        "${MOD_PLUGIN}"
        "${MOD_PEX}"
        "${MOD_MCM_CONFIG}"
        "${MOD_MCM_DEFAULTS}"
)

# Setup your SKSE plugin as an SKSE plugin!
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(SimpleIni CONFIG REQUIRED)
add_commonlibsse_plugin(
        ${PROJECT_NAME}
        SOURCES
        src/MCMHelperAPI.cpp
        src/MCMHelperAPI.h
        src/NotificationService.cpp
        src/NotificationService.h
        src/HeadMountedDisplay.cpp
        src/HeadMountedDisplay.h
        src/plugin.cpp
)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23) # <--- use C++23 standard
target_precompile_headers(${PROJECT_NAME} PRIVATE lib/PCH.h) # <--- PCH.h is required!
target_include_directories(${PROJECT_NAME} PRIVATE lib)
target_link_libraries(${PROJECT_NAME} PRIVATE SimpleIni::SimpleIni)
add_dependencies(${PROJECT_NAME} HelloWorldAssets)

include(CTest)
if(BUILD_TESTING)
    find_package(Catch2 3 CONFIG REQUIRED)

    add_executable(
            HelloWorldTests
            tests/MCMHelperAPITests.cpp
            tests/NotificationServiceTests.cpp
            tests/OpenVRTransportDetectorTests.cpp
            src/MCMHelperAPI.cpp
            src/NotificationService.cpp
            src/HeadMountedDisplay.cpp
    )
    target_compile_features(HelloWorldTests PRIVATE cxx_std_23)
    target_precompile_headers(HelloWorldTests PRIVATE lib/PCH.h)
    target_include_directories(HelloWorldTests PRIVATE lib src)
    target_link_libraries(
            HelloWorldTests
            PRIVATE
            CommonLibSSE::CommonLibSSE
            Catch2::Catch2WithMain
            SimpleIni::SimpleIni
    )

    include(Catch)
    catch_discover_tests(HelloWorldTests)
endif()

# When your SKSE .dll is compiled, this will automatically copy the .dll into your mods folder.
# Only works if you configure DEPLOY_ROOT above (or set the SKYRIM_MODS_FOLDER environment variable)

# Copy the SKSE plugin .dll files into the SKSE/Plugins/ folder
set(DLL_FOLDER "${MOD_CONTENTS}/SKSE/Plugins")
set(
        DEPLOY_FOLDER
        "${CMAKE_SOURCE_DIR}/build/packages"
        CACHE PATH
        "Directory receiving packaged releases"
)
get_filename_component(PACKAGE_OUTPUT_DIR "${MOD_CONTENTS}" DIRECTORY)
set(MOD_PATH "${PACKAGE_OUTPUT_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}-SkyrimVR.7z")

message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")
message(STATUS "DEPLOY_FOLDER: ${DEPLOY_FOLDER}")

add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        # DLL_FOLDER
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        # Archive from inside the staging directory so entries are relative
        # (SKSE/Plugins/...) and never contain absolute or ".." components.
        # Keep the archive outside MOD_CONTENTS to avoid archiving an older copy.
        COMMAND "${CMAKE_COMMAND}" -E chdir "${MOD_CONTENTS}"
                "${CMAKE_COMMAND}" -E tar cf "${MOD_PATH}" --format=7zip .
        # DEPLOY_FOLDER
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DEPLOY_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${MOD_PATH}" "${DEPLOY_FOLDER}/"
        VERBATIM
)

# If you perform a "Debug" build, also copy .pdb file (for debug symbols)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
    )
endif()
