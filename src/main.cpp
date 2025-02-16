#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlFileSelector>
#include <QQuickStyle>
#include <QIcon>
#include <QVersionNumber>
#include <QLibraryInfo>
#include <QSettings>
#include <QLocale>
#include <QTranslator>
#include <QDirIterator>
#ifdef Q_OS_ANDROID
#include <QStandardPaths>
#endif

#include <QFileSelector>

#include "libssh/libsshpp.hpp" // just for ssh_init()/ssh_finalize()
#include "DirSpaceUsed.h"
#include "DesktopView.h"
#include "HttpRequest.h"
#include "TextRender.h"
#include "SshProcess.h"
#include "SshSession.h"
#include "SystemHelper.h"
#include "SystemProcess.h"
#include "UrlModel.h"
#include "WindowState.h"

#ifndef QML_CUSTOM_MODULES
#define QML_CUSTOM_MODULES "QmlCustomModules"
#endif
#ifndef CPP_CUSTOM_MODULES
#define CPP_CUSTOM_MODULES "CppCustomModules"
#endif

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
static const char *quickControlStyle = "Material";
static const char *materialStyleMode = "Normal";
#else
static const char *quickControlStyle = "Material";
static const char *materialStyleMode = "Dense";
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
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#else
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
#endif
    QGuiApplication app(argc, argv);

    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION);
    app.setApplicationDisplayName(APP_DISPLAYNAME);
    app.setWindowIcon(QIcon(QStringLiteral(":/image-logo")));
    QQuickStyle::setStyle(quickControlStyle);

#ifdef Q_OS_ANDROID
    app.setOrganizationDomain(QStringLiteral("settings")); // just a fake for QSettings
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
#else
    app.setOrganizationDomain(app.applicationName());
    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif

    QDirIterator res_it(QStringLiteral(":/resource"), QDir::Files);
    while (res_it.hasNext()) {
        QString path = res_it.next();
        SystemHelper::copyFile(path, res_it.fileName());
    }

    QTranslator translator;
    const QStringList languages = QLocale::system().uiLanguages();
    for (const QString &lang : languages) {
        if (translator.load(QString(":/i18n/%1-%2").arg(APP_NAME, QLocale(lang).name()))) {
            app.installTranslator(&translator);
            break;
        }
    }

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
    qmlRegisterSingletonType<WindowState>(CPP_CUSTOM_MODULES, 1, 0, "WindowState",
                                          [](QQmlEngine*, QJSEngine*)->QObject* { return new WindowState(); });

    qmlRegisterType<DirSpaceUsed>(CPP_CUSTOM_MODULES, 1, 0, "DirSpaceUsed");
    qmlRegisterType<DesktopView>(CPP_CUSTOM_MODULES, 1, 0, "DesktopView");
    qmlRegisterType<SshSession>(CPP_CUSTOM_MODULES, 1, 0, "SshSession");
    qmlRegisterType<SystemProcess>(CPP_CUSTOM_MODULES, 1, 0, "RunProcess");
    qmlRegisterType<TextRender>(CPP_CUSTOM_MODULES, 1, 0, "TextRender");
    qmlRegisterType<UrlModel>(CPP_CUSTOM_MODULES, 1, 0, "UrlModel");

    QQmlApplicationEngine engine;
    QStringList efs;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    efs.append(QStringLiteral("mobile"));
#endif
    const QVersionNumber qt_ver = QLibraryInfo::version(); // Qt run-time version
    if (qt_ver.majorVersion() > 5)
        efs.append(QStringLiteral("qt") + QString::number(qt_ver.majorVersion()));
    if (!efs.isEmpty()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QQmlFileSelector::get(&engine)->setExtraSelectors(efs);
#else
        engine.setExtraFileSelectors(efs);
#endif
    }

    const QUrl qml(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [qml](QObject *obj, const QUrl &url) {
        if (!obj && qml == url) QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(qml);
    int rc = app.exec();

    ::ssh_finalize();
    return rc;
}
