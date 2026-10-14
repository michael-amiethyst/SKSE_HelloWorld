file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}")
set(pex "${OUTPUT_DIRECTORY}/HelloWorldMCM.pex")
get_filename_component(source_directory "${SOURCE}" DIRECTORY)
# Never accept output left behind by an earlier successful compilation.
file(REMOVE "${pex}")
execute_process(
    COMMAND "${COMPILER}" "${SOURCE}" "-i=${source_directory};${IMPORT_DIRECTORIES}" "-f=${FLAGS}" "-o=${OUTPUT_DIRECTORY}"
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0 OR NOT EXISTS "${pex}")
    message(FATAL_ERROR "Papyrus compilation failed or produced no HelloWorldMCM.pex")
endif()
