// SPDX-License-Identifier: GPL-2.0-or-later
// KWin is the authority for panel identity, output and stacking. A constraint
// survives clicking/remapping without changing either panel's visibility layer.
(function () {
    var busy = false;
    var constraints = [];
    var lastPayload = "";
    var canConstrain = typeof workspace.constrain === "function"
                       && typeof workspace.unconstrain === "function";

    function isPlasmaPanel(w) {
        return w.dock && (String(w.resourceClass) === "plasmashell"
                         || String(w.resourceClass) === "org.kde.plasmashell");
    }

    function isLatteView(w) {
        return w.dock && String(w.caption).indexOf("#view#") === 0
                && (String(w.resourceClass) === "org.kde.latte-dock"
                    || String(w.resourceClass) === "latte-dock-ng"
                    || String(w.resourceClass) === "org.kde.latte-dock-ng");
    }

    function overlaps(a, b) {
        return a.x < b.x + b.width && b.x < a.x + a.width
                && a.y < b.y + b.height && b.y < a.y + a.height;
    }

    function update() {
        if (busy) return;
        busy = true;
        try {
            var windows = workspace.stackingOrder;
            var panels = windows.filter(isPlasmaPanel);
            var views = windows.filter(isLatteView);
            var next = [];
            var payload = [];
            for (var panel of panels) {
                var p = panel.frameGeometry;
                if (panel.minimized || !panel.output || p.width <= 0 || p.height <= 0) continue;
                payload.push({output: panel.output.name, x: p.x, y: p.y, width: p.width, height: p.height});
                for (var view of views) {
                    var v = view.frameGeometry;
                    if (!view.output || view.output.name !== panel.output.name
                            || (v.width > v.height) === (p.width > p.height) || !overlaps(v, p)) continue;
                    var existing = constraints.some(pair => pair[0] === view && pair[1] === panel);
                    next.push([view, panel]);
                    if (canConstrain) {
                        if (!existing) workspace.constrain(view, panel);
                    } else if (windows.indexOf(view) > windows.indexOf(panel)) {
                        // Plasma < 6.5 has no pair constraint API. Repair only
                        // inverted pairs on stacking changes, without focus changes.
                        workspace.raiseWindow(panel);
                    }
                }
            }
            if (canConstrain) {
                for (var pair of constraints) {
                    if (windows.indexOf(pair[0]) >= 0 && windows.indexOf(pair[1]) >= 0
                            && !next.some(candidate => candidate[0] === pair[0] && candidate[1] === pair[1])) {
                        workspace.unconstrain(pair[0], pair[1]);
                    }
                }
            }
            constraints = next;
            // Stable order avoids geometry notifications on pure restacking.
            payload.sort((a, b) => a.output.localeCompare(b.output) || a.x - b.x || a.y - b.y);
            var serialized = JSON.stringify(payload);
            if (serialized !== lastPayload) {
                lastPayload = serialized;
                callDBus("@LATTE_BUS@", "/Latte/PanelTracker", "org.kde.Latte.PanelTracker", "updatePanels", serialized);
            }
        } finally {
            busy = false;
        }
    }

    function watch(w) {
        // Identity can arrive after windowAdded. Observe all windows, but only
        // the explicitly identified dock pairs can change geometry or stacking.
        w.frameGeometryChanged.connect(update);
        w.windowClassChanged.connect(update);
        w.captionChanged.connect(update);
        w.outputChanged.connect(update);
        w.minimizedChanged.connect(update);
        w.stackingOrderChanged.connect(update);
        update();
    }
    workspace.windowAdded.connect(watch);
    workspace.windowRemoved.connect(update);
    workspace.screensChanged.connect(update);
    workspace.stackingOrder.forEach(watch);
    update();
})();
