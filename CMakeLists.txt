cmake_minimum_required(VERSION 3.16)

project(Pl0_Compiler_Suite LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core)

add_executable(pl0_core
    src_compiler/pl0.c
    src_compiler/strtonum.c
    src_compiler/strtonum.h
)
if(UNIX)
    target_link_libraries(pl0_core m)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(Pl0Studio
    src_gui/main.cpp
    src_gui/MainWindow.h
    src_gui/MainWindow.cpp
    src_gui/Highlighter.h
    src_gui/Highlighter.cpp
    src_gui/CodeEditor.h    # New
    src_gui/CodeEditor.cpp  # New
)

target_link_libraries(Pl0Studio PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

add_custom_command(TARGET Pl0Studio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:pl0_core>
    $<TARGET_FILE_DIR:Pl0Studio>
)
