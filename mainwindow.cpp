/*
 * @Author: ckdfs 2459317008@qq.com
 * @Date: 2024-11-01 09:32:11
 * @LastEditors: ckdfs 2459317008@qq.com
 * @LastEditTime: 2024-11-01 09:42:06
 * @FilePath: \qttest1\mainwindow.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serial(new QSerialPort(this))
{
    ui->setupUi(this);

    // Populate serial port combo box
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ui->comboBoxPort->addItem(info.portName());
    }

    // Connect signals and slots
    connect(ui->buttonOpenPort, &QPushButton::clicked, this, &MainWindow::openSerialPort);
    connect(ui->buttonClosePort, &QPushButton::clicked, this, &MainWindow::closeSerialPort);
    connect(ui->buttonSend, &QPushButton::clicked, this, &MainWindow::sendSerialData);
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openSerialPort()
{
    serial->setPortName(ui->comboBoxPort->currentText());
    serial->setBaudRate(ui->comboBoxBaudRate->currentText().toInt());
    serial->setDataBits(static_cast<QSerialPort::DataBits>(ui->comboBoxDataBits->currentText().toInt()));
    serial->setParity(static_cast<QSerialPort::Parity>(ui->comboBoxParity->currentIndex()));
    serial->setStopBits(static_cast<QSerialPort::StopBits>(ui->comboBoxStopBits->currentIndex()));
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (serial->open(QIODevice::ReadWrite)) {
        QMessageBox::information(this, tr("成功"), tr("串口已打开"));
    } else {
        QMessageBox::critical(this, tr("错误"), serial->errorString());
    }
}

void MainWindow::closeSerialPort()
{
    if (serial->isOpen()) {
        serial->close();
        QMessageBox::information(this, tr("成功"), tr("串口已关闭"));
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
