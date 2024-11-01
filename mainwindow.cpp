#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serial(new QSerialPort(this))
    , isSerialPortOpen(false)
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

void MainWindow::sendSerialData()
{
    if (serial->isOpen()) {
        QString data = ui->textEditToSend->toPlainText();
        serial->write(data.toUtf8());
        ui->textEditHistory->append("发送: " + data);
    } else {
        QMessageBox::warning(this, tr("错误"), tr("串口未打开"));
    }
}

void MainWindow::readSerialData()
{
    if (serial->isOpen()) {
        QByteArray data = serial->readAll();
        ui->textEditHistory->append("接收: " + QString::fromUtf8(data));
    }
}

void MainWindow::clearHistory()
{
    ui->textEditHistory->clear();
}
