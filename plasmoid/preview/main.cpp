/* SPDX-FileCopyrightText: 2026 Latte Dock Contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QScreen>
#include <QSocketNotifier>
#include <QTimer>
#include <QUuid>
#include <KIconTheme>
#include <LayerShellQt/window.h>
#include <Plasma/Plasma>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
namespace {
// Keep the wire format bounded on both sides of the process boundary.
constexpr int MaxProtocolBytes = 64 * 1024;
constexpr int MaxPreviews = 9;
// While a preview is on screen the dock re-sends the show request on every
// hover poll; if that stops for this long the helper is orphaned and exits.
constexpr int ActiveLeaseMs = 5000;
// After the preview is hidden the helper may leave after this bounded idle
// lease. The dock treats that exit as normal and starts a fresh helper on the
// next hover, so idle time does not accumulate processes.
constexpr int IdleLeaseMs = 30000;
// Gap between the dock's icon anchor and the preview card.
constexpr int AnchorGap = 8;
bool isBoundedNumber(const QJsonValue &value)
{
    if (!value.isDouble()) {
        return false;
    }
    const double number = value.toDouble();
    return std::isfinite(number) && std::abs(number) <= 100000.0;
}
}
// A bare QQuickWindow cannot take part in a Wayland popup: xdg_popup requires a
// transientParent in the same client, which a separate preview process cannot
// have. The surface is therefore a LayerShellQt overlay, positioned with
// anchors + margins. That is the only Wayland mechanism the compositor honours
// for an absolutely placed, cross-process tooltip surface.
class PreviewView : public QQuickView
{
    Q_OBJECT
public:
    bool hovered{false};
Q_SIGNALS:
    void hoveredChanged();
    void activateRequested(const QString &uuid);
public Q_SLOTS:
    // QML exposes activate() as an old-style signal, so it needs a real slot
    // to bridge into the protocol writer.
    void activate(const QString &uuid) { Q_EMIT activateRequested(uuid); }
protected:
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
            hovered = event->type() == QEvent::Enter;
            Q_EMIT hoveredChanged();
        }
        return QQuickView::event(event);
    }
};
int main(int argc, char **argv)
{
    KIconTheme::initTheme();
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setApplicationName(QStringLiteral("latte-dock-ng-preview"));
    PreviewView view;
    view.setFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint);
    view.setColor(Qt::transparent);
    view.setSource(QUrl(QStringLiteral("qrc:/preview/Preview.qml")));
    if (view.status() != QQuickView::Ready) {
        qWarning() << view.errors();
        return 1;
    }
    QQuickItem *item = view.rootObject();
    if (!item) {
        qWarning() << "Preview.qml has no root item";
        return 1;
    }
    // Configure the layer surface before the window is first shown so the QPA
    // plugin creates the correct role. Margins and size are refreshed later.
    auto *layer = LayerShellQt::Window::get(&view);
    layer->setScope(QStringLiteral("latte-dock-ng-preview"));
    layer->setLayer(LayerShellQt::Window::LayerOverlay);
    layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
    layer->setExclusiveZone(-1);
    layer->setActivateOnShow(false);
    LayerShellQt::Window::Anchors anchors;
    anchors |= LayerShellQt::Window::AnchorTop;
    anchors |= LayerShellQt::Window::AnchorLeft;
    layer->setAnchors(anchors);
    ::fcntl(STDIN_FILENO, F_SETFL, ::fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    ::fcntl(STDOUT_FILENO, F_SETFL, ::fcntl(STDOUT_FILENO, F_GETFL) | O_NONBLOCK);
    QByteArray input;
    QJsonArray currentWindows;
    QRect anchor;
    int edge = Plasma::Types::BottomEdge;
    int serial = 0;
    bool closing = false;
    // Cached layer-surface properties: during the zoom animation only the
    // margins change per frame, so avoid redundant size/screen round trips.
    QSize appliedSize;
    QScreen *appliedOutput = nullptr;
    QMargins appliedMargins(-1, -1, -1, -1);
    auto writeMessage = [&](const QJsonObject &message) {
        const QByteArray bytes = QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
        // stdout is non-blocking: a partial or failed write means the dock is
        // gone, so the helper exits instead of spinning.
        if (::write(STDOUT_FILENO, bytes.constData(), static_cast<size_t>(bytes.size())) != bytes.size()) {
            QCoreApplication::quit();
        }
    };
    auto report = [&]() {
        writeMessage(QJsonObject{{QStringLiteral("type"), QStringLiteral("heartbeat")},
            {QStringLiteral("serial"), serial}, {QStringLiteral("visible"), view.isVisible()},
            {QStringLiteral("hovered"), view.hovered}});
    };
    auto place = [&]() {
        if (!view.isVisible()) {
            return;
        }
        QScreen *output = QGuiApplication::screenAt(anchor.center());
        if (!output) {
            return;
        }
        // The QML root owns the card geometry; read it directly so the layer
        // surface is sized before it is mapped.
        const QSize size(qCeil(item->width()), qCeil(item->height()));
        if (size.isEmpty()) {
            return;
        }
        QPoint pos(anchor.center().x() - size.width() / 2, anchor.top() - size.height() - AnchorGap);
        if (edge == Plasma::Types::TopEdge) {
            pos.setY(anchor.bottom() + AnchorGap);
        } else if (edge == Plasma::Types::LeftEdge) {
            pos = QPoint(anchor.right() + AnchorGap, anchor.center().y() - size.height() / 2);
        } else if (edge == Plasma::Types::RightEdge) {
            pos = QPoint(anchor.left() - size.width() - AnchorGap, anchor.center().y() - size.height() / 2);
        }
        const QRect bounds = output->geometry();
        pos.setX(qBound(bounds.left(), pos.x(), qMax(bounds.left(), bounds.right() - size.width() + 1)));
        pos.setY(qBound(bounds.top(), pos.y(), qMax(bounds.top(), bounds.bottom() - size.height() + 1)));
        if (output != appliedOutput) {
            appliedOutput = output;
#ifdef LATTE_LAYERSHELL_HAS_SET_SCREEN
            layer->setScreen(output);
#else
            view.setScreen(output);
#endif
        }
        if (size != appliedSize) {
            appliedSize = size;
#ifdef LATTE_LAYERSHELL_HAS_DESIRED_SIZE
            layer->setDesiredSize(size);
#endif
            view.resize(size);
        }
        if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
            // Layer-shell positions relative to the chosen output's origin.
            const QMargins margins(pos.x() - bounds.x(), pos.y() - bounds.y(), 0, 0);
            if (margins != appliedMargins) {
                appliedMargins = margins;
                layer->setMargins(margins);
                // A margin-only layer-shell change does not damage the scene
                // graph, so Qt never commits the wl_surface and the compositor
                // keeps the previous position. Force a repaint so the new
                // margins are committed on the next frame.
                view.update();
            }
        } else {
            // X11 honours direct window placement.
            view.setPosition(pos);
        }
    };
    QObject::connect(item, SIGNAL(activate(QString)), &view, SLOT(activate(QString)));
    QObject::connect(&view, &PreviewView::activateRequested, &app, [&](const QString &uuid) {
        writeMessage(QJsonObject{{QStringLiteral("type"), QStringLiteral("activate")},
            {QStringLiteral("serial"), serial}, {QStringLiteral("uuid"), uuid}});
    });
    QObject::connect(&view, &PreviewView::hoveredChanged, &app, report);
    QSocketNotifier notifier(STDIN_FILENO, QSocketNotifier::Read);
    QTimer lease;
    lease.setSingleShot(true);
    auto closeAndQuit = [&]() {
        if (!closing) {
            closing = true;
            writeMessage(QJsonObject{{QStringLiteral("type"), QStringLiteral("closed")},
                {QStringLiteral("serial"), serial}});
        }
        app.quit();
    };
    QObject::connect(&lease, &QTimer::timeout, &app, closeAndQuit);
    auto hidePreview = [&]() {
        view.hide();
        view.hovered = false;
        item->setProperty("windows", QVariantList{});
        currentWindows = {};
        lease.setInterval(IdleLeaseMs);
        lease.start();
    };
    QObject::connect(&notifier, &QSocketNotifier::activated, &app, [&]() {
        char buffer[8192];
        const auto length = ::read(STDIN_FILENO, buffer, sizeof(buffer));
        if (length == 0) {
            // The dock closed the pipe: shut down cleanly.
            closeAndQuit();
            return;
        }
        if (length < 0) {
            return;
        }
        input.append(buffer, static_cast<qsizetype>(length));
        if (input.size() > MaxProtocolBytes) {
            closeAndQuit();
            return;
        }
        while (input.contains('\n')) {
            const auto end = input.indexOf('\n');
            const auto request = QJsonDocument::fromJson(input.left(end)).object();
            input.remove(0, end + 1);
            const QString type = request.value(QStringLiteral("type")).toString();
            // Malformed or unknown frames are ignored: the helper must survive
            // a bad request and let the dock's watchdog decide.
            if (!request.contains(QStringLiteral("serial"))) {
                continue;
            }
            if (type == QStringLiteral("hide")) {
                serial = request.value(QStringLiteral("serial")).toInt();
                hidePreview();
                report();
                continue;
            }
            const auto x = request.value(QStringLiteral("x"));
            const auto y = request.value(QStringLiteral("y"));
            const auto width = request.value(QStringLiteral("width"));
            const auto height = request.value(QStringLiteral("height"));
            const auto edgeValue = request.value(QStringLiteral("edge"));
            if (!isBoundedNumber(x) || !isBoundedNumber(y)
                    || !isBoundedNumber(width) || !isBoundedNumber(height)
                    || !edgeValue.isDouble()) {
                continue;
            }
            const int requestedEdge = edgeValue.toInt();
            if (requestedEdge < Plasma::Types::TopEdge || requestedEdge > Plasma::Types::RightEdge) {
                continue;
            }
            if (type == QStringLiteral("move")) {
                // A move only refines the anchor of the current selection; a
                // stale generation must not drag a newer preview around.
                if (request.value(QStringLiteral("serial")).toInt() != serial) {
                    continue;
                }
                anchor = QRect(x.toInt(), y.toInt(), width.toInt(), height.toInt());
                edge = requestedEdge;
                place();
                continue;
            }
            if (type != QStringLiteral("show")) {
                continue;
            }
            serial = request.value(QStringLiteral("serial")).toInt();
            const auto windows = request.value(QStringLiteral("windows")).toArray();
            if (windows.isEmpty() || windows.size() > MaxPreviews) {
                continue;
            }
            bool valid = true;
            for (const auto &window : windows) {
                const auto object = window.toObject();
                if (QUuid(object.value(QStringLiteral("uuid")).toString()).isNull()
                        || !object.value(QStringLiteral("title")).isString()) {
                    valid = false;
                    break;
                }
            }
            if (!valid) {
                continue;
            }
            anchor = QRect(x.toInt(), y.toInt(), width.toInt(), height.toInt());
            edge = requestedEdge;
            if (windows != currentWindows) {
                currentWindows = windows;
                item->setProperty("windows", windows.toVariantList());
            }
            place();
            item->setVisible(true);
            view.show();
            place();
            lease.setInterval(ActiveLeaseMs);
            lease.start();
            report();
        }
    });
    QTimer heartbeat;
    heartbeat.setInterval(250);
    QObject::connect(&heartbeat, &QTimer::timeout, &app, [&]() {
        place();
        report();
    });
    heartbeat.start();
    return app.exec();
}
#include "main.moc"
