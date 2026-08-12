# It's recommended to set a minimum CMake version.
# If you use CMake features from higher versions, update this to match.
cmake_minimum_required(VERSION 3.21)

# Set your project name. This will be the name of your SKSE .dll file.
# Version format: major.minor.patch.rc (bump the RC component manually).
project(HelloWorld VERSION 0.1.0.2 LANGUAGES CXX)

# #
# YOU DO NOT NEED TO EDIT ANYTHING BELOW HERE
# #

# If you're not using a mod manager, you probably want the SKSE plugin to go
# inside of your Skyrim "Data" folder.
#
# To do this automatically, set the `SKYRIM_FOLDER` environment variable
# to the path of your Skyrim Special Edition folder
if(DEFINED ENV{SKYRIM_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_FOLDER}/Data")
    set(MOD_CONTENTS "$ENV{SKYRIM_FOLDER}/Data")
endif()

# If you're using Mod Organizer 2 or Vortex, you might want this to go inside
# of your "mods" folder, inside of a subfolder named "<your mod>".
#
# To do this automatically, set the `SKYRIM_MODS_FOLDER` environment variable
# to the path of your "mods" folder
if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
    set(MOD_CONTENTS "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME}")
endif()

# Otherwise, you can set MOD_CONTENTS to any place you'd like :)
set(MOD_CONTENTS "${CMAKE_SOURCE_DIR}/build/ModContents")

# Setup your SKSE plugin as an SKSE plugin!
find_package(CommonLibSSE CONFIG REQUIRED)
add_commonlibsse_plugin(${PROJECT_NAME} SOURCES src/plugin.cpp) # <--- specifies plugin.cpp
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23) # <--- use C++23 standard
target_precompile_headers(${PROJECT_NAME} PRIVATE lib/PCH.h) # <--- PCH.h is required!

# When your SKSE .dll is compiled, this will automatically copy the .dll into your mods folder.
# Only works if you configure DEPLOY_ROOT above (or set the SKYRIM_MODS_FOLDER environment variable)
if(DEFINED MOD_CONTENTS)
    # If you specify an <MOD_CONTENTS> (including via environment variables)
    # then we'll copy your mod files into Skyrim or a mod manager for you!

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
else ()
    message(STATUS "Default Output location used")
endif()
