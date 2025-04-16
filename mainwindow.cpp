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

    // 初始化偏压值
    for (int i = 2; i <= 7; ++i) {
        channelVoltages[i] = 0.0;
    }

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
    connect(ui->comboBoxChannel, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateCurrentVoltage);

    // 初始化接收定时器
    receiveTimer->setSingleShot(true);
    connect(receiveTimer, &QTimer::timeout, this, &MainWindow::processReceivedData);

    // 初始化通道选择下拉框
    QMap<int, QString> channelMap;
    channelMap[2] = "Y下";
    channelMap[3] = "Y上";
    channelMap[4] = "X下";
    channelMap[5] = "X上";
    channelMap[6] = "Y主";
    channelMap[7] = "X主";
    
    for(int i = 2; i <= 7; i++) {
        ui->comboBoxChannel->addItem(channelMap[i], i);  // 显示文本为映射值，实际值为通道号
    }

    // 连接新的信号槽
    connect(ui->buttonToggleMode, &QPushButton::clicked, this, &MainWindow::toggleControlMode);
    connect(ui->buttonSendControl, &QPushButton::clicked, this, &MainWindow::sendControlData);
    connect(ui->lineEditVoltage, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);
    connect(ui->buttonSetInitialValue, &QPushButton::clicked, this, &MainWindow::sendInitialValues);

    // 设置电压输入验证器
    QDoubleValidator *validator = new QDoubleValidator(-9.99, 9.99, 2, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    ui->lineEditVoltage->setValidator(validator);
    ui->lineEditInitialValueYQ->setValidator(validator);
    ui->lineEditInitialValueYI->setValidator(validator);
    ui->lineEditInitialValueXQ->setValidator(validator);
    ui->lineEditInitialValueXI->setValidator(validator);
    ui->lineEditInitialValueYP->setValidator(validator);
    ui->lineEditInitialValueXP->setValidator(validator);

    // 连接其他输入框的信号槽
    connect(ui->lineEditInitialValueYQ, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);
    connect(ui->lineEditInitialValueYI, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);
    connect(ui->lineEditInitialValueXQ, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);
    connect(ui->lineEditInitialValueXI, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);
    connect(ui->lineEditInitialValueYP, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);
    connect(ui->lineEditInitialValueXP, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);

    // 连接增减按钮的信号槽
    connect(ui->buttonVoltageMinus0, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClicked);
    connect(ui->buttonVoltageMinus1, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClicked);
    connect(ui->buttonVoltageMinus2, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClicked);
    connect(ui->buttonVoltagePlus2, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClicked);
    connect(ui->buttonVoltagePlus1, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClicked);
    connect(ui->buttonVoltagePlus0, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClicked);

    // 初始化控制模式按钮和发送控制命令按钮
    ui->buttonToggleMode->setEnabled(false);
    ui->buttonToggleMode->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonToggleMode->setText("手动模式");
    ui->buttonSendControl->setEnabled(false);
    ui->buttonSendControl->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->comboBoxChannel->setEnabled(false);

    // 禁用增减按钮
    ui->buttonVoltageMinus0->setEnabled(false);
    ui->buttonVoltageMinus0->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltageMinus1->setEnabled(false);
    ui->buttonVoltageMinus1->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltageMinus2->setEnabled(false);
    ui->buttonVoltageMinus2->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltagePlus2->setEnabled(false);
    ui->buttonVoltagePlus2->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltagePlus1->setEnabled(false);
    ui->buttonVoltagePlus1->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltagePlus0->setEnabled(false);
    ui->buttonVoltagePlus0->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->lineEditVoltage->setEnabled(false);

    // 更新当前偏压显示
    updateCurrentVoltage();
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

        // 禁用增减按钮
        ui->buttonVoltageMinus0->setEnabled(false);
        ui->buttonVoltageMinus0->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltageMinus1->setEnabled(false);
        ui->buttonVoltageMinus1->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltageMinus2->setEnabled(false);
        ui->buttonVoltageMinus2->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltagePlus2->setEnabled(false);
        ui->buttonVoltagePlus2->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltagePlus1->setEnabled(false);
        ui->buttonVoltagePlus1->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltagePlus0->setEnabled(false);
        ui->buttonVoltagePlus0->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->lineEditVoltage->setEnabled(false);
        
        // 启用串口设置控件
        ui->comboBoxPort->setEnabled(true);
        ui->comboBoxBaudRate->setEnabled(true);
        ui->comboBoxDataBits->setEnabled(true);
        ui->comboBoxParity->setEnabled(true);
        ui->comboBoxStopBits->setEnabled(true);
        ui->buttonRefreshPorts->setEnabled(true);
        ui->buttonRefreshPorts->setStyleSheet(""); // 恢复默认样式
        ui->comboBoxChannel->setEnabled(false);
        
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

            // 启用增减按钮
            ui->buttonVoltageMinus0->setEnabled(true);
            ui->buttonVoltageMinus0->setStyleSheet(""); // 恢复默认样式
            ui->buttonVoltageMinus1->setEnabled(true);
            ui->buttonVoltageMinus1->setStyleSheet(""); // 恢复默认样式
            ui->buttonVoltageMinus2->setEnabled(true);
            ui->buttonVoltageMinus2->setStyleSheet(""); // 恢复默认样式
            ui->buttonVoltagePlus2->setEnabled(true);
            ui->buttonVoltagePlus2->setStyleSheet(""); // 恢复默认样式
            ui->buttonVoltagePlus1->setEnabled(true);
            ui->buttonVoltagePlus1->setStyleSheet(""); // 恢复默认样式
            ui->buttonVoltagePlus0->setEnabled(true);
            ui->buttonVoltagePlus0->setStyleSheet(""); // 恢复默认样式
            ui->lineEditVoltage->setEnabled(true);
            
            // 禁用串口设置控件
            ui->comboBoxPort->setEnabled(false);
            ui->comboBoxBaudRate->setEnabled(false);
            ui->comboBoxDataBits->setEnabled(false);
            ui->comboBoxParity->setEnabled(false);
            ui->comboBoxStopBits->setEnabled(false);
            ui->buttonRefreshPorts->setEnabled(false);
            ui->buttonRefreshPorts->setStyleSheet("background-color: #9E9E9E;"); // 设置为灰色
            ui->comboBoxChannel->setEnabled(true);
            
            QMessageBox::information(this, tr("成功"), tr("串口已打开"));

            // 发送4开头的命令
            QByteArray data = "40000000000000000000000000";
            serial->write(data);
            QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
            ui->textEditHistory->append("[" + timestamp + "] 发送初始命令: " + data);
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

    QString data = QString::fromUtf8(receiveBuffer);
    QStringList lines = data.split('\n', Qt::SkipEmptyParts);  // 按行分割数据
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

    foreach(QString line, lines) {
        line = line.trimmed();  // 移除每行首尾的空白字符
        if (line.isEmpty()) continue;

        // 显示接收到的数据
        ui->textEditHistory->append("[" + timestamp + "] 接收: " + line);

        // 检查数据是否以"Bias Voltage"开头
        if (line.startsWith("Bias Voltage")) {
            // 移除"Bias Voltage:"前缀并分割数据
            QString values = line.mid(13).trimmed(); // "Bias Voltage:".length() == 13
            QStringList voltages = values.split(",", Qt::SkipEmptyParts); // 跳过空字符串
            
            // 确保有6个数据
            if (voltages.size() == 6) {
                // 更新UI显示，按YQ, YI, XQ, XI, YP, XP的顺序
                ui->labelYQValue->setText(voltages[0].trimmed());
                ui->labelYIValue->setText(voltages[1].trimmed());
                ui->labelXQValue->setText(voltages[2].trimmed());
                ui->labelXIValue->setText(voltages[3].trimmed());
                ui->labelYPValue->setText(voltages[4].trimmed());
                ui->labelXPValue->setText(voltages[5].trimmed());

                // 更新存储的偏压值
                channelVoltages[2] = voltages[0].toDouble(); // YQ
                channelVoltages[3] = voltages[1].toDouble(); // YI
                channelVoltages[4] = voltages[2].toDouble(); // XQ
                channelVoltages[5] = voltages[3].toDouble(); // XI
                channelVoltages[6] = voltages[4].toDouble(); // YP
                channelVoltages[7] = voltages[5].toDouble(); // XP

                // 更新当前偏压显示
                updateCurrentVoltage();
            }
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
        ui->buttonVoltageMinus0->setEnabled(false);
        ui->buttonVoltageMinus0->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltageMinus1->setEnabled(false);
        ui->buttonVoltageMinus1->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltageMinus2->setEnabled(false);
        ui->buttonVoltageMinus2->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltagePlus2->setEnabled(false);
        ui->buttonVoltagePlus2->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltagePlus1->setEnabled(false);
        ui->buttonVoltagePlus1->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltagePlus0->setEnabled(false);
        ui->buttonVoltagePlus0->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->lineEditVoltage->setEnabled(false);
        
        // 发送自动模式切换命令
        if (serial->isOpen()) {
            QByteArray data = "10000000000000000000000000";
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
        ui->buttonVoltageMinus0->setEnabled(true);
        ui->buttonVoltageMinus0->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltageMinus1->setEnabled(true);
        ui->buttonVoltageMinus1->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltageMinus2->setEnabled(true);
        ui->buttonVoltageMinus2->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltagePlus2->setEnabled(true);
        ui->buttonVoltagePlus2->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltagePlus1->setEnabled(true);
        ui->buttonVoltagePlus1->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltagePlus0->setEnabled(true);
        ui->buttonVoltagePlus0->setStyleSheet(""); // 恢复默认样式
        ui->lineEditVoltage->setEnabled(true);

        // 发送手动模式切换命令
        if (serial->isOpen()) {
            QByteArray data = "20000000000000000000000000";
            serial->write(data);
            QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
            ui->textEditHistory->append("[" + timestamp + "] 发送手动模式命令: " + data);
        }
    }
}

void MainWindow::validateVoltageInput(QLineEdit *lineEdit, const QString &text)
{
    if (!text.isEmpty()) {
        bool ok;
        double value = text.toDouble(&ok);
        if (!ok || value < -9.99 || value > 9.99) {
            lineEdit->setStyleSheet("background-color: #FFEBEE; border-color: #F44336;");
        } else {
            lineEdit->setStyleSheet("");
        }
    }
}

void MainWindow::onVoltageInputChanged(const QString &text)
{
    QLineEdit *lineEdit = qobject_cast<QLineEdit *>(sender());
    if (lineEdit) {
        validateVoltageInput(lineEdit, text);
    }
}

void MainWindow::sendControlData()
{
    int channel = ui->comboBoxChannel->currentData().toInt();
    double voltage = ui->lineEditVoltage->text().toDouble();
    sendChannelControlData(channel, voltage);
}

void MainWindow::sendChannelControlData(int channel, double voltage)
{
    if (!serial->isOpen()) {
        QMessageBox::warning(this, tr("错误"), tr("串口未打开"));
        return;
    }

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
    int intPart = static_cast<int>(absVoltage + 1e-6);  // 整数部分 加一个极小值，避免浮点误差
    
    // 正确处理小数部分
    int decimalPart = static_cast<int>(std::round(absVoltage * 100)) % 100;  // 两位小数
    int decimal1 = decimalPart / 10;  // 第一位小数
    int decimal2 = decimalPart % 10;  // 第二位小数
    
    // 将数字转换为字符并添加到数据中
    data.append('0' + static_cast<char>(intPart));
    data.append('0' + static_cast<char>(decimal1));
    data.append('0' + static_cast<char>(decimal2));

    // 补0到26位
    while (data.size() < 26) {
        data.append('0');
    }
    
    return data;
}

void MainWindow::updateCurrentVoltage()
{
    int channel = ui->comboBoxChannel->currentData().toInt();
    double voltage = channelVoltages.value(channel, 0.0);
    ui->labelCurrentVoltage->setText(QString("当前偏压：%1").arg(voltage, 0, 'f', 2));
    ui->lineEditVoltage->setText(QString::number(voltage, 'f', 2));
}

void MainWindow::onVoltageButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;

    int channel = ui->comboBoxChannel->currentData().toInt();
    double voltage = channelVoltages.value(channel, 0.0);

    if (button == ui->buttonVoltageMinus0) {
        voltage -= 1.0;
    } else if (button == ui->buttonVoltageMinus1) {
        voltage -= 0.1;
    } else if (button == ui->buttonVoltageMinus2) {
        voltage -= 0.01;
    } else if (button == ui->buttonVoltagePlus2) {
        voltage += 0.01;
    } else if (button == ui->buttonVoltagePlus1) {
        voltage += 0.1;
    } else if (button == ui->buttonVoltagePlus0) {
        voltage += 1.0;
    }

    // 限制电压值范围
    if (voltage < -9.99) voltage = -9.99;
    if (voltage > 9.99) voltage = 9.99;

    channelVoltages[channel] = voltage;
    sendChannelControlData(channel, voltage);

    updateCurrentVoltage();
}

void MainWindow::sendInitialValues()
{
    if (!serial->isOpen()) {
        QMessageBox::warning(this, tr("错误"), tr("串口未打开"));
        return;
    }

    QByteArray data;
    data.append('3');  // 命令以3开头

    // 获取六个文本框中的数值并按照指定格式添加到数据中
    QList<QLineEdit*> lineEdits = {
        ui->lineEditInitialValueYQ,
        ui->lineEditInitialValueYI,
        ui->lineEditInitialValueXQ,
        ui->lineEditInitialValueXI,
        ui->lineEditInitialValueYP,
        ui->lineEditInitialValueXP
    };

    for (QLineEdit *lineEdit : lineEdits) {
        double voltage = lineEdit->text().toDouble();
        data.append(voltage >= 0 ? '0' : '1');  // 符号位
        double absVoltage = std::abs(voltage);
        int intPart = static_cast<int>(absVoltage);  // 个位
        int decimalPart = static_cast<int>(std::round(absVoltage * 100)) % 100;  // 两位小数
        int decimal1 = decimalPart / 10;  // 小数十分位
        int decimal2 = decimalPart % 10;  // 小数百分位
        data.append('0' + static_cast<char>(intPart));
        data.append('0' + static_cast<char>(decimal1));
        data.append('0' + static_cast<char>(decimal2));
    }

    // 补0到26位
    while (data.size() < 26) {
        data.append('0');
    }

    serial->write(data);

    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    ui->textEditHistory->append("[" + timestamp + "] 发送初值命令: " + QString::fromUtf8(data));
}