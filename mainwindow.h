#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void toggleSerialPort();
    void refreshSerialPorts();
    void sendSerialData();
    void readSerialData();
    void clearHistory();
    void processReceivedData();

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    bool isSerialPortOpen;
    QByteArray receiveBuffer;
    QTimer *receiveTimer;
    static const int RECEIVE_TIMEOUT = 50;
};

#endif // MAINWINDOW_H
