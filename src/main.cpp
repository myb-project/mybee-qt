#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlFileSelector>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QVersionNumber>
#include <QLibraryInfo>
#include <QSettings>
#include <QLocale>
#include <QTranslator>
#include <QDirIterator>
#include <QFontDatabase>
#include <QLoggingCategory>
#ifdef Q_OS_ANDROID
#include <QStandardPaths>
#endif

#include "libssh/libsshpp.hpp" // just for ssh_init()/ssh_finalize()
#include "DesktopView.h"
#include "HttpRequest.h"
#include "KeyLoader.h"
#include "TextRender.h"
#include "SshProcess.h"
#include "SshSession.h"
#include "SystemHelper.h"
#include "SystemProcess.h"
#include "UrlModel.h"

#ifndef QML_CUSTOM_MODULES
#define QML_CUSTOM_MODULES "QmlCustomModules"
#endif
#ifndef CPP_CUSTOM_MODULES
#define CPP_CUSTOM_MODULES "CppCustomModules"
#endif

#if defined(__mobile__)
const bool isMobile = true;
static const char *quickControlStyle = "Material";
static const char *materialStyleMode = "Normal";
#else
const bool isMobile = false;
static const char *quickControlStyle = "Material";
static const char *materialStyleMode = "Dense";
#endif
static const char *embeddedFontFamily = "Roboto";

#ifdef Q_OS_ANDROID
static const char *android_permission[] = { // must match android/AndroidManifest.xml !!!
    "android.permission.ACCESS_NETWORK_STATE",
    "android.permission.ACCESS_WIFI_STATE",
    "android.permission.CHANGE_WIFI_MULTICAST_STATE",
    "android.permission.FOREGROUND_SERVICE",
    "android.permission.INTERNET",
    "android.permission.QUERY_ALL_PACKAGES",
    "android.permission.RECEIVE_BOOT_COMPLETED",
    "android.permission.WAKE_LOCK"
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtAndroid>
bool setAndroidPermission()
{
    QStringList request;
    for (uint i = 0; i < sizeof(android_permission) / sizeof(android_permission[0]); i++) {
        if (QtAndroid::checkPermission(android_permission[i]) == QtAndroid::PermissionResult::Denied)
            request += android_permission[i];
    }
    if (!request.isEmpty()) {
        auto map = QtAndroid::requestPermissionsSync(request);
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.value() == QtAndroid::PermissionResult::Denied) {
                qWarning() << it.key() << "Required permissions denied!";
                return false;
            }
        }
    }
    return true;
}
#else
bool setAndroidPermission()
{
    QStringList request;
    for (uint i = 0; i < sizeof(android_permission) / sizeof(android_permission[0]); i++) {
        auto result = QtAndroidPrivate::checkPermission(android_permission[i])
                          .then([](QtAndroidPrivate::PermissionResult result) { return result; });
        result.waitForFinished();
        if (result.result() == QtAndroidPrivate::PermissionResult::Denied)
            request += android_permission[i];
    }
    if (!request.isEmpty()) {
        for (const auto &perm : request) {
            auto result = QtAndroidPrivate::requestPermission(perm)
                              .then([](QtAndroidPrivate::PermissionResult result) { return result; });
            result.waitForFinished();
            if (result.result() == QtAndroidPrivate::PermissionResult::Denied) {
                qWarning() << perm << "Required permissions denied!";
                return false;
            }
        }
    }
    return true;
}
#endif
#endif

static void setCbsdSearchPaths()
{
    static const char *search_paths[] = { // in reverse order
        "/usr/local/bin", "/usr/local/sbin", "/usr/bin", "/usr/sbin", "/bin", "/sbin"
    };

    bool modify = false;
    QStringList env_path = qEnvironmentVariable("PATH").split(QDir::listSeparator(), Qt::SkipEmptyParts);
    for (uint i = 0; i < sizeof(search_paths) / sizeof(search_paths[0]); i++) {
         if (!env_path.contains(search_paths[i])) {
             env_path.prepend(search_paths[i]);
             modify = true;
         }
    }
    if (modify) {
        env_path.removeDuplicates();
        qputenv("PATH", env_path.join(QDir::listSeparator()).toLatin1());
    }
}

int main(int argc, char *argv[])
{
    if (::ssh_init() != SSH_OK) {
        qFatal("Can't initialize libssh");
        return -1;
    }

    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE"))
        qputenv("QT_QUICK_CONTROLS_STYLE", quickControlStyle);

    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_MATERIAL_VARIANT"))
        qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", materialStyleMode);

    qputenv("NOCOLOR", "1"); // disable ANSI colors code for CBSD commands
    setCbsdSearchPaths();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if defined(__mobile__)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#else
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
#endif
    QGuiApplication app(argc, argv);

    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION);
    app.setApplicationDisplayName(SystemHelper::camelCase(app.applicationName(), '-'));
    app.setWindowIcon(QIcon(QStringLiteral(":/image-box")));
    QQuickStyle::setStyle(quickControlStyle);

#ifdef Q_OS_ANDROID
    setAndroidPermission();

    app.setOrganizationDomain(QStringLiteral("settings")); // just a fake for QSettings
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
#else
    app.setOrganizationDomain(app.applicationName());
    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif

    bool set_font = false;
    QDirIterator font_it(QStringLiteral(":/"), QStringList() << "font-*");
    while (font_it.hasNext()) {
        QString path = font_it.next();
        int font_id = QFontDatabase::addApplicationFont(path);
        if (!set_font) {
            auto font_families = QFontDatabase::applicationFontFamilies(font_id);
            set_font = font_families.contains(QLatin1String(embeddedFontFamily));
        }
    }
    if (set_font) {
        QFont sys_font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
        QFont app_font(QLatin1String(embeddedFontFamily), sys_font.pointSize(), sys_font.weight());
        app_font.setStyleStrategy(QFont::PreferAntialias);
        app.setFont(app_font);
    }

    QDirIterator res_it(QStringLiteral(":/"), QStringList() << KeyLoader::keyLayoutFilter << "*.wav");
    while (res_it.hasNext()) {
        QString path = res_it.next();
        SystemHelper::copyFile(path, res_it.fileName());
    }

    QString locale;
    QTranslator translator;
    const QStringList languages = QLocale::system().uiLanguages();
    for (const QString &lang : languages) {
        if (translator.load(QString(":/i18n/%1-%2").arg(APP_NAME, QLocale(lang).name()))) {
            locale = QLocale(lang).name();
            app.installTranslator(&translator);
            break;
        }
    }
    KeyLoader keyLoader(SystemHelper::appDataDir());
    qmlRegisterSingletonInstance(CPP_CUSTOM_MODULES, 1, 0, "KeyLoader", &keyLoader);
    if (!keyLoader.loadLayout(locale) && !keyLoader.loadLayout())
        qFatal("Failure loading keyboard layout");

    QLoggingCategory::setFilterRules(QStringLiteral("qtc.ssh=true"));

    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/MaterialSet.qml")), QML_CUSTOM_MODULES, 1, 0, "MaterialSet");
    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/RestApiSet.qml")), QML_CUSTOM_MODULES, 1, 0, "RestApiSet");
    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/VMConfigSet.qml")), QML_CUSTOM_MODULES, 1, 0, "VMConfigSet");

    qmlRegisterSingletonType<SystemHelper>(CPP_CUSTOM_MODULES, 1, 0, "SystemHelper",
                                          [](QQmlEngine*, QJSEngine*)->QObject* { return new SystemHelper(); });
    qmlRegisterSingletonType<SystemProcess>(CPP_CUSTOM_MODULES, 1, 0, "SystemProcess",
                                            [](QQmlEngine*, QJSEngine*)->QObject* { return new SystemProcess(); });
    qmlRegisterSingletonType<SshProcess>(CPP_CUSTOM_MODULES, 1, 0, "SshProcess",
                                         [](QQmlEngine*, QJSEngine*)->QObject* { return new SshProcess(); });
    qmlRegisterSingletonType<HttpRequest>(CPP_CUSTOM_MODULES, 1, 0, "HttpRequest",
                                          [](QQmlEngine*, QJSEngine*)->QObject* { return new HttpRequest(); });
    qmlRegisterSingletonType<UrlModel>(CPP_CUSTOM_MODULES, 1, 0, "Url",
                                       [](QQmlEngine*, QJSEngine*)->QObject* { return new UrlModel(); });

    qmlRegisterType<DesktopView>(CPP_CUSTOM_MODULES, 1, 0, "DesktopView");
    qmlRegisterType<SshSession>(CPP_CUSTOM_MODULES, 1, 0, "SshSession");
    qmlRegisterType<TextRender>(CPP_CUSTOM_MODULES, 1, 0, "TextRender");
    qmlRegisterType<UrlModel>(CPP_CUSTOM_MODULES, 1, 0, "UrlModel");

    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();

    const QString appSettingsPath(QSettings().fileName());
    context->setContextProperty(QStringLiteral("appSettingsPath"), appSettingsPath);
    context->setContextProperty(QStringLiteral("isMobile"), isMobile);

    const QVersionNumber qt_ver = QLibraryInfo::version(); // Qt run-time version
    if (qt_ver.majorVersion() > 5) {
        QStringList efs;
        efs.append(QStringLiteral("qt") + QString::number(qt_ver.majorVersion()));
        efs.append(QStringLiteral("mobile"));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QQmlFileSelector::get(&engine)->setExtraSelectors(efs);
#else
        engine.setExtraFileSelectors(efs);
#endif
    }
    QString qtRunningVersion = qVersion();
    if (qtRunningVersion != QT_VERSION_STR) qtRunningVersion += QString(" (build on %1)").arg(QT_VERSION_STR);
    context->setContextProperty(QStringLiteral("qtVersion"), qtRunningVersion);

    const QUrl qml(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [qml](QObject *obj, const QUrl &url) {
        if (!obj && qml == url) QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(qml);
    int rc = app.exec();

    ::ssh_finalize();
    return rc;
}
