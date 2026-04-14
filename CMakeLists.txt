cmake_minimum_required(VERSION 3.14)
project(wxFreeCell LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base xrc html xml)

if(NOT WIN32)
    include(${wxWidgets_USE_FILE})
endif()

# Build bin2c host tool (preserved for non-XRC binary embedding tasks).
set(BIN2C ${CMAKE_BINARY_DIR}/bin2c)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(HostCompile)
host_compile(${CMAKE_CURRENT_SOURCE_DIR}/bin2c.c ${BIN2C})

# Find wxrc tool
find_program(WXRC_EXECUTABLE wxrc)
if(NOT WXRC_EXECUTABLE)
    message(FATAL_ERROR "wxrc not found. Install wxWidgets tools.")
endif()

# Compile .xrc -> .xrs (binary XRC with embedded bitmaps)
set(XRS_FILE ${CMAKE_BINARY_DIR}/freecell.xrs)
add_custom_command(
    OUTPUT ${XRS_FILE}
    COMMAND ${WXRC_EXECUTABLE} -o ${XRS_FILE} freecell.xrc
    DEPENDS ${CMAKE_SOURCE_DIR}/freecell.xrc
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Compiling freecell.xrc -> freecell.xrs (embedding bitmaps)"
)

# Convert .xrs -> .h via bin2c
set(XRS_HEADER ${CMAKE_BINARY_DIR}/freecell_xrs.h)
add_custom_command(
    OUTPUT ${XRS_HEADER}
    COMMAND ${BIN2C} ${XRS_FILE} ${XRS_HEADER} freecell_xrs
    DEPENDS ${XRS_FILE} ${BIN2C}
    COMMENT "Converting freecell.xrs -> freecell_xrs.h via bin2c"
)

# Source files
set(SOURCES
    wx_freecell_app.cpp
    wx_freecell_frame.cpp
    wx_freecell_panel.cpp
    wx_freecell_game.cpp
    wx_freecell_cards.cpp
    wx_freecell_dialogs.cpp
)

set(HEADERS
    wx_freecell_app.h
    wx_freecell_frame.h
    wx_freecell_panel.h
    wx_freecell_game.h
    wx_freecell_cards.h
    wx_freecell_dialogs.h
    wx_freecell.h
)

if(APPLE)
    set(ICON_FILE ${CMAKE_SOURCE_DIR}/wxfreecell.icns)
    set_source_files_properties(${ICON_FILE} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    add_executable(wxFreeCell MACOSX_BUNDLE
        ${SOURCES} ${HEADERS} ${XRS_HEADER} ${ICON_FILE})
    set_target_properties(wxFreeCell PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "wxFreeCell"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.wxfreecell.app"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_ICON_FILE "wxfreecell.icns")
elseif(WIN32)
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/wxfreecell.rc)
    add_executable(wxFreeCell WIN32 ${SOURCES} ${HEADERS} ${XRS_HEADER})
elseif(UNIX)
    add_executable(wxFreeCell ${SOURCES} ${HEADERS} ${XRS_HEADER})
    install(TARGETS wxFreeCell RUNTIME DESTINATION bin)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxfreecell.desktop      DESTINATION share/applications)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxfreecell.metainfo.xml DESTINATION share/metainfo)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sizes/                    DESTINATION share/icons/hicolor)
else()
    add_executable(wxFreeCell ${SOURCES} ${HEADERS} ${XRS_HEADER})
endif()

# The generated header lives in the build directory
target_include_directories(wxFreeCell PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(wxFreeCell PRIVATE ${wxWidgets_LIBRARIES})
