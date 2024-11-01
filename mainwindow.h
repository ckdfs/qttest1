#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>

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

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    bool isSerialPortOpen;
};

#endif // MAINWINDOW_H
