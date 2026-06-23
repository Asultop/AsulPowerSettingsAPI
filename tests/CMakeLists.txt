# Minimal test framework (no external dependencies)
add_executable(AsulPowerSettingsApiTests
    test_main.cpp
    test_guid.cpp
    test_power_manager.cpp
)

target_include_directories(AsulPowerSettingsApiTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(AsulPowerSettingsApiTests
    PRIVATE
        AsulPowerSettingsApi
)

set_target_properties(AsulPowerSettingsApiTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME PowerSettingsTests COMMAND AsulPowerSettingsApiTests)
