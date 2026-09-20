/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef LATTE_WINDOWVIEWBACKEND_H
#define LATTE_WINDOWVIEWBACKEND_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace Latte::Tasks {

class WindowViewBackend : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(WindowViewBackend)
public:
    explicit WindowViewBackend(QObject *parent = nullptr);
    Q_INVOKABLE void presentWindows(const QVariantList &windowIds);
    Q_INVOKABLE void setHighlightedWindows(const QVariantList &windowIds, bool hovered);
    Q_INVOKABLE void cancelHighlightWindows();

Q_SIGNALS:
    void finished(bool presented);

private:
    static QStringList validWindowIds(const QVariantList &windowIds);
    void sendHighlightedWindows(const QStringList &windowIds);

    bool m_pending{false};
    QStringList m_highlightedWindowIds;
};

}
#endif
