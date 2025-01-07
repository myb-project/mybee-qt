#
# Build deployment package
#
include(../common.pri)

win32 {
    OS_DEPLOY_DIR = $${PWD}/windows
    COPIES += windowsDll
    windowsDll.files = $$files($${OS_DEPLOY_DIR}/*.dll)
    windowsDll.path = "$${OUT_PWD}/$${APP_STAGING_DIR}"

    INNO_SETUP_EXEC = "C:\\Program Files \(x86\)\\Inno Setup 6\\iscc.exe"
    INNO_SETUP_FILE = "inno_setup.iss"
    !exists("$${INNO_SETUP_EXEC}"): error("No executable \"$${INNO_SETUP_EXEC}\" found in the System")
    INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppName \"$${APP_DISPLAYNAME}\""
    INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppVersion \"$${APP_VERSION}\""
    INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppPublisher \"$${APP_PUBLISHER}\""
    INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppURL \"$${APP_HOMEURL}\""
    INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppExeName \"$${APP_NAME}.exe\""
    INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppIconFile \"$$shell_path($${OS_DEPLOY_DIR}/logo.ico)\""
    INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppDeployFiles \"$$shell_path($${APP_STAGING_DIR}/*)\""
    INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppOutputName \"$${APP_RELEASE}\""
    INNO_SETUP_DATA += "$${LITERAL_HASH}define MyWizardImageLarge \"$$shell_path($${OS_DEPLOY_DIR}/inno_setup_large.bmp)\""
    INNO_SETUP_DATA += "$${LITERAL_HASH}define MyWizardImageSmall \"$$shell_path($${OS_DEPLOY_DIR}/inno_setup_small.bmp)\""
    INNO_SETUP_DATA += $$cat($${OS_DEPLOY_DIR}/$${INNO_SETUP_FILE}, blob)
    !write_file($${OUT_PWD}/$${INNO_SETUP_FILE}, INNO_SETUP_DATA): error("Can't write file")

    DEPLOYQT_TOOL = $$[QT_HOST_BINS]/windeployqt
    !exists("$${DEPLOYQT_TOOL}"): DEPLOYQT_TOOL = $$system(where windeployqt)
    !isEmpty(DEPLOYQT_TOOL) {
        QMAKE_POST_LINK = $${DEPLOYQT_TOOL} --release --force --no-compiler-runtime --no-system-d3d-compiler --no-translations \
            --qtpaths \"$$[QT_HOST_BINS]/qtpaths.exe\" --qmldir \"$${APP_SOURCE_DIR}/qml\" --dir $${APP_STAGING_DIR} $${APP_NAME}.exe
        QMAKE_POST_LINK += $$escape_expand(\\n) $$quote("\"$${INNO_SETUP_EXEC}\"" $${INNO_SETUP_FILE})
    }
} else: macx {
    #CODESIGN_CERT = "Developer ID Application: Ivan Ivanov (QWERTY1234)"
    OS_DEPLOY_DIR = $${PWD}/macos

    DEPLOYQT_TOOL = $$[QT_HOST_BINS]/macdeployqt6
    !exists("$${DEPLOYQT_TOOL}"): DEPLOYQT_TOOL = $$system(which macdeployqt)
    !isEmpty(DEPLOYQT_TOOL) {
        message("Make sure that $${DEPLOYQT_TOOL} is used from the Qt library of the MacOS/arm64 version!")

        QMAKE_POST_LINK = $${DEPLOYQT_TOOL} $${APP_NAME}.app -qmldir=\"$${APP_SOURCE_DIR}/qml\" -always-overwrite
        isEmpty(CODESIGN_CERT) {
            QMAKE_POST_LINK += -dmg
        } else {
            QMAKE_POST_LINK += -dmg -codesign=\"$${CODESIGN_CERT}\" -timestamp
            #QMAKE_POST_LINK += -sign-for-notarization=\"$${CODESIGN_CERT}\"
            #exists($${OS_DEPLOY_DIR}/exclude.libs) {
            #    exclude_libs = $$cat($${OS_DEPLOY_DIR}/exclude.libs)
            #    for(exclude_lib, exclude_libs) {
            #        REMOVE_LIBS += $${APP_NAME}.app/$${exclude_lib}
            #    }
            #    !isEmpty(REMOVE_LIBS): QMAKE_POST_LINK += && rm -rf $${REMOVE_LIBS}
            #}
            #QMAKE_POST_LINK += && hdiutil create -volname \"$${APP_DISPLAYNAME}-$${APP_VERSION}\" -srcfolder $${APP_NAME}.app -ov -format ULMO $${APP_RELEASE}.dmg
            #!isEmpty(CODESIGN_CERT): QMAKE_POST_LINK += && codesign --options runtime --timestamp -s \"$${CODESIGN_CERT}\" $${APP_RELEASE}.dmg
        }
    }
} else: !android|ios {
    # Any other unix exclude Android and iOS
    OS_DEPLOY_DIR = $${PWD}/linux

    DESKTOP_FILE = $${APP_STAGING_DIR}/usr/share/applications/$${APP_NAME}.desktop
    QMAKE_POST_LINK = $${QMAKE_MKDIR} $${APP_STAGING_DIR}/usr/bin $${APP_STAGING_DIR}/usr/lib && \
        $${COPY_COMMAND} $${APP_NAME} $${APP_STAGING_DIR}/usr/bin

    DESKTOP_ENTRY += "[Desktop Entry]"
    DESKTOP_ENTRY += "Name=$${APP_DISPLAYNAME} $${APP_VERSION}"
    DESKTOP_ENTRY += "Comment=$${APP_PUBLISHER} - $${APP_HOMEURL}"
    DESKTOP_ENTRY += "Categories=Network;Utility;"
    DESKTOP_ENTRY += "Exec=$${APP_NAME}"
    DESKTOP_ENTRY += "Icon=$${APP_NAME}"
    DESKTOP_ENTRY += "Type=Application"
    DESKTOP_ENTRY += "Terminal=false"
    DESKTOP_ENTRY += "StartupNotify=true"
    !write_file($${OUT_PWD}/$${DESKTOP_FILE}, DESKTOP_ENTRY): error("Can't write file")

    ICONS_HICOLOR = $${APP_STAGING_DIR}/usr/share/icons/hicolor
    ICONS_SOURCES = $$files($${OS_DEPLOY_DIR}/icons/*)
    for(icon_file, ICONS_SOURCES) {
        icon_name = $$basename(icon_file)
        icon_parts = $$split(icon_name, .)
        icon_apps_dir = "$${ICONS_HICOLOR}/$$first(icon_parts)/apps"
        QMAKE_POST_LINK += && $${QMAKE_MKDIR} $${icon_apps_dir} && \
            $${COPY_COMMAND} \"$${icon_file}\" $${icon_apps_dir}/$${APP_NAME}.$$last(icon_parts)
    }

    DEPLOYQT_TOOL = $$[QT_HOST_BINS]/linuxdeployqt
    !exists("$${DEPLOYQT_TOOL}"): DEPLOYQT_TOOL = $$system(which linuxdeployqt)
    !isEmpty(DEPLOYQT_TOOL) {
        QMAKE_POST_LINK += && $${DEPLOYQT_TOOL} $${DESKTOP_FILE} -qmake=\"$${QMAKE_QMAKE}\" -qmldir=\"$${APP_SOURCE_DIR}/qml\" \
            -appimage -always-overwrite -extra-plugins=platformthemes -no-copy-copyright-files -no-translations
        exists($${OS_DEPLOY_DIR}/exclude.libs) {
            exclude_libs = $$cat($${OS_DEPLOY_DIR}/exclude.libs)
            exclude_split = $$split(exclude_libs)
            QMAKE_POST_LINK += -exclude-libs=$$join(exclude_split, ",")
        }
    }
}

isEmpty(DEPLOYQT_TOOL) {
    warning("The [OS]deployqt not found, so the deployment package will not be generated")
}
