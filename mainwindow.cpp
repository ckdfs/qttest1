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
        this->channelVoltagesCW[i] = 0.0;
        this->channelVoltagesCCW[i] = 0.0;
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
    connect(ui->comboBoxChannelCW, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateCurrentVoltage);

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
        ui->comboBoxChannelCW->addItem(channelMap[i], i);  // 显示文本为映射值，实际值为通道号
    }

    // 连接新的信号槽
    connect(ui->buttonToggleMode, &QPushButton::clicked, this, &MainWindow::toggleControlMode);
    connect(ui->buttonSendControlCW, &QPushButton::clicked, this, &MainWindow::sendControlDataCW);
    connect(ui->lineEditVoltageCW, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);
    // connect(ui->buttonSetInitialValue, &QPushButton::clicked, this, &MainWindow::sendInitialValues);

    // 设置电压输入验证器
    QDoubleValidator *validator = new QDoubleValidator(-9.99, 9.99, 2, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    ui->lineEditVoltageCW->setValidator(validator);
    // ui->lineEditInitialValueYQ->setValidator(validator);
    // ui->lineEditInitialValueYI->setValidator(validator);
    // ui->lineEditInitialValueXQ->setValidator(validator);
    // ui->lineEditInitialValueXI->setValidator(validator);
    // ui->lineEditInitialValueYP->setValidator(validator);
    // ui->lineEditInitialValueXP->setValidator(validator);

    // 设置功率差输入验证器
    QDoubleValidator *powerDiffValidator = new QDoubleValidator(0.0, 9.99, 2, this);
    powerDiffValidator->setNotation(QDoubleValidator::StandardNotation);
    ui->lineEditCurrentPowerDiff->setValidator(powerDiffValidator);
    ui->lineEditTargetPowerDiff->setValidator(powerDiffValidator);

    // 连接其他输入框的信号槽
    // connect(ui->lineEditInitialValueYQ, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);
    // connect(ui->lineEditInitialValueYI, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);
    // connect(ui->lineEditInitialValueXQ, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);
    // connect(ui->lineEditInitialValueXI, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);
    // connect(ui->lineEditInitialValueYP, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);
    // connect(ui->lineEditInitialValueXP, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChanged);

    // 连接增减按钮的信号槽
    connect(ui->buttonVoltageMinus0_CW, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClicked);
    connect(ui->buttonVoltageMinus1_CW, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClicked);
    connect(ui->buttonVoltageMinus2_CW, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClicked);
    connect(ui->buttonVoltagePlus2_CW, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClicked);
    connect(ui->buttonVoltagePlus1_CW, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClicked);
    connect(ui->buttonVoltagePlus0_CW, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClicked);

    // 连接设置功率差按钮的信号槽
    connect(ui->buttonSetPowerDiff, &QPushButton::clicked, this, &MainWindow::sendPowerDiffCommand);

    // 初始化控制模式按钮和发送控制命令按钮
    ui->buttonToggleMode->setEnabled(false);
    ui->buttonToggleMode->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonToggleMode->setText("手动模式");
    ui->buttonSendControlCW->setEnabled(false);
    ui->buttonSendControlCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonSendControlCCW->setEnabled(false);
    ui->buttonSendControlCCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->comboBoxChannelCW->setEnabled(false);
    ui->lineEditVoltageCW->setEnabled(false);
    ui->comboBoxChannelCCW->setEnabled(false);
    ui->lineEditVoltageCCW->setEnabled(false);
    // 禁用增减按钮
    ui->buttonVoltageMinus0_CW->setEnabled(false);
    ui->buttonVoltageMinus0_CW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltageMinus1_CW->setEnabled(false);
    ui->buttonVoltageMinus1_CW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltageMinus2_CW->setEnabled(false);
    ui->buttonVoltageMinus2_CW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltagePlus2_CW->setEnabled(false);
    ui->buttonVoltagePlus2_CW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltagePlus1_CW->setEnabled(false);
    ui->buttonVoltagePlus1_CW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltagePlus0_CW->setEnabled(false);
    ui->buttonVoltagePlus0_CW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltageMinus0_CCW->setEnabled(false);
    ui->buttonVoltageMinus0_CCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltageMinus1_CCW->setEnabled(false);
    ui->buttonVoltageMinus1_CCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltageMinus2_CCW->setEnabled(false);
    ui->buttonVoltageMinus2_CCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltagePlus2_CCW->setEnabled(false);
    ui->buttonVoltagePlus2_CCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltagePlus1_CCW->setEnabled(false);
    ui->buttonVoltagePlus1_CCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->buttonVoltagePlus0_CCW->setEnabled(false);
    ui->buttonVoltagePlus0_CCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
    ui->lineEditVoltageCW->setEnabled(false);

    // 更新当前偏压显示
    updateCurrentVoltage();

    powerDiffEnabled = false;
    updatePowerDiffUi();

    // CW行
    connect(ui->comboBoxChannelCW, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateCurrentVoltageCW);
    connect(ui->lineEditVoltageCW, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChangedCW);
    // CCW行
    connect(ui->comboBoxChannelCCW, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateCurrentVoltageCCW);
    connect(ui->lineEditVoltageCCW, &QLineEdit::textChanged, this, &MainWindow::onVoltageInputChangedCCW);
    // 设置电压输入验证器
    QDoubleValidator *validatorCW = new QDoubleValidator(-9.99, 9.99, 2, this);
    validatorCW->setNotation(QDoubleValidator::StandardNotation);
    ui->lineEditVoltageCW->setValidator(validatorCW);
    QDoubleValidator *validatorCCW = new QDoubleValidator(-9.99, 9.99, 2, this);
    validatorCCW->setNotation(QDoubleValidator::StandardNotation);
    ui->lineEditVoltageCCW->setValidator(validatorCCW);
    // 初始化通道下拉框（与主通道一致）
    for(int i = 2; i <= 7; i++) {
        ui->comboBoxChannelCCW->addItem(channelMap[i], i);
    }
    // 初始化显示
    updateCurrentVoltageCW();
    updateCurrentVoltageCCW();

    // 连接CCW增减按钮的信号槽
    connect(ui->buttonVoltageMinus0_CCW, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClickedCCW);
    connect(ui->buttonVoltageMinus1_CCW, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClickedCCW);
    connect(ui->buttonVoltageMinus2_CCW, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClickedCCW);
    connect(ui->buttonVoltagePlus2_CCW, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClickedCCW);
    connect(ui->buttonVoltagePlus1_CCW, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClickedCCW);
    connect(ui->buttonVoltagePlus0_CCW, &QPushButton::clicked, this, &MainWindow::onVoltageButtonClickedCCW);
    // 连接CCW发送按钮
    connect(ui->buttonSendControlCCW, &QPushButton::clicked, this, &MainWindow::sendControlDataCCW);
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
        ui->buttonSendControlCW->setEnabled(false);
        ui->buttonSendControlCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonSendControlCCW->setEnabled(false);
        ui->buttonSendControlCCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        // 禁用CW/CCW控件
        ui->comboBoxChannelCW->setEnabled(false);
        ui->lineEditVoltageCW->setEnabled(false);
        ui->comboBoxChannelCCW->setEnabled(false);
        ui->lineEditVoltageCCW->setEnabled(false);
        // 禁用CW增减按钮
        ui->buttonVoltageMinus0_CW->setEnabled(false);
        ui->buttonVoltageMinus0_CW->setStyleSheet("background-color: #9E9E9E;");
        ui->buttonVoltageMinus1_CW->setEnabled(false);
        ui->buttonVoltageMinus1_CW->setStyleSheet("background-color: #9E9E9E;");
        ui->buttonVoltageMinus2_CW->setEnabled(false);
        ui->buttonVoltageMinus2_CW->setStyleSheet("background-color: #9E9E9E;");
        ui->buttonVoltagePlus2_CW->setEnabled(false);
        ui->buttonVoltagePlus2_CW->setStyleSheet("background-color: #9E9E9E;");
        ui->buttonVoltagePlus1_CW->setEnabled(false);
        ui->buttonVoltagePlus1_CW->setStyleSheet("background-color: #9E9E9E;");
        ui->buttonVoltagePlus0_CW->setEnabled(false);
        ui->buttonVoltagePlus0_CW->setStyleSheet("background-color: #9E9E9E;");
        // 禁用CCW增减按钮
        ui->buttonVoltageMinus0_CCW->setEnabled(false);
        ui->buttonVoltageMinus0_CCW->setStyleSheet("background-color: #9E9E9E;");
        ui->buttonVoltageMinus1_CCW->setEnabled(false);
        ui->buttonVoltageMinus1_CCW->setStyleSheet("background-color: #9E9E9E;");
        ui->buttonVoltageMinus2_CCW->setEnabled(false);
        ui->buttonVoltageMinus2_CCW->setStyleSheet("background-color: #9E9E9E;");
        ui->buttonVoltagePlus2_CCW->setEnabled(false);
        ui->buttonVoltagePlus2_CCW->setStyleSheet("background-color: #9E9E9E;");
        ui->buttonVoltagePlus1_CCW->setEnabled(false);
        ui->buttonVoltagePlus1_CCW->setStyleSheet("background-color: #9E9E9E;");
        ui->buttonVoltagePlus0_CCW->setEnabled(false);
        ui->buttonVoltagePlus0_CCW->setStyleSheet("background-color: #9E9E9E;");
        
        // 启用串口设置控件
        ui->comboBoxPort->setEnabled(true);
        ui->comboBoxBaudRate->setEnabled(true);
        ui->comboBoxDataBits->setEnabled(true);
        ui->comboBoxParity->setEnabled(true);
        ui->comboBoxStopBits->setEnabled(true);
        ui->buttonRefreshPorts->setEnabled(true);
        ui->buttonRefreshPorts->setStyleSheet(""); // 恢复默认样式
        ui->comboBoxChannelCW->setEnabled(false);
        
        QMessageBox::information(this, tr("成功"), tr("串口已关闭"));

        powerDiffEnabled = false;
        updatePowerDiffUi();
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
            ui->buttonSendControlCW->setEnabled(true);
            ui->buttonSendControlCW->setStyleSheet("background-color: #2196F3;"); // 蓝色
            ui->buttonSendControlCCW->setEnabled(true);
            ui->buttonSendControlCCW->setStyleSheet("background-color: #2196F3;"); // 蓝色
            // 启用CW/CCW控件
            ui->comboBoxChannelCW->setEnabled(true);
            ui->lineEditVoltageCW->setEnabled(true);
            ui->comboBoxChannelCCW->setEnabled(true);
            ui->lineEditVoltageCCW->setEnabled(true);
            // 启用增减按钮
            ui->buttonVoltageMinus0_CW->setEnabled(true);
            ui->buttonVoltageMinus0_CW->setStyleSheet(""); // 恢复默认样式
            ui->buttonVoltageMinus1_CW->setEnabled(true);
            ui->buttonVoltageMinus1_CW->setStyleSheet(""); // 恢复默认样式
            ui->buttonVoltageMinus2_CW->setEnabled(true);
            ui->buttonVoltageMinus2_CW->setStyleSheet(""); // 恢复默认样式
            ui->buttonVoltagePlus2_CW->setEnabled(true);
            ui->buttonVoltagePlus2_CW->setStyleSheet(""); // 恢复默认样式
            ui->buttonVoltagePlus1_CW->setEnabled(true);
            ui->buttonVoltagePlus1_CW->setStyleSheet(""); // 恢复默认样式
            ui->buttonVoltagePlus0_CW->setEnabled(true);
            ui->buttonVoltagePlus0_CW->setStyleSheet(""); // 恢复默认样式
            ui->buttonVoltageMinus0_CCW->setEnabled(true);
            ui->buttonVoltageMinus0_CCW->setStyleSheet("");
            ui->buttonVoltageMinus1_CCW->setEnabled(true);
            ui->buttonVoltageMinus1_CCW->setStyleSheet("");
            ui->buttonVoltageMinus2_CCW->setEnabled(true);
            ui->buttonVoltageMinus2_CCW->setStyleSheet("");
            ui->buttonVoltagePlus2_CCW->setEnabled(true);
            ui->buttonVoltagePlus2_CCW->setStyleSheet("");
            ui->buttonVoltagePlus1_CCW->setEnabled(true);
            ui->buttonVoltagePlus1_CCW->setStyleSheet("");
            ui->buttonVoltagePlus0_CCW->setEnabled(true);
            ui->buttonVoltagePlus0_CCW->setStyleSheet("");
            ui->lineEditVoltageCW->setEnabled(true);
            ui->lineEditVoltageCCW->setEnabled(true);
            
            // 禁用串口设置控件
            ui->comboBoxPort->setEnabled(false);
            ui->comboBoxBaudRate->setEnabled(false);
            ui->comboBoxDataBits->setEnabled(false);
            ui->comboBoxParity->setEnabled(false);
            ui->comboBoxStopBits->setEnabled(false);
            ui->buttonRefreshPorts->setEnabled(false);
            ui->buttonRefreshPorts->setStyleSheet("background-color: #9E9E9E;"); // 设置为灰色
            ui->comboBoxChannelCW->setEnabled(true);
            
            QMessageBox::information(this, tr("成功"), tr("串口已打开"));

            // 发送4开头的命令
            QByteArray data = "40000000000000000000000000";
            serial->write(data);
            QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
            ui->textEditHistory->append("[" + timestamp + "] 发送初始命令: " + data);

            powerDiffEnabled = false;
            updatePowerDiffUi();
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

        // 检查是否收到6000...命令
        if (line.startsWith("60000000000000000000000000")) {
            powerDiffEnabled = true;
            updatePowerDiffUi();
        }

        // 检查数据是否以"Bias Voltage"开头
        if (line.startsWith("Bias Voltage")) {
            // 移除"Bias Voltage:"前缀并分割数据
            QString values = line.mid(13).trimmed(); // "Bias Voltage:".length() == 13
            QStringList parts = values.split(",", Qt::SkipEmptyParts); // 跳过空字符串

            // 判断最后一位是否为CW/CCW标志
            int channelFlag = 0;
            if (parts.size() == 7) {
                channelFlag = parts.last().toInt();
                parts.removeLast();
            }
            // parts 现在为6个电压值
            if (parts.size() == 6) {
                if (channelFlag == 2) {
                    // CW
                    this->channelVoltagesCW[2] = parts[0].toDouble(); // YQ
                    this->channelVoltagesCW[3] = parts[1].toDouble(); // YI
                    this->channelVoltagesCW[4] = parts[2].toDouble(); // XQ
                    this->channelVoltagesCW[5] = parts[3].toDouble(); // XI
                    this->channelVoltagesCW[6] = parts[4].toDouble(); // YP
                    this->channelVoltagesCW[7] = parts[5].toDouble(); // XP
                    updateCurrentVoltageCW();
                } else if (channelFlag == 3) {
                    // CCW
                    this->channelVoltagesCCW[2] = parts[0].toDouble(); // YQ
                    this->channelVoltagesCCW[3] = parts[1].toDouble(); // YI
                    this->channelVoltagesCCW[4] = parts[2].toDouble(); // XQ
                    this->channelVoltagesCCW[5] = parts[3].toDouble(); // XI
                    this->channelVoltagesCCW[6] = parts[4].toDouble(); // YP
                    this->channelVoltagesCCW[7] = parts[5].toDouble(); // XP
                    updateCurrentVoltageCCW();
                } else {
                    // 未指定，默认CW
                    this->channelVoltagesCW[2] = parts[0].toDouble();
                    this->channelVoltagesCW[3] = parts[1].toDouble();
                    this->channelVoltagesCW[4] = parts[2].toDouble();
                    this->channelVoltagesCW[5] = parts[3].toDouble();
                    this->channelVoltagesCW[6] = parts[4].toDouble();
                    this->channelVoltagesCW[7] = parts[5].toDouble();
                    updateCurrentVoltageCW();
                }
                // UI显示（如有总览标签）可选
                ui->labelYQValue->setText(parts[0].trimmed());
                ui->labelYIValue->setText(parts[1].trimmed());
                ui->labelXQValue->setText(parts[2].trimmed());
                ui->labelXIValue->setText(parts[3].trimmed());
                ui->labelYPValue->setText(parts[4].trimmed());
                ui->labelXPValue->setText(parts[5].trimmed());
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
        ui->comboBoxChannelCW->setEnabled(false);
        ui->lineEditVoltageCW->setEnabled(false);
        ui->comboBoxChannelCCW->setEnabled(false);
        ui->lineEditVoltageCCW->setEnabled(false);
        ui->buttonSendControlCW->setEnabled(false);  // 禁用发送控制命令按钮
        ui->buttonSendControlCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonSendControlCCW->setEnabled(false);  // 禁用发送控制命令按钮
        ui->buttonSendControlCCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltageMinus0_CW->setEnabled(false);
        ui->buttonVoltageMinus0_CW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltageMinus1_CW->setEnabled(false);
        ui->buttonVoltageMinus1_CW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltageMinus2_CW->setEnabled(false);
        ui->buttonVoltageMinus2_CW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltagePlus2_CW->setEnabled(false);
        ui->buttonVoltagePlus2_CW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltagePlus1_CW->setEnabled(false);
        ui->buttonVoltagePlus1_CW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltagePlus0_CW->setEnabled(false);
        ui->buttonVoltagePlus0_CW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltageMinus0_CCW->setEnabled(false);
        ui->buttonVoltageMinus0_CCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltageMinus1_CCW->setEnabled(false);
        ui->buttonVoltageMinus1_CCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltageMinus2_CCW->setEnabled(false);
        ui->buttonVoltageMinus2_CCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltagePlus2_CCW->setEnabled(false);
        ui->buttonVoltagePlus2_CCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltagePlus1_CCW->setEnabled(false);
        ui->buttonVoltagePlus1_CCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        ui->buttonVoltagePlus0_CCW->setEnabled(false);
        ui->buttonVoltagePlus0_CCW->setStyleSheet("background-color: #9E9E9E;"); // 灰色
        
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
        ui->comboBoxChannelCW->setEnabled(true);
        ui->lineEditVoltageCW->setEnabled(true);
        ui->comboBoxChannelCCW->setEnabled(true);
        ui->lineEditVoltageCCW->setEnabled(true);
        ui->buttonSendControlCW->setEnabled(true);  // 启用发送控制命令按钮
        ui->buttonSendControlCW->setStyleSheet("background-color: #2196F3;"); // 蓝色
        ui->buttonSendControlCCW->setEnabled(true);  // 启用发送控制命令按钮
        ui->buttonSendControlCCW->setStyleSheet("background-color: #2196F3;"); // 蓝色
        ui->buttonVoltageMinus0_CW->setEnabled(true);
        ui->buttonVoltageMinus0_CW->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltageMinus1_CW->setEnabled(true);
        ui->buttonVoltageMinus1_CW->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltageMinus2_CW->setEnabled(true);
        ui->buttonVoltageMinus2_CW->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltagePlus2_CW->setEnabled(true);
        ui->buttonVoltagePlus2_CW->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltagePlus1_CW->setEnabled(true);
        ui->buttonVoltagePlus1_CW->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltagePlus0_CW->setEnabled(true);
        ui->buttonVoltagePlus0_CW->setStyleSheet(""); // 恢复默认样式
        // 启用CCW增减按钮
        ui->buttonVoltageMinus0_CCW->setEnabled(true);
        ui->buttonVoltageMinus0_CCW->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltageMinus1_CCW->setEnabled(true);
        ui->buttonVoltageMinus1_CCW->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltageMinus2_CCW->setEnabled(true);
        ui->buttonVoltageMinus2_CCW->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltagePlus2_CCW->setEnabled(true);
        ui->buttonVoltagePlus2_CCW->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltagePlus1_CCW->setEnabled(true);
        ui->buttonVoltagePlus1_CCW->setStyleSheet(""); // 恢复默认样式
        ui->buttonVoltagePlus0_CCW->setEnabled(true);
        ui->buttonVoltagePlus0_CCW->setStyleSheet(""); // 恢复默认样式
        ui->lineEditVoltageCW->setEnabled(true);
        ui->lineEditVoltageCCW->setEnabled(true);

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
    if (!serial->isOpen()) {
        QMessageBox::warning(this, tr("错误"), tr("串口未打开"));
        return;
    }
    // 先发送CW（2开头）
    int channelCW = ui->comboBoxChannelCW->currentData().toInt();
    double voltageCW = ui->lineEditVoltageCW->text().toDouble();
    QByteArray dataCW = packProtocolData(false, voltageCW, channelCW); // false=手动模式，2开头
    dataCW[0] = '2'; // 明确2开头
    serial->write(dataCW);
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    ui->textEditHistory->append("[" + timestamp + "] 发送CW控制命令: " + QString::fromUtf8(dataCW));
    // 再发送CCW（3开头）
    int channelCCW = ui->comboBoxChannelCCW->currentData().toInt();
    double voltageCCW = ui->lineEditVoltageCCW->text().toDouble();
    QByteArray dataCCW = packProtocolData(false, voltageCCW, channelCCW); // false=手动模式
    dataCCW[0] = '3'; // 3开头
    serial->write(dataCCW);
    ui->textEditHistory->append("[" + timestamp + "] 发送CCW控制命令: " + QString::fromUtf8(dataCCW));
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
    int channel = ui->comboBoxChannelCW->currentData().toInt();
    double voltage = this->channelVoltagesCW.value(channel, 0.0);
    ui->labelCurrentVoltageCW->setText(QString("当前偏压：%1").arg(voltage, 0, 'f', 2));
    ui->lineEditVoltageCW->setText(QString::number(voltage, 'f', 2));
}

void MainWindow::onVoltageButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;

    int channel = ui->comboBoxChannelCW->currentData().toInt();
    double voltage = this->channelVoltagesCW.value(channel, 0.0);

    if (button == ui->buttonVoltageMinus0_CW) {
        voltage -= 1.0;
    } else if (button == ui->buttonVoltageMinus1_CW) {
        voltage -= 0.1;
    } else if (button == ui->buttonVoltageMinus2_CW) {
        voltage -= 0.01;
    } else if (button == ui->buttonVoltagePlus2_CW) {
        voltage += 0.01;
    } else if (button == ui->buttonVoltagePlus1_CW) {
        voltage += 0.1;
    } else if (button == ui->buttonVoltagePlus0_CW) {
        voltage += 1.0;
    }

    // 限制电压值范围
    if (voltage < -9.99) voltage = -9.99;
    if (voltage > 9.99) voltage = 9.99;

    this->channelVoltagesCW[channel] = voltage;
    // 发送CW协议（2开头）
    QByteArray data = packProtocolData(false, voltage, channel);
    data[0] = '2';
    if (serial->isOpen()) {
        serial->write(data);
        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
        ui->textEditHistory->append("[" + timestamp + "] 发送CW控制命令: " + QString::fromUtf8(data));
    }
    updateCurrentVoltageCW();
}

void MainWindow::sendPowerDiffCommand()
{
    if (!serial->isOpen()) {
        QMessageBox::warning(this, tr("错误"), tr("串口未打开"));
        return;
    }

    QString curStr = ui->lineEditCurrentPowerDiff->text();
    QString tarStr = ui->lineEditTargetPowerDiff->text();

    bool ok1 = false, ok2 = false;
    double curVal = curStr.toDouble(&ok1);
    double tarVal = tarStr.toDouble(&ok2);

    if (!ok1 || !ok2 || curVal < 0 || curVal > 9.99 || tarVal < 0 || tarVal > 9.99) {
        QMessageBox::warning(this, tr("错误"), tr("请输入0~9.99之间的功率差值"));
        return;
    }

    int curInt = static_cast<int>(std::round(curVal * 100));
    int tarInt = static_cast<int>(std::round(tarVal * 100));

    QString curStr3 = QString("%1").arg(curInt, 3, 10, QChar('0'));
    QString tarStr3 = QString("%1").arg(tarInt, 3, 10, QChar('0'));

    QByteArray data = "5" + curStr3.toUtf8() + tarStr3.toUtf8();
    while (data.size() < 26) data.append('0');

    serial->write(data);

    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    ui->textEditHistory->append("[" + timestamp + "] 发送功率差命令: " + QString::fromUtf8(data));
}

void MainWindow::updatePowerDiffUi()
{
    ui->buttonSetPowerDiff->setEnabled(powerDiffEnabled);
    ui->buttonSetPowerDiff->setStyleSheet(powerDiffEnabled ? "background-color: #2196F3; color: white;" : "background-color: #9E9E9E; color: white;");
    ui->lineEditCurrentPowerDiff->setEnabled(powerDiffEnabled);
    ui->lineEditTargetPowerDiff->setEnabled(powerDiffEnabled);
}

void MainWindow::updateCurrentVoltageCW()
{
    int channel = ui->comboBoxChannelCW->currentData().toInt();
    double voltage = this->channelVoltagesCW.value(channel, 0.0);
    ui->labelCurrentVoltageCW->setText(QString("当前偏压：%1").arg(voltage, 0, 'f', 2));
    ui->lineEditVoltageCW->setText(QString::number(voltage, 'f', 2));
}

void MainWindow::updateCurrentVoltageCCW()
{
    int channel = ui->comboBoxChannelCCW->currentData().toInt();
    double voltage = this->channelVoltagesCCW.value(channel, 0.0);
    ui->labelCurrentVoltageCCW->setText(QString("当前偏压：%1").arg(voltage, 0, 'f', 2));
    ui->lineEditVoltageCCW->setText(QString::number(voltage, 'f', 2));
}

void MainWindow::onVoltageInputChangedCW(const QString &text)
{
    QLineEdit *lineEdit = ui->lineEditVoltageCW;
    if (lineEdit) {
        validateVoltageInput(lineEdit, text);
    }
}

void MainWindow::onVoltageInputChangedCCW(const QString &text)
{
    QLineEdit *lineEdit = ui->lineEditVoltageCCW;
    if (lineEdit) {
        validateVoltageInput(lineEdit, text);
    }
}

void MainWindow::sendControlDataCW()
{
    if (!serial->isOpen()) {
        QMessageBox::warning(this, tr("错误"), tr("串口未打开"));
        return;
    }
    int channel = ui->comboBoxChannelCW->currentData().toInt();
    double voltage = this->channelVoltagesCW.value(channel, 0.0);
    QByteArray data = packProtocolData(false, voltage, channel);
    data[0] = '2'; // 明确2开头
    serial->write(data);
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    ui->textEditHistory->append("[" + timestamp + "] 发送CW控制命令: " + QString::fromUtf8(data));
}

void MainWindow::sendControlDataCCW()
{
    if (!serial->isOpen()) {
        QMessageBox::warning(this, tr("错误"), tr("串口未打开"));
        return;
    }
    int channel = ui->comboBoxChannelCCW->currentData().toInt();
    double voltage = this->channelVoltagesCCW.value(channel, 0.0);
    QByteArray data = packProtocolData(false, voltage, channel);
    data[0] = '3'; // 明确3开头
    serial->write(data);
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    ui->textEditHistory->append("[" + timestamp + "] 发送CCW控制命令: " + QString::fromUtf8(data));
}

void MainWindow::onVoltageButtonClickedCCW()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;

    int channel = ui->comboBoxChannelCCW->currentData().toInt();
    double voltage = this->channelVoltagesCCW.value(channel, 0.0);

    if (button == ui->buttonVoltageMinus0_CCW) {
        voltage -= 1.0;
    } else if (button == ui->buttonVoltageMinus1_CCW) {
        voltage -= 0.1;
    } else if (button == ui->buttonVoltageMinus2_CCW) {
        voltage -= 0.01;
    } else if (button == ui->buttonVoltagePlus2_CCW) {
        voltage += 0.01;
    } else if (button == ui->buttonVoltagePlus1_CCW) {
        voltage += 0.1;
    } else if (button == ui->buttonVoltagePlus0_CCW) {
        voltage += 1.0;
    }

    // 限制电压值范围
    if (voltage < -9.99) voltage = -9.99;
    if (voltage > 9.99) voltage = 9.99;

    this->channelVoltagesCCW[channel] = voltage;
    // 发送CCW协议（3开头）
    QByteArray data = packProtocolData(false, voltage, channel);
    data[0] = '3';
    if (serial->isOpen()) {
        serial->write(data);
        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
        ui->textEditHistory->append("[" + timestamp + "] 发送CCW控制命令: " + QString::fromUtf8(data));
    }
    updateCurrentVoltageCCW();
}