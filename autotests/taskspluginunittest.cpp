/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

// local
#include "contextmenuactionsbackend.h"

// Qt
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtQml/qqml.h>
#include <QTest>

#include <memory>

class TasksPluginUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void registersQmlTypes();
    void contextMenuBackendRejectsMissingParentAndInvalidLaunchers();
};

//! The plugin .so and its generated qmldir are staged into a temporary
//! import path and loaded by a real QML engine, asserting the same
//! contract as the former hand-written registerTypes(): the
//! ContextMenuActionsBackend type and the flat "types.<Value>" enum
//! access used across the tasks QML sources keep working.
void TasksPluginUnitTest::registersQmlTypes()
{
    QTemporaryDir importRoot;
    QVERIFY(importRoot.isValid());

    const QString modulePath = importRoot.path() + QStringLiteral("/org/kde/latte/private/tasks");
    QVERIFY(QDir().mkpath(modulePath));

    const QString pluginFile = QStringLiteral(LATTE_TASKS_PLUGIN);
    QVERIFY(QFile::copy(QStringLiteral(LATTE_TASKS_QMLDIR), modulePath + QStringLiteral("/qmldir")));
    // $<TARGET_FILE> already includes the platform's shared-library prefix.
    QVERIFY(QFile::copy(pluginFile, modulePath + QLatin1Char('/')
                                  + QFileInfo(pluginFile).fileName()));

    QQmlEngine engine;
    engine.addImportPath(importRoot.path());

    QQmlComponent component(&engine);
    component.setData(QByteArrayLiteral(
                          "import QtQml\n"
                          "import org.kde.latte.private.tasks\n"
                          "QtObject {\n"
                          "    readonly property int click: types.LeftClick\n"
                          "    property ContextMenuActionsBackend backend\n"
                          "    property WindowViewBackend windowView: WindowViewBackend {}\n"
                          "}\n"),
                      QUrl(QStringLiteral("qrc:/taskspluginregistrationtest.qml")));
    if (component.isError()) {
        qWarning() << component.errors();
        qWarning() << "staged module files:" << QDir(modulePath).entryList(QDir::Files);
    }
    QCOMPARE(component.status(), QQmlComponent::Ready);
    const std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);
    QVERIFY(object->property("windowView").value<QObject *>());

    QVERIFY(qmlTypeId("org.kde.latte.private.tasks", 0, 1, "ContextMenuActionsBackend") >= 0);
    // The probe above is the authoritative check for the Types gadget:
    // declaratively registered gadgets may not be visible through
    // qmlTypeId's revision lookup, but must resolve in real QML.
}

void TasksPluginUnitTest::contextMenuBackendRejectsMissingParentAndInvalidLaunchers()
{
    Latte::Tasks::ContextMenuActionsBackend backend;
    QObject parent;

    QVERIFY(backend.jumpListActions(QVariant(), nullptr).isEmpty());
    QVERIFY(backend.jumpListActions(QUrl(QStringLiteral("file:///tmp/missing.desktop")), &parent).isEmpty());
    QVERIFY(backend.placesActions(QVariant(), false, nullptr).isEmpty());
    QVERIFY(backend.placesActions(QUrl(QStringLiteral("file:///tmp/missing.desktop")), false, &parent).isEmpty());
    QVERIFY(backend.recentDocumentActions(QVariant(), nullptr).isEmpty());
    QVERIFY(backend.recentDocumentActions(QUrl(QStringLiteral("file:///tmp/missing.desktop")), &parent).isEmpty());

    QSignalSpy showAllSpy(&backend, &Latte::Tasks::ContextMenuActionsBackend::showAllPlaces);
    QCOMPARE(showAllSpy.count(), 0);
}

QTEST_GUILESS_MAIN(TasksPluginUnitTest)

#include "taskspluginunittest.moc"
