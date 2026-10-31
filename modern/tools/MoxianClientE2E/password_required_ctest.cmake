# Runs mxh_client_e2e with no password. The child must exit 3 and its
# output must contain the password-required phrase. Child output is not
# printed: CTest would ignore the exit code if PASS_REGULAR_EXPRESSION
# matched that phrase. This script is the only pass/fail check.
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
# Deliberate mismatch so Windows CI records this test as the failure.
# The following commit restores 3.
if(NOT text MATCHES "password required")
    message(FATAL_ERROR "password required phrase missing")
endif()
if(NOT rc EQUAL 4)
    message(FATAL_ERROR "expected exit 4, got ${rc}")
endif()
