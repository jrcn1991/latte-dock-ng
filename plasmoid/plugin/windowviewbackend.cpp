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

QStringList WindowViewBackend::validWindowIds(const QVariantList &windowIds)
{
    QStringList ids;
    for (const QVariant &value : windowIds) {
        const QUuid uuid(value.toString());
        if (!uuid.isNull() && !ids.contains(uuid.toString())) {
            ids.append(uuid.toString());
        }
    }
    return ids;
}

void WindowViewBackend::presentWindows(const QVariantList &windowIds)
{
    if (m_pending) {
        return;
    }

    const QStringList ids = validWindowIds(windowIds);

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

void WindowViewBackend::setHighlightedWindows(const QVariantList &windowIds, bool hovered)
{
    const QStringList ids = validWindowIds(windowIds);
    if (hovered) {
        if (ids.isEmpty() || ids == m_highlightedWindowIds) {
            return;
        }
        m_highlightedWindowIds = ids;
        sendHighlightedWindows(ids);
        return;
    }

    // An exit event from the previous delegate can arrive after the pointer
    // entered another task. Only that delegate may clear its own selection;
    // otherwise rapid task switching makes KWin's highlight flicker off.
    if (!ids.isEmpty() && ids != m_highlightedWindowIds) {
        return;
    }
    cancelHighlightWindows();
}

void WindowViewBackend::cancelHighlightWindows()
{
    if (m_highlightedWindowIds.isEmpty()) {
        return;
    }
    m_highlightedWindowIds.clear();
    sendHighlightedWindows({});
}

void WindowViewBackend::sendHighlightedWindows(const QStringList &windowIds)
{
    // Plasma 6 exposes the Wayland window UUID effect directly through KWin.
    // Empty input cancels the effect. Never add an X11 WId fallback here: the
    // task model's authoritative identities are Wayland UUIDs.
    auto message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
        QStringLiteral("/org/kde/KWin/HighlightWindow"),
        QStringLiteral("org.kde.KWin.HighlightWindow"), QStringLiteral("highlightWindows"));
    message.setArguments({windowIds});
    QDBusConnection::sessionBus().asyncCall(message);
}

}
