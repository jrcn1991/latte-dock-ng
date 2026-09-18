/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "previewprocess.h"

#include <QSignalSpy>
#include <QTest>
#include <QUuid>

using Latte::Tasks::PreviewProcess;

class PreviewProcessUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void disabledGateStaysClosed();
    void closeReplyIsAcceptedForCurrentWindow();
    void forgedCloseReplyIsRejected();
    void activationReplyHidesPreview();
    void moveHeartbeatUpdatesHoverState();

private:
    static QVariantList windows(const QString &title);
};

QVariantList PreviewProcessUnitTest::windows(const QString &title)
{
    return {{QVariantMap{{QStringLiteral("uuid"),
                          QStringLiteral("11111111-1111-1111-1111-111111111111")},
                         {QStringLiteral("appName"), QStringLiteral("Autotest")},
                         {QStringLiteral("title"), title},
                         {QStringLiteral("launcherUrl"),
                          QStringLiteral("applications:org.kde.konsole.desktop")},
                         {QStringLiteral("appPid"), 123},
                         {QStringLiteral("minimized"), true}}}};
}

void PreviewProcessUnitTest::disabledGateStaysClosed()
{
    PreviewProcess process(QStringLiteral(LATTE_PREVIEW_FAKE_HELPER), false);
    process.show(windows(QStringLiteral("idle")), QRect(10, 20, 40, 40), 4);
    QTest::qWait(100);
    QVERIFY(!process.enabled());
    QVERIFY(!process.visible());
    QVERIFY(!process.hovered());
}

void PreviewProcessUnitTest::closeReplyIsAcceptedForCurrentWindow()
{
    PreviewProcess process(QStringLiteral(LATTE_PREVIEW_FAKE_HELPER), true);
    QSignalSpy closeSpy(&process, &PreviewProcess::closeRequested);

    process.show(windows(QStringLiteral("close")), QRect(10, 20, 40, 40), 4);
    QTRY_VERIFY(process.visible());
    QTRY_COMPARE(closeSpy.count(), 1);
    QCOMPARE(closeSpy.first().first().toString(),
             QStringLiteral("11111111-1111-1111-1111-111111111111"));
    // Closing remains dock-owned, so the helper stays mapped until the task
    // model removes the window or the pointer leaves.
    QVERIFY(process.visible());
}

void PreviewProcessUnitTest::forgedCloseReplyIsRejected()
{
    PreviewProcess process(QStringLiteral(LATTE_PREVIEW_FAKE_HELPER), true);
    QSignalSpy closeSpy(&process, &PreviewProcess::closeRequested);

    process.show(windows(QStringLiteral("stale-close")), QRect(10, 20, 40, 40), 4);
    QTRY_VERIFY(process.visible());
    QTest::qWait(100);
    QCOMPARE(closeSpy.count(), 0);
}

void PreviewProcessUnitTest::activationReplyHidesPreview()
{
    PreviewProcess process(QStringLiteral(LATTE_PREVIEW_FAKE_HELPER), true);
    QSignalSpy activateSpy(&process, &PreviewProcess::activateRequested);

    process.show(windows(QStringLiteral("activate")), QRect(10, 20, 40, 40), 4);
    QTRY_COMPARE(activateSpy.count(), 1);
    QCOMPARE(activateSpy.first().first().toString(),
             QStringLiteral("11111111-1111-1111-1111-111111111111"));
    QTRY_VERIFY(!process.visible());
}

void PreviewProcessUnitTest::moveHeartbeatUpdatesHoverState()
{
    PreviewProcess process(QStringLiteral(LATTE_PREVIEW_FAKE_HELPER), true);
    process.show(windows(QStringLiteral("idle")), QRect(10, 20, 40, 40), 4);
    QTRY_VERIFY(process.visible());
    QVERIFY(!process.hovered());

    process.move(QRect(50, 60, 40, 40), 4);
    QTRY_VERIFY(process.hovered());
}

QTEST_GUILESS_MAIN(PreviewProcessUnitTest)

#include "previewprocessunittest.moc"
