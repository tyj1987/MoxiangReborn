# Runs mxh_client_e2e with no password and requires exit code 3 plus
# the password-required phrase. CTest PASS_REGULAR_EXPRESSION on the
# parent test sees the phrase printed below. WILL_FAIL is not used:
# CTest inverts a regex match when WILL_FAIL is true.
if(NOT EXE)
    message(FATAL_ERROR "EXE is not set")
endif()
execute_process(
    COMMAND "${EXE}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
)
set(text "${out}${err}")
message("${text}")
if(NOT rc EQUAL 3)
    message(FATAL_ERROR "expected exit 3, got ${rc}")
endif()
if(NOT text MATCHES "password required")
    message(FATAL_ERROR "expected password required phrase")
endif()
