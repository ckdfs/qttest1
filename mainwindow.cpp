#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDateTime>
#include <QDoubleValidator>
#include <QSerialPortInfo>
#include <algorithm>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serial(new QSerialPort(this))
    , isSerialPortOpen(false)
    , isAutoMode(false)
    , receiveTimer(new QTimer(this))
{
    ui->setupUi(this);

    // 设置窗口图标
    setWindowIcon(QIcon(":/images/icon.ico"));

    // 设置打开串口按钮的初始样式为红色
    ui->buttonTogglePort->setStyleSheet("background-color: #F44336;");

    // 设置数据位默认值为8
    ui->comboBoxDataBits->setCurrentText("8");

    // 设置波特率默认值为115200
    ui->comboBoxBaudRate->setCurrentText("115200");

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
    QMap<int, QString> channelMap;
    channelMap[2] = "YQ";
    channelMap[3] = "YI";
    channelMap[4] = "XQ";
    channelMap[5] = "XI";
    channelMap[6] = "YP";
    channelMap[7] = "XP";
    
    for(int i = 2; i <= 7; i++) {
        ui->comboBoxChannel->addItem(channelMap[i], i);  // 显示文本为映射值，实际值为通道号
    }

    // 连接新的信号槽
    connect(ui->buttonToggleMode, &QPushButton::clicked, this, &MainWindow::toggleControlMode);
    connect(ui->buttonSendControl, &QPushButton::clicked, this, &MainWindow::sendControlData);
    connect(ui->lineEditVoltage, &QLineEdit::textChanged, this, &MainWindow::validateVoltageInput);

    // 设置电压输入验证器
    QDoubleValidator *validator = new QDoubleValidator(-9.99, 9.99, 2, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    ui->lineEditVoltage->setValidator(validator);

    // 初始化控制模式按钮和发送控制命令按钮
    ui->buttonToggleMode->setEnabled(false);
    ui->buttonToggleMode->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonToggleMode->setText("手动模式");
    ui->buttonSendControl->setEnabled(false);
    ui->buttonSendControl->setStyleSheet("background-color: #9E9E9E;"); // 灰色
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
        ui->buttonToggleMode->setEnabled(false);
        ui->buttonToggleMode->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonSendControl->setEnabled(false);
        ui->buttonSendControl->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        
        // 启用串口设置控件
        ui->comboBoxPort->setEnabled(true);
        ui->comboBoxBaudRate->setEnabled(true);
        ui->comboBoxDataBits->setEnabled(true);
        ui->comboBoxParity->setEnabled(true);
        ui->comboBoxStopBits->setEnabled(true);
        ui->buttonRefreshPorts->setEnabled(true);
        ui->buttonRefreshPorts->setStyleSheet(""); // 恢复默认样式
        
        QMessageBox::information(this, tr("成功"), tr("串口已关闭"));
    } else {
        QString portName = ui->comboBoxPort->currentText();  // 直接使用显示的文本
        
        #ifdef Q_OS_LINUX
        if (portName.startsWith("/dev/")) {
            portName = portName.mid(5);
        }
        #endif
        
        serial->setPortName(portName);
        
        serial->setBaudRate(ui->comboBoxBaudRate->currentText().toInt());
        serial->setDataBits(static_cast<QSerialPort::DataBits>(ui->comboBoxDataBits->currentText().toInt()));
        serial->setParity(static_cast<QSerialPort::Parity>(ui->comboBoxParity->currentIndex()));
        serial->setStopBits(static_cast<QSerialPort::StopBits>(ui->comboBoxStopBits->currentIndex()));
        serial->setFlowControl(QSerialPort::NoFlowControl);

        if (serial->open(QIODevice::ReadWrite)) {
            ui->buttonTogglePort->setText("关闭串口");
            ui->buttonTogglePort->setStyleSheet("background-color: #4CAF50;");
            ui->buttonToggleMode->setEnabled(true);
            ui->buttonToggleMode->setStyleSheet("background-color: #F44336;"); // 红色
            ui->buttonSendControl->setEnabled(true);
            ui->buttonSendControl->setStyleSheet("background-color: #2196F3;"); // 蓝色
            
            // 禁用串口设置控件
            ui->comboBoxPort->setEnabled(false);
            ui->comboBoxBaudRate->setEnabled(false);
            ui->comboBoxDataBits->setEnabled(false);
            ui->comboBoxParity->setEnabled(false);
            ui->comboBoxStopBits->setEnabled(false);
            ui->buttonRefreshPorts->setEnabled(false);
            ui->buttonRefreshPorts->setStyleSheet("background-color: #9E9E9E;"); // 设置为灰色
            
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
    
    // 自定义排序函数
    std::sort(ports.begin(), ports.end(), [](const QSerialPortInfo &a, const QSerialPortInfo &b) {
        #ifdef Q_OS_LINUX
        QString aName = a.portName();
        QString bName = b.portName();
        if (aName.startsWith("ttyUSB") && bName.startsWith("ttyUSB")) {
            return aName.mid(6).toInt() < bName.mid(6).toInt();
        } else if (aName.startsWith("ttyACM") && bName.startsWith("ttyACM")) {
            return aName.mid(6).toInt() < bName.mid(6).toInt();
        }
        return aName < bName;
        #else
        QString aName = a.portName().toLower();
        QString bName = b.portName().toLower();
        aName.remove("com");
        bName.remove("com");
        return aName.toInt() < bName.toInt();
        #endif
    });
    
    foreach (const QSerialPortInfo &info, ports) {
        QString portName;
        
        #ifdef Q_OS_LINUX
        portName = "/dev/" + info.portName();
        #else
        portName = info.portName();
        #endif
        
        ui->comboBoxPort->addItem(portName);
    }

    if (ui->comboBoxPort->count() > 0) {
        ui->comboBoxPort->setCurrentIndex(0);
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
    QString data = QString::fromUtf8(receiveBuffer);
    ui->textEditHistory->append("[" + timestamp + "] 接收: " + data);

    // 检查数据是否以"Bias Voltage:"开头
    if (data.startsWith("Bias Voltage")) {
        // 移除"Bias Voltage:"前缀并分割数据
        QString values = data.mid(13).trimmed(); // "Bias Voltage:".length() == 13
        QStringList voltages = values.split(",", Qt::SkipEmptyParts); // 跳过空字符串
        
        // 确保有6个数据
        if (voltages.size() == 6) {
            // 更新UI显示，按XI,XQ,XP,YI,YQ,YP的顺序
            ui->labelXIValue->setText(voltages[0].trimmed());  // 添加trimmed()去除每个值可能的空白字符
            ui->labelXQValue->setText(voltages[1].trimmed());
            ui->labelXPValue->setText(voltages[2].trimmed());
            ui->labelYIValue->setText(voltages[3].trimmed());
            ui->labelYQValue->setText(voltages[4].trimmed());
            ui->labelYPValue->setText(voltages[5].trimmed());
        }
    }

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
        ui->buttonSendControl->setEnabled(false);  // 禁用发送控制命令按钮
        ui->buttonSendControl->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        
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
        ui->buttonSendControl->setEnabled(true);  // 启用发送控制命令按钮
        ui->buttonSendControl->setStyleSheet("background-color: #2196F3;"); // 蓝色
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
    
    int channel = ui->comboBoxChannel->currentData().toInt();
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
    
    // 通道选通
    data.append('0' + static_cast<char>(channel));
    
    // 处理电压值
    double absVoltage = std::abs(voltage);
    int intPart = static_cast<int>(absVoltage);  // 整数部分
    
    // 正确处理小数部分
    int decimalPart = static_cast<int>(std::round(absVoltage * 100)) % 100;  // 两位小数
    int decimal1 = decimalPart / 10;  // 第一位小数
    int decimal2 = decimalPart % 10;  // 第二位小数
    
    // 将数字转换为字符并添加到数据中
    data.append('0' + static_cast<char>(intPart));
    data.append('0' + static_cast<char>(decimal1));
    data.append('0' + static_cast<char>(decimal2));
    
    return data;
}
