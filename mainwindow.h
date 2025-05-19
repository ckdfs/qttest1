#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QLineEdit>

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
    void sendChannelControlData(int channel, double voltage);
    // void sendInitialValues();
    void validateVoltageInput(QLineEdit *lineEdit, const QString &text);
    void onVoltageInputChanged(const QString &text);
    void onVoltageButtonClicked();
    void sendPowerDiffCommand();
    void updatePowerDiffUi();
    void updateCurrentVoltage();
    // 新增CW/CCW相关槽函数声明
    void updateCurrentVoltageCW();
    void updateCurrentVoltageCCW();
    void onVoltageInputChangedCW(const QString &text);
    void onVoltageInputChangedCCW(const QString &text);
    void sendControlDataCW();
    void sendControlDataCCW();
    void onVoltageButtonClickedCCW();

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    bool isSerialPortOpen;
    bool isAutoMode;
    bool powerDiffEnabled = false;
    QByteArray receiveBuffer;
    QTimer *receiveTimer;
    static const int RECEIVE_TIMEOUT = 50;
    QMap<int, double> channelVoltages;
    QMap<int, double> channelVoltagesCW;
    QMap<int, double> channelVoltagesCCW;
    QByteArray packProtocolData(bool isAuto, double voltage, int channel);
};

#endif // MAINWINDOW_H
