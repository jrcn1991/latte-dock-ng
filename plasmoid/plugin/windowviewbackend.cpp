/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "windowviewbackend.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QUuid>

namespace Latte::Tasks {

WindowViewBackend::WindowViewBackend(QObject *parent)
    : QObject(parent)
{
}

void WindowViewBackend::presentWindows(const QVariantList &windowIds)
{
    if (m_pending) {
        return;
    }

    QStringList ids;
    for (const QVariant &value : windowIds) {
        const QUuid uuid(value.toString());
        if (!uuid.isNull() && !ids.contains(uuid.toString())) {
            ids.append(uuid.toString());
        }
    }

    // KWin owns WindowView rendering; this path never creates a Latte preview
    // window or a PipeWire stream. Wayland task IDs are UUIDs, not numeric XIDs.
    // Do not report success for an empty selection: KWin accepts that call but
    // displays nothing, leaving the click without either presentation or fallback.
    if (ids.isEmpty()) {
        Q_EMIT finished(false);
        return;
    }

    auto message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin.Effect.WindowView1"),
        QStringLiteral("/org/kde/KWin/Effect/WindowView1"),
        QStringLiteral("org.kde.KWin.Effect.WindowView1"), QStringLiteral("activate"));
    message.setArguments({ids});
    m_pending = true;
    // Query the actual operation on every click: a cached capability flag can
    // become stale when effects are toggled or KWin restarts. Keep the dock's
    // UI thread responsive and only fall back after an explicit bus error.
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message, 1500), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        const QDBusPendingReply<> reply = *watcher;
        m_pending = false;
        watcher->deleteLater();
        Q_EMIT finished(!reply.isError());
    });
}

}
