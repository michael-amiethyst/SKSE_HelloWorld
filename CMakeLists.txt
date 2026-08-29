cmake_minimum_required(VERSION 3.21)

set(MOD_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/mod")
set(REQUIRED_SEQ_FILE "${MOD_SOURCE}/Seq/HelloWorld.seq")

if(NOT EXISTS "${REQUIRED_SEQ_FILE}")
    message(
            FATAL_ERROR
            "Required SEQ file is missing: ${REQUIRED_SEQ_FILE}\n"
            "HelloWorld's start-game-enabled MCM quest will not reliably register without it.\n"
            "Generate the file with xEdit before building. Full instructions at \"Generating the SEQ file\" header in "
            "${CMAKE_CURRENT_SOURCE_DIR}/docs/contributing.md\n"
    )
endif()

# Set your project name. This will be the name of your SKSE .dll file.
project(HelloWorld VERSION 0.3.0 LANGUAGES CXX)

set(MOD_CONTENTS "${CMAKE_SOURCE_DIR}/build/ModContents")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${REQUIRED_SEQ_FILE}")

add_custom_target(
        HelloWorldAssets
        COMMAND "${CMAKE_COMMAND}" -E remove_directory "${MOD_CONTENTS}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${MOD_CONTENTS}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${MOD_SOURCE}" "${MOD_CONTENTS}"
        COMMENT "Staging checked-in mod assets"
        VERBATIM
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

add_custom_target(
        HelloWorldPackage ALL
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND "${CMAKE_COMMAND}" -E chdir "${MOD_CONTENTS}"
                "${CMAKE_COMMAND}" -E tar cf "${MOD_PATH}" --format=7zip .
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DEPLOY_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${MOD_PATH}" "${DEPLOY_FOLDER}/"
        DEPENDS "${PROJECT_NAME}" HelloWorldAssets
        COMMENT "Packaging HelloWorld for Skyrim VR"
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
