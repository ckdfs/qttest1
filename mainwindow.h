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
    void readSerialData();
    void clearHistory();
    void processReceivedData();
    void toggleControlMode();
    void sendControlData();
    void validateVoltageInput(const QString &text);

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    bool isSerialPortOpen;
    bool isAutoMode;
    QByteArray receiveBuffer;
    QTimer *receiveTimer;
    static const int RECEIVE_TIMEOUT = 50;
    QByteArray packProtocolData(bool isAuto, double voltage, int channel);
};

#endif // MAINWINDOW_H
