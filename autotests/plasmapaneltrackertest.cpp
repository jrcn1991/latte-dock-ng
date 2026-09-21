// SPDX-License-Identifier: GPL-2.0-or-later
#include <QFile>
#include <QJSEngine>
#include <QTest>

class PlasmaPanelTrackerTest : public QObject
{
    Q_OBJECT
  private Q_SLOTS:
    void stackingAndOutputChanges();
    void legacyStackingRepair();
};

static void
prepare(QJSEngine &engine, bool constraints)
{
    engine.evaluate(QStringLiteral(R"JS(
        function signal() { return { callbacks: [], connect: function(f) { this.callbacks.push(f); },
            emit: function() { this.callbacks.forEach(f => f()); } }; }
        function windowFor(name, caption, output, rect) {
            var w = { dock: true, resourceClass: name, caption: caption,
                output: {name: output}, frameGeometry: rect, minimized: false };
            for (var key of ['frameGeometryChanged', 'windowClassChanged', 'captionChanged',
                            'outputChanged', 'minimizedChanged', 'stackingOrderChanged']) w[key] = signal();
            return w;
        }
        var panel = windowFor('plasmashell', '', 'DP-2', {x:0,y:0,width:1920,height:56});
        var view = windowFor('org.kde.latte-dock', '#view#1', 'DP-2', {x:0,y:0,width:384,height:1200});
        var other = windowFor('org.kde.latte-dock', '#view#12', 'HDMI-A-1', {x:1920,y:120,width:384,height:1080});
        var added = [], removed = [], raised = [], payloads = [];
        function callDBus(bus, path, iface, method, payload) { payloads.push(JSON.parse(payload)); }
        var workspace = { stackingOrder: [panel, view, other], windowAdded: signal(),
            windowRemoved: signal(), screensChanged: signal(), raiseWindow: function(w) { raised.push(w); } };
    )JS"));
    if (constraints) {
        engine.evaluate(QStringLiteral("workspace.constrain = (a,b) => added.push([a,b]); workspace.unconstrain = (a,b) => removed.push([a,b]);"));
    }
}

static QJSValue
runTracker(QJSEngine &engine)
{
    QFile file(QStringLiteral(PANEL_TRACKER_SCRIPT));
    if (!file.open(QIODevice::ReadOnly))
        return engine.newErrorObject(QJSValue::GenericError, file.errorString());
    return engine.evaluate(QString::fromUtf8(file.readAll()), file.fileName());
}

void
PlasmaPanelTrackerTest::stackingAndOutputChanges()
{
    QJSEngine engine;
    prepare(engine, true);
    const auto result = runTracker(engine);
    QVERIFY2(!result.isError(), qPrintable(result.toString()));
    QVERIFY(engine.evaluate(QStringLiteral("added.length === 1 && added[0][0] === view && added[0][1] === panel")).toBool());
    QCOMPARE(engine.evaluate(QStringLiteral("payloads[0][0].height")).toInt(), 56);
    engine.evaluate(QStringLiteral("view.stackingOrderChanged.emit()"));
    QCOMPARE(engine.evaluate(QStringLiteral("added.length")).toInt(), 1);
    QCOMPARE(engine.evaluate(QStringLiteral("payloads.length")).toInt(), 1);
    // Moving the panel between staggered outputs must release the old pair.
    engine.evaluate(QStringLiteral("panel.output.name = 'HDMI-A-1'; panel.frameGeometry = {x:1920,y:120,width:1920,height:40}; panel.outputChanged.emit()"));
    QVERIFY(engine.evaluate(QStringLiteral("added.length === 2 && added[1][0] === other && removed.length === 1 && removed[0][0] === view")).toBool());
    // A short centered panel does not intersect the vertical dock.
    engine.evaluate(QStringLiteral("panel.frameGeometry.x = 2400; panel.frameGeometry.width = 800; panel.frameGeometryChanged.emit()"));
    QCOMPARE(engine.evaluate(QStringLiteral("removed.length")).toInt(), 2);
    engine.evaluate(QStringLiteral("workspace.stackingOrder = [view,other]; workspace.windowRemoved.emit()"));
    QCOMPARE(engine.evaluate(QStringLiteral("payloads[payloads.length-1].length")).toInt(), 0);
}

void
PlasmaPanelTrackerTest::legacyStackingRepair()
{
    QJSEngine engine;
    prepare(engine, false);
    const auto result = runTracker(engine);
    QVERIFY2(!result.isError(), qPrintable(result.toString()));
    QVERIFY(engine.evaluate(QStringLiteral("raised.length > 0 && raised.every(w => w === panel)")).toBool());
    engine.evaluate(QStringLiteral("raised = []; workspace.stackingOrder = [view,other,panel]; view.stackingOrderChanged.emit()"));
    QCOMPARE(engine.evaluate(QStringLiteral("raised.length")).toInt(), 0);
}

QTEST_GUILESS_MAIN(PlasmaPanelTrackerTest)
#include "plasmapaneltrackertest.moc"
