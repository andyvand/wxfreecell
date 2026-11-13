cmake_minimum_required(VERSION 3.14)

# macOS: build a universal (Apple silicon + Intel) binary by default. Must be
# set before project(). Not for cross builds (Android, Retro68), which come
# with a toolchain file. Override with -DCMAKE_OSX_ARCHITECTURES=arm64 (or
# x86_64) for a single-architecture build, e.g. against a wxWidgets that is
# not universal itself.
if(CMAKE_HOST_APPLE AND NOT CMAKE_TOOLCHAIN_FILE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures to build")
endif()

project(wxFreeCell LANGUAGES C CXX)

# Classic Mac OS (wxRetro68): 68k (Retro68) or PowerPC (RetroPPC).
if(CMAKE_SYSTEM_NAME MATCHES "^Retro(68|PPC)$")
    set(RETRO68 ON)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ANDROID)
    # wxWidgets was cross-built with the wxQt backend against Qt for Android.
    # Point FindwxWidgets at the cross wx-config shipped in the Qt Android prefix
    # (the same prefix that provides Qt6). Overridable from the command line.
    if(NOT wxWidgets_CONFIG_EXECUTABLE)
        set(wxWidgets_CONFIG_EXECUTABLE
            /usr/local/qt-6.9.3-android-arm64/bin/wx-config
            CACHE FILEPATH "Cross wx-config for the Android/wxQt build")
    endif()

    # Qt6 is needed for qt_add_executable() and its androiddeployqt integration.
    # PrintSupport is required because the wxQt core library references
    # QPrinter/QPrinterInfo (wxWidgets printing support).
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)
endif()

if(RETRO68)
    # wxWidgets from the sysroot's wx-config, never the host's: FindwxWidgets
    # is unreliable when cross-compiling and would pick up the host's wx.
    if(NOT wxWidgets_CONFIG_EXECUTABLE)
        set(wxWidgets_CONFIG_EXECUTABLE ${CMAKE_SYSTEM_PREFIX_PATH}/bin/wx-config)
    endif()
    execute_process(COMMAND ${wxWidgets_CONFIG_EXECUTABLE} --cxxflags core base xrc html xml
        OUTPUT_VARIABLE WX_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _wxr1)
    execute_process(COMMAND ${wxWidgets_CONFIG_EXECUTABLE} --libs core base xrc html xml
        OUTPUT_VARIABLE WX_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _wxr2)
    if(_wxr1 OR _wxr2)
        message(FATAL_ERROR "wx-config failed: ${wxWidgets_CONFIG_EXECUTABLE}")
    endif()
    separate_arguments(WX_CXXFLAGS_LIST NATIVE_COMMAND "${WX_CXXFLAGS}")
    separate_arguments(WX_LIBS_LIST NATIVE_COMMAND "${WX_LIBS}")
else()
    # Find wxWidgets
    find_package(wxWidgets REQUIRED COMPONENTS core base xrc html xml)

    if(NOT WIN32)
        include(${wxWidgets_USE_FILE})
    endif()

    # A universal build needs a universal wxWidgets: say so up front rather
    # than with a wall of "symbol(s) not found for architecture" link errors.
    if(APPLE AND NOT ANDROID AND CMAKE_OSX_ARCHITECTURES)
        find_program(LIPO_EXECUTABLE lipo)
        foreach(_lib ${wxWidgets_LIBRARIES})
            if(LIPO_EXECUTABLE AND _lib MATCHES "\\.(a|dylib)$" AND EXISTS "${_lib}")
                execute_process(COMMAND ${LIPO_EXECUTABLE} -archs "${_lib}"
                    OUTPUT_VARIABLE _lib_archs OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)
                separate_arguments(_lib_archs)
                foreach(_arch ${CMAKE_OSX_ARCHITECTURES})
                    if(NOT _arch IN_LIST _lib_archs)
                        message(FATAL_ERROR
                            "${_lib} has no ${_arch} code (it has: ${_lib_archs}). "
                            "Build wxWidgets universal (--enable-universal_binary=arm64,x86_64) "
                            "or configure with -DCMAKE_OSX_ARCHITECTURES=<one architecture>.")
                    endif()
                endforeach()
                break()
            endif()
        endforeach()
    endif()
endif()

# Build bin2c host tool (preserved for non-XRC binary embedding tasks).
set(BIN2C ${CMAKE_BINARY_DIR}/bin2c)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(HostCompile)
host_compile(${CMAKE_CURRENT_SOURCE_DIR}/bin2c.c ${BIN2C})

# Find wxrc tool.
# On Android this is a cross build: the wxrc inside the Qt Android prefix is an
# aarch64 Android ELF and cannot run on the build host. Pin to a native host
# wxrc instead (overridable via -DWXRC_EXECUTABLE=...).
if(ANDROID AND NOT WXRC_EXECUTABLE)
    if(EXISTS /usr/local/bin/wxrc)
        set(WXRC_EXECUTABLE /usr/local/bin/wxrc CACHE FILEPATH "Host wxrc for cross build")
    endif()
endif()
if(RETRO68 AND NOT WXRC_EXECUTABLE)
    # The sysroot's wxrc is a classic Mac program; use the host one.
    find_program(WXRC_EXECUTABLE NAMES wxrc wxrc-3.3 wxrc-3.2
        PATHS /usr/local/bin /opt/homebrew/bin NO_DEFAULT_PATH)
    if(NOT WXRC_EXECUTABLE)
        find_program(WXRC_EXECUTABLE NAMES wxrc wxrc-3.3 wxrc-3.2 NO_CMAKE_SYSTEM_PATH)
    endif()
endif()
if(NOT WXRC_EXECUTABLE)
    find_program(WXRC_EXECUTABLE wxrc)
endif()
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

set(CHM_FILE  ${CMAKE_CURRENT_SOURCE_DIR}/wxfreecell.chm)
set(HELP_BUNDLE ${CMAKE_CURRENT_SOURCE_DIR}/wxfreecell.help)

if(RETRO68)
    # =======================================================================
    #  Classic Mac OS (Retro68): 68k (Retro68) or PowerPC (RetroPPC)
    #
    #  Built against wxRetro68 (wxUniversal on QuickDraw) from the Retro68
    #  sysroot. Produces FreeCell.bin (MacBinary), FreeCell.APPL and
    #  FreeCell.dsk (an HFS disk image) for the one architecture;
    #  retro68/build.sh builds both and merges them into a single fat
    #  application on one disk image.
    # =======================================================================

    # The HTML help, zipped and embedded, shown in a wxHtmlWindow (classic
    # Mac OS has no browser to hand it to).
    set(HELP_SRC_DIR ${HELP_BUNDLE}/Contents/Resources/en.lproj)
    file(GLOB HELP_FILES RELATIVE ${HELP_SRC_DIR}
        ${HELP_SRC_DIR}/*.html ${HELP_SRC_DIR}/*.htm
        ${HELP_SRC_DIR}/css/*.css ${HELP_SRC_DIR}/img/* ${HELP_SRC_DIR}/lib/*)
    list(SORT HELP_FILES)
    set(HELP_DEPS)
    foreach(_f ${HELP_FILES})
        list(APPEND HELP_DEPS ${HELP_SRC_DIR}/${_f})
    endforeach()
    set(HELP_ZIP ${CMAKE_BINARY_DIR}/help.zip)
    add_custom_command(
        OUTPUT ${HELP_ZIP}
        COMMAND ${CMAKE_COMMAND} -E tar cf ${HELP_ZIP} --format=zip ${HELP_FILES}
        WORKING_DIRECTORY ${HELP_SRC_DIR}
        DEPENDS ${HELP_DEPS}
        COMMENT "Zipping FreeCell Help (Retro68)"
    )
    set(HELP_DATA_H ${CMAKE_BINARY_DIR}/help_data.h)
    add_custom_command(
        OUTPUT ${HELP_DATA_H}
        COMMAND ${BIN2C} ${HELP_ZIP} ${HELP_DATA_H} help_data
        DEPENDS ${HELP_ZIP} ${BIN2C}
        COMMENT "Embedding FreeCell Help as C array (Retro68)"
    )

    add_executable(wxFreeCell ${SOURCES} ${HEADERS} ${XRS_HEADER} ${HELP_DATA_H}
        ${CMAKE_SOURCE_DIR}/retro68/help_retro68.cpp)
    target_compile_options(wxFreeCell PRIVATE ${WX_CXXFLAGS_LIST}
        -ffunction-sections -fdata-sections)
    target_link_options(wxFreeCell PRIVATE -Wl,--gc-sections)
    target_link_libraries(wxFreeCell PRIVATE
        -Wl,--start-group ${WX_LIBS_LIST} -Wl,--end-group)
    # Relink when the wx libraries change (wx-config gives flags, so CMake
    # does not know about them): the full paths, plus -l names found in the
    # -L directories.
    set(WX_LIB_FILES)
    set(_wx_lib_dirs)
    foreach(_arg ${WX_LIBS_LIST})
        if(_arg MATCHES "^-L(.+)$")
            list(APPEND _wx_lib_dirs "${CMAKE_MATCH_1}")
        elseif(_arg MATCHES "\\.a$" AND EXISTS "${_arg}")
            list(APPEND WX_LIB_FILES "${_arg}")
        elseif(_arg MATCHES "^-l(.+)$")
            foreach(_dir ${_wx_lib_dirs})
                if(EXISTS "${_dir}/lib${CMAKE_MATCH_1}.a")
                    list(APPEND WX_LIB_FILES "${_dir}/lib${CMAKE_MATCH_1}.a")
                    break()
                endif()
            endforeach()
        endif()
    endforeach()
    list(REMOVE_DUPLICATES WX_LIB_FILES)
    set_property(TARGET wxFreeCell APPEND PROPERTY LINK_DEPENDS ${WX_LIB_FILES})

    # -DFREECELL_TRACE=ON: log startup to "wxtest.log" next to the
    # application and shut the Mac down once it is up, for unattended
    # emulator runs (retro68/run-basilisk.sh).
    option(FREECELL_TRACE "Trace startup to wxtest.log (classic Mac OS testing)" OFF)
    if(FREECELL_TRACE)
        target_sources(wxFreeCell PRIVATE ${CMAKE_SOURCE_DIR}/retro68/trace_retro68.cpp)
        target_compile_definitions(wxFreeCell PRIVATE FREECELL_TRACE=1)
    endif()

    set(FREECELL_REZ_DEFINES)
    if(CMAKE_SYSTEM_NAME STREQUAL "RetroPPC")
        set(FREECELL_REZ_DEFINES FREECELL_PPC=1)
    endif()

    include(Retro68Application)
    retro68_application(wxFreeCell
        NAME "FreeCell"
        CREATOR WXFC
        RESOURCES ${CMAKE_SOURCE_DIR}/retro68/wxFreeCell.r
        REZ_INCLUDES ${CMAKE_SOURCE_DIR}/retro68
        REZ_DEFINES ${FREECELL_REZ_DEFINES})
elseif(APPLE)
    # macOS-only Objective-C++ help launcher (implements ShowMacHelpBook).
    list(APPEND SOURCES help_mac.mm)
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
        MACOSX_BUNDLE_ICON_FILE "wxfreecell.icns"
        MACOSX_BUNDLE_HELP_BOOK_NAME "wxfreecell Help"
        MACOSX_BUNDLE_HELP_BOOK_FOLDER "wxfreecell.help")
    add_custom_command(TARGET wxFreeCell POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${HELP_BUNDLE}
            $<TARGET_BUNDLE_CONTENT_DIR:wxFreeCell>/Resources/wxfreecell.help
        COMMENT "Copying wxfreecell.help into app bundle Resources")
    # Register the help book in the app's Info.plist so NSApplication
    # showHelp: can find it. CMake's MACOSX_BUNDLE_HELP_BOOK_* target
    # properties are not written to the default Info.plist template,
    # so inject them explicitly. Use a generated shell helper so we can
    # swallow "key not present" errors from plutil -remove.
    set(HELP_PLIST_SCRIPT ${CMAKE_BINARY_DIR}/register_help_book.sh)
    file(WRITE ${HELP_PLIST_SCRIPT}
"#!/bin/sh
set -e
PLIST=\"$1\"
/usr/bin/plutil -remove CFBundleHelpBookName   \"$PLIST\" 2>/dev/null || true
/usr/bin/plutil -remove CFBundleHelpBookFolder \"$PLIST\" 2>/dev/null || true
/usr/bin/plutil -insert CFBundleHelpBookName   -string \"com.wxfreecell.app.help\"  \"$PLIST\"
/usr/bin/plutil -insert CFBundleHelpBookFolder -string \"wxfreecell.help\" \"$PLIST\"
")
    add_custom_command(TARGET wxFreeCell POST_BUILD
        COMMAND /bin/sh ${HELP_PLIST_SCRIPT}
            $<TARGET_BUNDLE_CONTENT_DIR:wxFreeCell>/Info.plist
        COMMENT "Registering Help Book keys in Info.plist")
elseif(WIN32)
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/wxfreecell.rc)
    add_executable(wxFreeCell WIN32 ${SOURCES} ${HEADERS} ${XRS_HEADER})
    add_custom_command(TARGET wxFreeCell POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CHM_FILE}
            $<TARGET_FILE_DIR:wxFreeCell>/wxfreecell.chm
        COMMENT "Copying wxfreecell.chm next to executable")
    install(TARGETS wxFreeCell RUNTIME DESTINATION .)
    install(FILES ${CHM_FILE} DESTINATION .)
elseif(ANDROID)
    # Android also satisfies UNIX, so this branch must precede the UNIX one.
    # On Android qt_add_executable() produces a shared library
    # (libwxFreeCell_arm64-v8a.so) and wires up the deployment-settings JSON and
    # the `apk`/`aab` build targets that drive androiddeployqt.
    # Android-only compatibility shims (see android_compat.cpp).
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/android_compat.cpp)
    qt_add_executable(wxFreeCell ${SOURCES} ${HEADERS} ${XRS_HEADER})
    set_target_properties(wxFreeCell PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/android
        QT_ANDROID_TARGET_SDK_VERSION 34
        QT_ANDROID_MIN_SDK_VERSION 28
        QT_ANDROID_ABIS "arm64-v8a")
    target_link_libraries(wxFreeCell PRIVATE Qt6::Widgets Qt6::PrintSupport)
elseif(UNIX)
    add_executable(wxFreeCell ${SOURCES} ${HEADERS} ${XRS_HEADER})
    install(TARGETS wxFreeCell RUNTIME DESTINATION bin)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxfreecell.desktop      DESTINATION share/applications)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxfreecell.metainfo.xml DESTINATION share/metainfo)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sizes/                  DESTINATION share/icons/hicolor)
    install(FILES     ${CHM_FILE}                                         DESTINATION share/wxfreecell)
    install(FILES     ${CMAKE_CURRENT_SOURCE_DIR}/wxfreecell.6            DESTINATION share/man/man6)
else()
    add_executable(wxFreeCell ${SOURCES} ${HEADERS} ${XRS_HEADER})
endif()

# The generated headers live in the build directory
target_include_directories(wxFreeCell PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

if(NOT RETRO68)
    target_link_libraries(wxFreeCell PRIVATE ${wxWidgets_LIBRARIES})
endif()
