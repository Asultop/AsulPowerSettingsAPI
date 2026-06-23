add_executable(power-settings-cli
    main.cpp
    CliApp.cpp
)

target_include_directories(power-settings-cli
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(power-settings-cli PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_link_libraries(power-settings-cli
    PRIVATE
        AsulPowerSettingsApi
)

set_target_properties(power-settings-cli PROPERTIES
    OUTPUT_NAME "power-settings"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
