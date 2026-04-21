cmake_minimum_required(VERSION 3.16)
project(CRTSimulator VERSION 2.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CRT_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets/branding")
set(CRT_LINUX_PACKAGING_DIR "${CMAKE_SOURCE_DIR}/packaging/linux")
set(CRT_MACOS_PACKAGING_DIR "${CMAKE_SOURCE_DIR}/packaging/macos")
set(CRT_WINDOWS_PACKAGING_DIR "${CMAKE_SOURCE_DIR}/packaging/windows")
set(CRT_LOGO_PNG "${CRT_ASSETS_DIR}/logo.png")
set(CRT_APP_ID "com.mechaml.crtemulator")
set(CRT_APP_DISPLAY_NAME "CRT_EMULATOR")
set(CRT_APP_DESCRIPTION "CRT physics emulator")

if(APPLE)
    set(CRT_MACOS_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist")
    configure_file("${CRT_MACOS_PACKAGING_DIR}/Info.plist.in"
                   "${CRT_MACOS_INFO_PLIST}" @ONLY)
endif()

if(WIN32)
    enable_language(RC)
    find_program(POWERSHELL_EXECUTABLE NAMES pwsh powershell REQUIRED)
    set(APP_ICON_ICO "${CMAKE_BINARY_DIR}/app_icon.ico")
    file(TO_NATIVE_PATH "${APP_ICON_ICO}" APP_ICON_NATIVE_PATH)
    string(REPLACE "\\" "\\\\" APP_ICON_RC_PATH "${APP_ICON_NATIVE_PATH}")
    configure_file("${CRT_WINDOWS_PACKAGING_DIR}/app_icon.rc.in"
                   "${CMAKE_BINARY_DIR}/app_icon.rc" @ONLY)
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/crtrenderer.h
    src/crtrenderer.cpp
    src/localization.h
    src/physics.h
    src/physics.cpp
    src/telemetrypanel.h
    src/telemetrypanel.cpp
    src/app_resources.qrc
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

if(WIN32)
    add_custom_command(
        OUTPUT "${APP_ICON_ICO}"
        COMMAND "${POWERSHELL_EXECUTABLE}" -ExecutionPolicy Bypass -File "${CRT_WINDOWS_PACKAGING_DIR}/generate_icon.ps1"
                -InputPng "${CRT_LOGO_PNG}"
                -OutputIco "${APP_ICON_ICO}"
        DEPENDS "${CRT_LOGO_PNG}" "${CRT_WINDOWS_PACKAGING_DIR}/generate_icon.ps1"
        VERBATIM
    )
    add_custom_target(generate_app_icon DEPENDS "${APP_ICON_ICO}")
    add_dependencies(${PROJECT_NAME} generate_app_icon)
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_BINARY_DIR}/app_icon.rc")
endif()

if(APPLE)
    find_program(SH_EXECUTABLE NAMES sh REQUIRED)
    find_program(SIPS_EXECUTABLE NAMES sips)
    find_program(ICONUTIL_EXECUTABLE NAMES iconutil)

    if(SIPS_EXECUTABLE AND ICONUTIL_EXECUTABLE)
        set(CRT_APP_ICON_ICNS "${CMAKE_BINARY_DIR}/CRT_EMULATOR.icns")
        add_custom_command(
            OUTPUT "${CRT_APP_ICON_ICNS}"
            COMMAND "${SH_EXECUTABLE}" "${CRT_MACOS_PACKAGING_DIR}/generate_icns.sh"
                    "${CRT_LOGO_PNG}"
                    "${CRT_APP_ICON_ICNS}"
                    "${SIPS_EXECUTABLE}"
                    "${ICONUTIL_EXECUTABLE}"
            DEPENDS "${CRT_LOGO_PNG}" "${CRT_MACOS_PACKAGING_DIR}/generate_icns.sh"
            VERBATIM
        )
        add_custom_target(generate_macos_icon DEPENDS "${CRT_APP_ICON_ICNS}")
        add_dependencies(${PROJECT_NAME} generate_macos_icon)
        set_source_files_properties("${CRT_APP_ICON_ICNS}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
        target_sources(${PROJECT_NAME} PRIVATE "${CRT_APP_ICON_ICNS}")
    else()
        message(WARNING "sips/iconutil were not found. The macOS bundle will be created without a custom icon.")
    endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "CRT_EMULATOR"
)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    set(CRT_MACOS_BUNDLE_ICON_FILE "")
    if(DEFINED CRT_APP_ICON_ICNS)
        set(CRT_MACOS_BUNDLE_ICON_FILE "CRT_EMULATOR.icns")
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "${CRT_APP_DISPLAY_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${CRT_APP_ID}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_PLIST "${CRT_MACOS_INFO_PLIST}"
        MACOSX_BUNDLE_ICON_FILE "${CRT_MACOS_BUNDLE_ICON_FILE}"
    )
endif()

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    set(CRT_LINUX_DESKTOP_FILE "${CMAKE_BINARY_DIR}/${CRT_APP_ID}.desktop")
    configure_file("${CRT_LINUX_PACKAGING_DIR}/CRT_EMULATOR.desktop.in"
                   "${CRT_LINUX_DESKTOP_FILE}" @ONLY)

    install(FILES "${CRT_LINUX_DESKTOP_FILE}"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    )
    install(FILES "${CRT_LINUX_PACKAGING_DIR}/${CRT_APP_ID}.appdata.xml"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo"
    )
    install(FILES "${CRT_LOGO_PNG}"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps"
        RENAME "${CRT_APP_ID}.png"
    )
    install(FILES "${CRT_LOGO_PNG}"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/pixmaps"
        RENAME "${CRT_APP_ID}.png"
    )
endif()

qt_generate_deploy_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT CRT_DEPLOY_SCRIPT
    CONTENT "
if(APPLE AND NOT IOS)
    qt_deploy_runtime_dependencies(
        EXECUTABLE \"$<TARGET_FILE_NAME:${PROJECT_NAME}>.app\"
    )
elseif(WIN32 OR (UNIX AND NOT APPLE AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING))
    qt_deploy_runtime_dependencies(
        EXECUTABLE \"$<TARGET_FILE:${PROJECT_NAME}>\"
        GENERATE_QT_CONF
    )
endif()
"
)
install(SCRIPT "${CRT_DEPLOY_SCRIPT}")

set(CPACK_PACKAGE_NAME "${CRT_APP_DISPLAY_NAME}")
set(CPACK_PACKAGE_VENDOR "MechaML Researchers")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${CRT_APP_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CRT_APP_DISPLAY_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /utf-8)
endif()
