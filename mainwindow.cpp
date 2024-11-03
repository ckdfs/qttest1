#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QDoubleValidator>
#include <QSerialPortInfo>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serial(new QSerialPort(this))
    , isSerialPortOpen(false)
    , isAutoMode(false)
    , receiveTimer(new QTimer(this))
{
    ui->setupUi(this);

    // Populate serial port combo box
    refreshSerialPorts();

    // Connect signals and slots
    connect(ui->buttonTogglePort, &QPushButton::clicked, this, &MainWindow::toggleSerialPort);
    connect(ui->buttonRefreshPorts, &QPushButton::clicked, this, &MainWindow::refreshSerialPorts);
    connect(ui->buttonClearHistory, &QPushButton::clicked, this, &MainWindow::clearHistory);
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readSerialData);

    // 初始化接收定时器
    receiveTimer->setSingleShot(true);
    connect(receiveTimer, &QTimer::timeout, this, &MainWindow::processReceivedData);

    // 初始化通道选择下拉框
    for(int i = 2; i <= 7; i++) {
        ui->comboBoxChannel->addItem(QString::number(i));
    }

    // 连接新的信号槽
    connect(ui->buttonToggleMode, &QPushButton::clicked, this, &MainWindow::toggleControlMode);
    connect(ui->buttonSendControl, &QPushButton::clicked, this, &MainWindow::sendControlData);
    connect(ui->lineEditVoltage, &QLineEdit::textChanged, this, &MainWindow::validateVoltageInput);

    // 设置电压输入验证器
    QDoubleValidator *validator = new QDoubleValidator(-9.99, 9.99, 2, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    ui->lineEditVoltage->setValidator(validator);

    // 初始化控制模式按钮
    ui->buttonToggleMode->setStyleSheet("background-color: red");
    ui->buttonToggleMode->setText("手动模式");
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
        ui->buttonTogglePort->setStyleSheet("background-color: #F44336;");
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
            ui->buttonTogglePort->setStyleSheet("background-color: #4CAF50;");
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
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    
    // 自定义排序函数，按COM号排序
    std::sort(ports.begin(), ports.end(), [](const QSerialPortInfo &a, const QSerialPortInfo &b) {
        QString aName = a.portName().toLower();
        QString bName = b.portName().toLower();
        aName.remove("com");
        bName.remove("com");
        return aName.toInt() < bName.toInt();
    });
    
    foreach (const QSerialPortInfo &info, ports) {
        ui->comboBoxPort->addItem(info.portName());
    }

    if (ui->comboBoxPort->count() > 0) {
        ui->comboBoxPort->setCurrentIndex(ui->comboBoxPort->count() - 1);
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

void MainWindow::toggleControlMode()
{
    isAutoMode = !isAutoMode;
    if (isAutoMode) {
        ui->buttonToggleMode->setText("自动模式");
        ui->buttonToggleMode->setStyleSheet("background-color: #4CAF50;");
        ui->comboBoxChannel->setEnabled(false);
        ui->lineEditVoltage->setEnabled(false);
        
        // 发送自动模式切换命令
        if (serial->isOpen()) {
            QByteArray data = "100000";
            serial->write(data);
            QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
            ui->textEditHistory->append("[" + timestamp + "] 发送自动模式命令: " + data);
        }
    } else {
        ui->buttonToggleMode->setText("手动模式");
        ui->buttonToggleMode->setStyleSheet("background-color: #F44336;");
        ui->comboBoxChannel->setEnabled(true);
        ui->lineEditVoltage->setEnabled(true);
    }
}

void MainWindow::validateVoltageInput(const QString &text)
{
    if (!text.isEmpty()) {
        bool ok;
        double value = text.toDouble(&ok);
        if (!ok || value < -9.99 || value > 9.99) {
            ui->lineEditVoltage->setStyleSheet("background-color: #FFEBEE; border-color: #F44336;");
        } else {
            ui->lineEditVoltage->setStyleSheet("");
        }
    }
}

void MainWindow::sendControlData()
{
    if (!serial->isOpen()) {
        QMessageBox::warning(this, tr("错误"), tr("串口未打开"));
        return;
    }
    
    int channel = ui->comboBoxChannel->currentText().toInt();
    double voltage = ui->lineEditVoltage->text().toDouble();
    
    QByteArray data = packProtocolData(isAutoMode, voltage, channel);
    serial->write(data);
    
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    ui->textEditHistory->append("[" + timestamp + "] 发送控制命令: " + QString::fromUtf8(data));
}

QByteArray MainWindow::packProtocolData(bool isAuto, double voltage, int channel)
{
    QByteArray data;
    
    // 控制位
    data.append(isAuto ? '1' : '2');
    
    // 符号位
    data.append(voltage >= 0 ? '0' : '1');
    
    // 通道选通 (将整数转换为字符)
    data.append('0' + static_cast<char>(channel));
    
    // 处理电压值
    double absVoltage = std::abs(voltage);
    int intPart = static_cast<int>(absVoltage);
    int decimal1 = static_cast<int>((absVoltage * 10)) % 10;
    int decimal2 = static_cast<int>((absVoltage * 100)) % 10;
    
    // 将数字转换为字符并添加到数据中
    data.append('0' + static_cast<char>(intPart));
    data.append('0' + static_cast<char>(decimal1));
    data.append('0' + static_cast<char>(decimal2));
    
    return data;
}
