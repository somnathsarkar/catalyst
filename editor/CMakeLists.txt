list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.2.3/msvc2019_64")
find_package(Qt6 REQUIRED COMPONENTS Widgets)
get_target_property(QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)

add_executable(editor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

target_link_libraries(editor PRIVATE Qt6::Widgets)
target_link_libraries(editor PRIVATE catalyst)

function(windeployqt target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${QT_BIN_DIR}/windeployqt.exe"         
                --verbose 1
                --debug
                --no-svg
                --no-opengl
                --no-opengl-sw
                --no-compiler-runtime
                --no-system-d3d-compiler
                \"$<TARGET_FILE:${target}>\"
        COMMENT "Deploying Qt libraries using windeployqt for compilation target '${target}' ..."
    )
endfunction()

windeployqt(editor)

target_sources(editor PRIVATE
"main.cpp"
"window/editorwindow.cc"
"window/editorwindow.h"
"window/qtwindow.h"
"window/qtwindow.cc"
"window/qtviewportwindow.h"
"window/qtviewportwindow.cc"
"script/editorscript.h"
"script/editorscript.cc"
"script/freecamera.h"
"script/freecamera.cc"
"window/qtscenetree.h"
"window/qtscenetree.cc"
"window/qtpropertiespanel.h"
"window/qtpropertiespanel.cc"
"../assets/theme/style.qrc"
"window/qtresourcepanel.h"
"window/qtresourcepanel.cc")