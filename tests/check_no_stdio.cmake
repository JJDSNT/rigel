if(NOT DEFINED NM OR NOT DEFINED LIB)
    message(FATAL_ERROR "NM and LIB are required")
endif()

execute_process(
    COMMAND "${NM}" -u "${LIB}"
    RESULT_VARIABLE nm_result
    OUTPUT_VARIABLE nm_output
    ERROR_VARIABLE nm_error
)

if(NOT nm_result EQUAL 0)
    message(FATAL_ERROR "nm failed: ${nm_error}")
endif()

if(nm_output MATCHES "(^|[\r\n]).*(printf|fprintf|snprintf|stderr)")
    message(FATAL_ERROR "bare-metal build references stdio symbols:\n${nm_output}")
endif()
