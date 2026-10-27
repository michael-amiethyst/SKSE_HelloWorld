set(HELLOWORLD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${HELLOWORLD_GENERATED}/Source")
configure_file("${PROJECT_SOURCE_DIR}/build-tools/Version.h.in"
               "${HELLOWORLD_GENERATED}/Version.h" @ONLY NEWLINE_STYLE LF)
configure_file("${PROJECT_SOURCE_DIR}/scripts/Source/HelloWorldMCM.psc.in"
               "${HELLOWORLD_GENERATED}/Source/HelloWorldMCM.psc" @ONLY NEWLINE_STYLE LF)

set(papyrus_bundle "${PROJECT_SOURCE_DIR}/build-tools/papyrus")
set(default_papyrus_compiler "")
set(default_papyrus_imports "")
set(default_papyrus_flags "")
if(EXISTS "${papyrus_bundle}/PapyrusCompiler.exe")
    set(default_papyrus_compiler "${papyrus_bundle}/PapyrusCompiler.exe")
    set(default_papyrus_imports "${papyrus_bundle}/MCMHelper;${papyrus_bundle}/SRC_SKYUI;${papyrus_bundle}/SRC")
    set(default_papyrus_flags "${papyrus_bundle}/SkyrimPapyrusFlags.flg")
endif()
set(HELLOWORLD_PAPYRUS_COMPILER "${default_papyrus_compiler}" CACHE FILEPATH "Bethesda PapyrusCompiler.exe (optional for unchanged checked-in assets)")
set(HELLOWORLD_PAPYRUS_IMPORTS "${default_papyrus_imports}" CACHE STRING "Semicolon-separated MCM Helper, SkyUI, and Skyrim Papyrus source directories")
set(HELLOWORLD_PAPYRUS_FLAGS "${default_papyrus_flags}" CACHE FILEPATH "Skyrim Papyrus flags file")
set(mcm_source "${HELLOWORLD_GENERATED}/Source/HelloWorldMCM.psc")
set(mcm_stamp "${PROJECT_SOURCE_DIR}/mod/Scripts/HelloWorldMCM.version.json")
set(mcm_checked_pex "${PROJECT_SOURCE_DIR}/mod/Scripts/HelloWorldMCM.pex")

if(HELLOWORLD_PAPYRUS_COMPILER)
    if(NOT EXISTS "${HELLOWORLD_PAPYRUS_COMPILER}" OR NOT EXISTS "${HELLOWORLD_PAPYRUS_FLAGS}" OR NOT HELLOWORLD_PAPYRUS_IMPORTS)
        message(FATAL_ERROR "Papyrus compilation requires valid HELLOWORLD_PAPYRUS_COMPILER, HELLOWORLD_PAPYRUS_FLAGS, and HELLOWORLD_PAPYRUS_IMPORTS.")
    endif()
    set(HELLOWORLD_MCM_PEX "${HELLOWORLD_GENERATED}/Scripts/HelloWorldMCM.pex")
    add_custom_command(OUTPUT "${HELLOWORLD_MCM_PEX}"
        COMMAND "${CMAKE_COMMAND}" "-DCOMPILER=${HELLOWORLD_PAPYRUS_COMPILER}"
            "-DSOURCE=${mcm_source}" "-DIMPORT_DIRECTORIES=${HELLOWORLD_PAPYRUS_IMPORTS}"
            "-DFLAGS=${HELLOWORLD_PAPYRUS_FLAGS}" "-DOUTPUT_DIRECTORY=${HELLOWORLD_GENERATED}/Scripts"
            -P "${PROJECT_SOURCE_DIR}/build-tools/CompilePapyrus.cmake"
        DEPENDS "${mcm_source}" "${HELLOWORLD_PAPYRUS_FLAGS}" "${HELLOWORLD_PAPYRUS_COMPILER}"
            "${PROJECT_SOURCE_DIR}/build-tools/CompilePapyrus.cmake"
        VERBATIM)
    add_custom_target(HelloWorldPapyrus DEPENDS "${HELLOWORLD_MCM_PEX}")
    add_custom_target(RefreshMCMArtifact
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${HELLOWORLD_MCM_PEX}" "${mcm_checked_pex}"
        COMMAND "${CMAKE_COMMAND}" -DMODE=refresh "-DVERSION=${PROJECT_VERSION}"
            "-DSOURCE=${mcm_source}" "-DPEX=${mcm_checked_pex}" "-DSTAMP=${mcm_stamp}"
            -P "${PROJECT_SOURCE_DIR}/build-tools/CheckPapyrusArtifact.cmake"
        DEPENDS HelloWorldPapyrus VERBATIM)
else()
    set(HELLOWORLD_MCM_PEX "${mcm_checked_pex}")
    add_custom_target(HelloWorldPapyrus
        COMMAND "${CMAKE_COMMAND}" "-DVERSION=${PROJECT_VERSION}" "-DSOURCE=${mcm_source}"
            "-DPEX=${mcm_checked_pex}" "-DSTAMP=${mcm_stamp}"
            -P "${PROJECT_SOURCE_DIR}/build-tools/CheckPapyrusArtifact.cmake"
        VERBATIM)
endif()
