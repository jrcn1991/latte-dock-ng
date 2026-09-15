/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "../plasmoid/plugin/windowviewbackend.h"

#include <QDBusConnection>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

static const QString service = QStringLiteral("org.kde.KWin.Effect.WindowView1");
static const QString path = QStringLiteral("/org/kde/KWin/Effect/WindowView1");
static const QString firstId = QStringLiteral("{12345678-1234-1234-1234-123456789abc}");
static const QString secondId = QStringLiteral("{87654321-4321-4321-4321-cba987654321}");

class FakeWindowView : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KWin.Effect.WindowView1")
public:
    QList<QStringList> calls;
public Q_SLOTS:
    void activate(const QStringList &ids) { calls.append(ids); }
};

class WindowViewBackendTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        // CTest creates a private session bus. Never claim an effect service on
        // the user's desktop, even when someone runs this executable directly.
        QVERIFY(qEnvironmentVariableIsSet("LATTE_PRIVATE_TEST_BUS"));
        QVERIFY(QDBusConnection::sessionBus().isConnected());
    }

    void init()
    {
        m_effect.calls.clear();
        QVERIFY(QDBusConnection::sessionBus().registerService(service));
        QVERIFY(QDBusConnection::sessionBus().registerObject(path, &m_effect, QDBusConnection::ExportAllSlots));
    }

    void cleanup()
    {
        QDBusConnection::sessionBus().unregisterObject(path);
        QDBusConnection::sessionBus().unregisterService(service);
    }

    void rejectsEmptySelection()
    {
        Latte::Tasks::WindowViewBackend backend;
        QSignalSpy result(&backend, &Latte::Tasks::WindowViewBackend::finished);
        backend.presentWindows({});
        backend.presentWindows({QString(), QStringLiteral("not-a-uuid"), 123, QUuid()});
        QCOMPARE(result.count(), 2);
        for (const auto &arguments : result) {
            QCOMPARE(arguments.at(0).toBool(), false);
        }
        QVERIFY(m_effect.calls.isEmpty());
    }

    void forwardsWindowGroup()
    {
        Latte::Tasks::WindowViewBackend backend;
        QSignalSpy result(&backend, &Latte::Tasks::WindowViewBackend::finished);
        backend.presentWindows({firstId, QUuid(firstId), QStringLiteral("invalid"), QUuid(secondId)});
        QTRY_COMPARE(result.count(), 1);
        QCOMPARE(result.first().at(0).toBool(), true);
        QCOMPARE(m_effect.calls, QList<QStringList>({{firstId, secondId}}));
    }

    void missingInterfaceFallsBackAndRecovers()
    {
        Latte::Tasks::WindowViewBackend backend;
        QSignalSpy result(&backend, &Latte::Tasks::WindowViewBackend::finished);
        QDBusConnection::sessionBus().unregisterObject(path);
        backend.presentWindows({firstId});
        QTRY_COMPARE(result.count(), 1);
        QCOMPARE(result.first().at(0).toBool(), false);
        QVERIFY(QDBusConnection::sessionBus().registerObject(path, &m_effect, QDBusConnection::ExportAllSlots));
        backend.presentWindows({secondId});
        QTRY_COMPARE(result.count(), 2);
        QCOMPARE(result.last().at(0).toBool(), true);
        QCOMPARE(m_effect.calls, QList<QStringList>({{secondId}}));
    }

    void absentEffectFallsBack()
    {
        Latte::Tasks::WindowViewBackend backend;
        QSignalSpy result(&backend, &Latte::Tasks::WindowViewBackend::finished);
        QVERIFY(QDBusConnection::sessionBus().unregisterService(service));
        backend.presentWindows({firstId});
        QTRY_COMPARE(result.count(), 1);
        QCOMPARE(result.first().at(0).toBool(), false);
        QVERIFY(m_effect.calls.isEmpty());
    }

    void coalescesPendingClicks()
    {
        Latte::Tasks::WindowViewBackend backend;
        QSignalSpy result(&backend, &Latte::Tasks::WindowViewBackend::finished);
        backend.presentWindows({firstId});
        backend.presentWindows({secondId});
        QTRY_COMPARE(result.count(), 1);
        QCOMPARE(m_effect.calls, QList<QStringList>({{firstId}}));
        backend.presentWindows({secondId});
        QTRY_COMPARE(result.count(), 2);
        QCOMPARE(m_effect.calls.size(), 2);
    }

private:
    FakeWindowView m_effect;
};

QTEST_GUILESS_MAIN(WindowViewBackendTest)
#include "windowviewbackendtest.moc"
