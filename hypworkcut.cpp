// HypWorkCut - Matthew Mueller

#include <QApplication>
#include <QTimer>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QKeyEvent>
#include <QFont>
#include <QSet>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QDateTime>
#include <QDialog>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QSettings>
#include <cmath>

// =================== SETTINGS ===================
// Centralized defaults and persistence so the GUI, storage, and runtime agree.
struct AppSettings {
    int cornerSize;      // trigger range: size (px) of the bottom-left hot corner
    int dismissMargin;   // distance (px) the cursor may stray from the popup before it hides
};

static constexpr int kDefaultCornerSize    = 50;   // pre-scale value (matches original 50/scale)
static constexpr int kDefaultDismissMargin = 800;  // matches original adjusted(-800,...,800,800)

static AppSettings loadSettings() {
    QSettings s("HypWorkCut", "HypWorkCut");
    AppSettings cfg;
    cfg.cornerSize    = s.value("cornerSize", kDefaultCornerSize).toInt();
    cfg.dismissMargin = s.value("dismissMargin", kDefaultDismissMargin).toInt();
    return cfg;
}

static void saveSettings(const AppSettings &cfg) {
    QSettings s("HypWorkCut", "HypWorkCut");
    s.setValue("cornerSize", cfg.cornerSize);
    s.setValue("dismissMargin", cfg.dismissMargin);
    s.sync();
}

// =================== SETTINGS DIALOG ===================
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const AppSettings &current, QWidget *parent = nullptr)
        : QDialog(parent) {
        setWindowTitle("HypWorkCut Settings");

        auto *form = new QFormLayout;

        cornerSpin = new QSpinBox(this);
        cornerSpin->setRange(1, 1000);
        cornerSpin->setSuffix(" px");
        cornerSpin->setValue(current.cornerSize);
        cornerSpin->setToolTip("How large the bottom-left hot corner is.\n"
                               "Larger values make the popup easier to trigger.");
        form->addRow("Trigger range (hot corner size):", cornerSpin);

        dismissSpin = new QSpinBox(this);
        dismissSpin->setRange(0, 5000);
        dismissSpin->setSuffix(" px");
        dismissSpin->setValue(current.dismissMargin);
        dismissSpin->setToolTip("How far the cursor may move away from the popup\n"
                                "before it is automatically dismissed.");
        form->addRow("Dismiss distance (popup margin):", dismissSpin);

        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults,
            this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
                this, [this]() {
                    cornerSpin->setValue(kDefaultCornerSize);
                    dismissSpin->setValue(kDefaultDismissMargin);
                });

        auto *root = new QVBoxLayout(this);
        auto *hint = new QLabel(
            "Trigger range controls how large the activation corner is.\n"
            "Dismiss distance controls how far you can move before the popup closes.", this);
        hint->setWordWrap(true);
        root->addWidget(hint);
        root->addLayout(form);
        root->addWidget(buttons);
        setLayout(root);
    }

    AppSettings values() const {
        AppSettings cfg;
        cfg.cornerSize    = cornerSpin->value();
        cfg.dismissMargin = dismissSpin->value();
        return cfg;
    }

private:
    QSpinBox *cornerSpin;
    QSpinBox *dismissSpin;
};

class WorkspaceSelector : public QWidget {
    Q_OBJECT
public:
    explicit WorkspaceSelector(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Hyprland Workspace Selector");
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setStyleSheet("background-color: rgba(15, 15, 35, 250); border-radius: 20px;");
        layout = new QGridLayout(this);
        layout->setSpacing(20);
        layout->setContentsMargins(30, 30, 30, 30);
        setLayout(layout);
    }

    void refresh(int currentWs, const QSet<int> &occupied) {
        while (QLayoutItem *item = layout->takeAt(0)) {
            delete item->widget(); delete item;
        }
        const int cols = 2;
        for (int i = 1; i <= 10; ++i) {
            QPushButton *btn = new QPushButton(QString::number(i));
            btn->setFixedSize(100, 100);
            btn->setCursor(Qt::PointingHandCursor);
            QFont font = btn->font(); font.setPointSize(30); font.setBold(true); btn->setFont(font);

            if (i == currentWs)
                btn->setStyleSheet("QPushButton { background-color: #ffaa00; color: black; border: 5px solid white; border-radius: 20px; } QPushButton:hover { background-color: #ffcc44; }");
            else if (occupied.contains(i))
                btn->setStyleSheet("QPushButton { background-color: #5555dd; color: white; border-radius: 20px; } QPushButton:hover { background-color: #7777ff; }");
            else
                btn->setStyleSheet("QPushButton { background-color: #333355; color: #8888bb; border-radius: 20px; } QPushButton:hover { background-color: #555588; }");

            connect(btn, &QPushButton::clicked, this, [this, i]() {
                QProcess::startDetached("hyprctl", {"dispatch", "workspace", QString::number(i)});
                hide();
            });

            int idx = i - 1;
            layout->addWidget(btn, 4 - (idx / cols), idx % cols);
        }
        adjustSize();
    }

protected:
    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape) hide();
        QWidget::keyPressEvent(e);
    }
private:
    QGridLayout *layout;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HypWorkCut");
    app.setOrganizationName("HypWorkCut");
    app.setQuitOnLastWindowClosed(false);  // Essential for tray-only apps

    WorkspaceSelector selector;
    selector.hide();

    // Load persisted settings (trigger range + dismiss distance).
    AppSettings cfg = loadSettings();

    // ====================== SYSTEM TRAY ICON ======================
    QSystemTrayIcon trayIcon(QIcon(QApplication::applicationDirPath() + "/HypWorkCut.png"), &app);
    trayIcon.setToolTip("HypWorkCut - Workspace Hot Corner");

    QMenu trayMenu;
    trayMenu.addAction("Show Workspace Selector", [&selector]() {
        // Force show in bottom-left corner of current monitor (for manual testing)
        QProcess p; p.start("hyprctl", QStringList() << "cursorpos" << "-j"); p.waitForFinished();
        auto doc = QJsonDocument::fromJson(p.readAllStandardOutput());
        if (doc.isObject()) {
            int cx = doc.object()["x"].toInt(), cy = doc.object()["y"].toInt();
            QProcess m; m.start("hyprctl", QStringList() << "monitors" << "-j"); m.waitForFinished();
            auto arr = QJsonDocument::fromJson(m.readAllStandardOutput()).array();
            for (auto v : arr) {
                auto mon = v.toObject();
                int mx = mon["x"].toInt(), my = mon["y"].toInt(), mw = mon["width"].toInt(), mh = mon["height"].toInt();
                if (cx >= mx && cx < mx+mw && cy >= my && cy < my+mh) {
                    selector.move(mx + 40, my + mh - selector.height() - 40);
                    break;
                }
            }
        }
        selector.show(); selector.raise();
    });

    trayMenu.addSeparator();

    // ---- Settings entry: opens the GUI to edit trigger range & dismiss distance ----
    trayMenu.addAction("Settings...", [&cfg]() {
        SettingsDialog dlg(cfg);
        if (dlg.exec() == QDialog::Accepted) {
            cfg = dlg.values();   // live update - picked up by the timer immediately
            saveSettings(cfg);    // persist across restarts
        }
    });

    trayMenu.addSeparator();
    trayMenu.addAction("Quit HypWorkCut", [&app]() { app.quit(); });

    trayIcon.setContextMenu(&trayMenu);
    trayIcon.show();

    // === GET SCALE AND MONITOR DIMENSIONS ONLY ONCE AT STARTUP ===
    double scale = 1.0;
    int mon_x = 0, mon_y = 0, mon_width = 0, mon_height = 0;
    QProcess monProc;
    monProc.start("hyprctl", {"monitors", "-j"});
    monProc.waitForFinished();
    QJsonArray monArr = QJsonDocument::fromJson(monProc.readAllStandardOutput()).array();
    if (!monArr.isEmpty()) {
        QJsonObject mo = monArr.first().toObject();  // safe for single monitor
        scale = mo["scale"].toDouble(1.0);
        mon_x = mo["x"].toInt();
        mon_y = mo["y"].toInt();
        mon_width = qRound(mo["width"].toInt() / scale);
        mon_height = qRound(mo["height"].toInt() / scale);
    }

    const int activationDelayMs = 120;
    const int checkIntervalMs = 66;

    qint64 cornerEntryTime = 0;
    bool trackingCorner = false;

    QTimer timer;
    timer.start(checkIntervalMs);

    QObject::connect(&timer, &QTimer::timeout, [&]() {
        // Read live, scale-adjusted values from settings every tick so changes
        // made in the GUI take effect without restarting the app.
        const int cornerSize    = qRound(cfg.cornerSize / scale);
        const int dismissMargin = cfg.dismissMargin;

        QProcess cursorProc;
        cursorProc.start("hyprctl", QStringList() << "cursorpos" << "-j");
        cursorProc.waitForFinished(100);
        QJsonDocument doc = QJsonDocument::fromJson(cursorProc.readAllStandardOutput());
        if (!doc.isObject()) return;
        int cx = doc.object()["x"].toInt(-99999);
        int cy = doc.object()["y"].toInt(-99999);
        if (cx < 0 || cy < 0) return;

        int localX = cx - mon_x;
        int localY = cy - mon_y;
        bool inZone = (localX <= cornerSize) && (localY >= mon_height - cornerSize);

        if (inZone) {
            if (!trackingCorner) {
                trackingCorner = true;
                cornerEntryTime = QDateTime::currentMSecsSinceEpoch();
            }
        } else trackingCorner = false;

        bool inPopup = selector.isVisible() &&
            selector.geometry()
                .adjusted(-dismissMargin, -dismissMargin, dismissMargin, dismissMargin)
                .contains(QPoint(cx, cy));

        if (selector.isVisible() && !inZone && !inPopup) selector.hide();

        if (!selector.isVisible() && trackingCorner &&
            (QDateTime::currentMSecsSinceEpoch() - cornerEntryTime) >= activationDelayMs) {

            // === FIXED ACTIVE WORKSPACE DETECTION (read output only ONCE) ===
            QProcess p;
            p.start("hyprctl", QStringList() << "activeworkspace" << "-j");
            p.waitForFinished(200);
            QByteArray activeOutput = p.readAllStandardOutput();  // <- read once
            QJsonObject activeObj = QJsonDocument::fromJson(activeOutput).object();
            int currentWs = activeObj.isEmpty() ? 1 : activeObj["id"].toInt(1);

            // === OCCUPIED WORKSPACES ===
            QSet<int> occupied;
            QProcess wp;
            wp.start("hyprctl", QStringList() << "workspaces" << "-j");
            wp.waitForFinished(200);
            QJsonArray wsArr = QJsonDocument::fromJson(wp.readAllStandardOutput()).array();
            for (const auto &v : wsArr) {
                QJsonObject o = v.toObject();
                int id = o["id"].toInt();
                if (id >= 1 && id <= 10 && o["windows"].toInt() > 0) {
                    occupied.insert(id);
                }
            }

            selector.refresh(currentWs, occupied);
            selector.show();
            selector.raise();
            selector.move(mon_x + 40, mon_y + mon_height - selector.height() - 40);
        }
    });

    return app.exec();
}

#include "hypworkcut.moc"
