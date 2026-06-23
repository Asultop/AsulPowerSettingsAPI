add_library(AsulPowerSettingsApi STATIC
    PowerSettingsManager.cpp
    Types.cpp
)

target_include_directories(AsulPowerSettingsApi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(AsulPowerSettingsApi PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_link_libraries(AsulPowerSettingsApi
    PRIVATE
        powrprof
        advapi32
)

set_target_properties(AsulPowerSettingsApi PROPERTIES
    OUTPUT_NAME "AsulPowerSettingsApi"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
