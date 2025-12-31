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
#include <cmath>

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
    app.setQuitOnLastWindowClosed(false);  // Essential for tray-only apps

    WorkspaceSelector selector;
    selector.hide();

    // ====================== SYSTEM TRAY ICON ======================
    QSystemTrayIcon trayIcon(QIcon(QApplication::applicationDirPath() + "/HypWorkCut.png"), &app);
    trayIcon.setToolTip("HypWorkCut – Workspace Hot Corner");

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

    const int cornerSize = qRound(50 / scale);
    const int activationDelayMs = 120;
    const int checkIntervalMs = 66;

    qint64 cornerEntryTime = 0;
    bool trackingCorner = false;

    QTimer timer;
    timer.start(checkIntervalMs);

    QObject::connect(&timer, &QTimer::timeout, [&]() {
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

        bool inPopup = selector.isVisible() && selector.geometry().adjusted(-800,-800,800,800).contains(QPoint(cx,cy));

        if (selector.isVisible() && !inZone && !inPopup) selector.hide();

        if (!selector.isVisible() && trackingCorner &&
            (QDateTime::currentMSecsSinceEpoch() - cornerEntryTime) >= activationDelayMs) {

            // === FIXED ACTIVE WORKSPACE DETECTION (read output only ONCE) ===
            QProcess p;
            p.start("hyprctl", QStringList() << "activeworkspace" << "-j");
            p.waitForFinished(200);
            QByteArray activeOutput = p.readAllStandardOutput();  // ← read once
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
