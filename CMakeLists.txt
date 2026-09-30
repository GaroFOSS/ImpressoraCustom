cmake_minimum_required(VERSION 3.21)
project(ImpressoraCustom VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Quick QuickControls2 Network)

add_library(printer_backend STATIC
    src/escpos.cpp src/escpos.h
    src/printercontroller.cpp src/printercontroller.h
)
target_include_directories(printer_backend PUBLIC src)
target_link_libraries(printer_backend PUBLIC Qt6::Core Qt6::Network)

qt_add_executable(impressora_custom src/main.cpp)
# Shared Qt loads QML plugins at runtime. Import scanning is only needed to
# link them into static builds; distro packages may omit their CMake targets.
get_target_property(qt_qml_library_type Qt6::Qml TYPE)
set(qml_import_options)
if(qt_qml_library_type STREQUAL "SHARED_LIBRARY")
    list(APPEND qml_import_options NO_IMPORT_SCAN)
endif()
qt_add_qml_module(impressora_custom
    URI ImpressoraCustom
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES Main.qml
    ${qml_import_options}
)
target_link_libraries(impressora_custom PRIVATE
    printer_backend Qt6::Quick Qt6::QuickControls2
)
set_target_properties(impressora_custom PROPERTIES WIN32_EXECUTABLE TRUE)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(printer_tests tests/printer_tests.cpp)
    target_link_libraries(printer_tests PRIVATE printer_backend Qt6::Test)
    add_test(NAME printer_tests COMMAND printer_tests)
endif()

install(TARGETS impressora_custom RUNTIME DESTINATION bin)
