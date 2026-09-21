/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QDBusContext>
#include <QList>
#include <QJsonArray>
#include <QObject>
#include <QRect>
#include <QTemporaryFile>
#include <QTimer>

namespace Latte::WindowSystem {

// KWin owns panel identity, output assignment, geometry and stacking. Keep
// this transient bridge separate from application-window task tracking.
class PlasmaPanelTracker
  : public QObject
  , protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Latte.PanelTracker")
  public:
    explicit PlasmaPanelTracker(QObject *parent = nullptr);
    ~PlasmaPanelTracker() override;
    QList<QRect> geometries() const;

  public Q_SLOTS:
    void updatePanels(const QString &payload);

  Q_SIGNALS:
    void changed();

  private:
    void start();
    void clear();
    void refreshGeometries();
    QJsonArray m_panels;
    QTemporaryFile m_script;
    QTimer m_timeout;
    QList<QRect> m_geometries;
    QString m_owner;
    QString m_pluginName;
    int m_generation{ 0 };
};
}
