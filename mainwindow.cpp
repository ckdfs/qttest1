#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serial(new QSerialPort(this))
    , isSerialPortOpen(false)
    , receiveTimer(new QTimer(this))
{
    ui->setupUi(this);

    // Populate serial port combo box
    refreshSerialPorts();

    // Select the last serial port
    if (ui->comboBoxPort->count() > 0) {
        ui->comboBoxPort->setCurrentIndex(ui->comboBoxPort->count() - 1);
    }

    // Connect signals and slots
    connect(ui->buttonTogglePort, &QPushButton::clicked, this, &MainWindow::toggleSerialPort);
    connect(ui->buttonRefreshPorts, &QPushButton::clicked, this, &MainWindow::refreshSerialPorts);
    connect(ui->buttonSend, &QPushButton::clicked, this, &MainWindow::sendSerialData);
    connect(ui->buttonClearHistory, &QPushButton::clicked, this, &MainWindow::clearHistory);
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readSerialData);

    // 初始化接收定时器
    receiveTimer->setSingleShot(true);
    connect(receiveTimer, &QTimer::timeout, this, &MainWindow::processReceivedData);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::toggleSerialPort()
{
    if (isSerialPortOpen) {
        serial->close();
        ui->buttonTogglePort->setText("打开串口");
        ui->buttonTogglePort->setStyleSheet("background-color: red");
        QMessageBox::information(this, tr("成功"), tr("串口已关闭"));
    } else {
        serial->setPortName(ui->comboBoxPort->currentText());
        serial->setBaudRate(ui->comboBoxBaudRate->currentText().toInt());
        serial->setDataBits(static_cast<QSerialPort::DataBits>(ui->comboBoxDataBits->currentText().toInt()));
        serial->setParity(static_cast<QSerialPort::Parity>(ui->comboBoxParity->currentIndex()));
        serial->setStopBits(static_cast<QSerialPort::StopBits>(ui->comboBoxStopBits->currentIndex()));
        serial->setFlowControl(QSerialPort::NoFlowControl);

        if (serial->open(QIODevice::ReadWrite)) {
            ui->buttonTogglePort->setText("关闭串口");
            ui->buttonTogglePort->setStyleSheet("background-color: green");
            QMessageBox::information(this, tr("成功"), tr("串口已打开"));
        } else {
            QMessageBox::critical(this, tr("错误"), serial->errorString());
        }
    }
    isSerialPortOpen = !isSerialPortOpen;
}

void MainWindow::refreshSerialPorts()
{
    ui->comboBoxPort->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ui->comboBoxPort->addItem(info.portName());
    }

    // Select the last serial port
    if (ui->comboBoxPort->count() > 0) {
        ui->comboBoxPort->setCurrentIndex(ui->comboBoxPort->count() - 1);
    }
}

// 发送串口数据
void MainWindow::sendSerialData()
{
    if (serial->isOpen()) {
        QString data = ui->textEditToSend->toPlainText();
        serial->write(data.toUtf8());
        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
        ui->textEditHistory->append("[" + timestamp + "] 发送: " + data);
    } else {
        QMessageBox::warning(this, tr("错误"), tr("串口未打开"));
    }
}

// 读取串口数据
void MainWindow::readSerialData()
{
    if (serial->isOpen()) {
        receiveBuffer.append(serial->readAll());
        // 重启定时器
        receiveTimer->start(RECEIVE_TIMEOUT);
    }
}

void MainWindow::processReceivedData()
{
    if (receiveBuffer.isEmpty()) {
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    ui->textEditHistory->append("[" + timestamp + "] 接收: " + QString::fromUtf8(receiveBuffer));
    receiveBuffer.clear();
}

void MainWindow::clearHistory()
{
    ui->textEditHistory->clear();
}
