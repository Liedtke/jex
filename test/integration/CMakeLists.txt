configure_file(lit.site.cfg.py.in lit.site.cfg.py @ONLY)

set(INTEGRATION_CMD "${CMAKE_CURRENT_SOURCE_DIR}/run-lit.py" "${CMAKE_CURRENT_BINARY_DIR}" -v)

add_custom_target(check_integration
    COMMAND ${INTEGRATION_CMD}
    DEPENDS jexc)

add_test(NAME test_integration COMMAND ${INTEGRATION_CMD})
