#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVariant>

class ctkEventAdminBus;

namespace Ui {
    class MainWindow;
}

class ctkEventDemo : public QObject {
    Q_OBJECT

Q_SIGNALS:
    void receiveEventSignal(QVariantList l);
    void updateMessageSignal(QString message);

public Q_SLOTS:
    void receiveEvent(QVariantList l);
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    MainWindow(ctkEventAdminBus *bus, QWidget *parent = 0);
    ~MainWindow();

public Q_SLOTS:
    void sendEvent();
    void updateMessage(QString message);
    void connectClient();

protected:
    void changeEvent(QEvent *e);
    void connectEvents();

private:
    Ui::MainWindow *ui;
    ctkEventAdminBus *m_EventBus;

    ctkEventDemo *handler;
};

#endif // MAINWINDOW_H
