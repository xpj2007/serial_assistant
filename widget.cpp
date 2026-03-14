#include "widget.h"
#include "./ui_widget.h"

#include <QMessageBox>
#include<qdebug.h>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    timer_1_id=this->startTimer(1);

    ui->waveView->viewport()->installEventFilter(this);
    serialport =new QSerialPort;
    connect(serialport,&QSerialPort::readyRead,this,&Widget::serial_ready_read_slots);
    Serial_Scan();
    Chart_Init();

}

Widget::~Widget()
{
    delete ui;
}

//可用串口扫描
void Widget::Serial_Scan()
{
    ui->serialCb->clear();
    QStringList serialPortNum;
    foreach (const QSerialPortInfo &Info, QSerialPortInfo::availablePorts()) {
        serialPortNum << Info.portName();
    }
    ui->serialCb->addItems(serialPortNum);
}
//串口基础设置
void Widget::Serial_Set()
{
    QSerialPort::BaudRate baudrate=QSerialPort::Baud115200;
    QSerialPort::DataBits databits=QSerialPort::Data8;
    QSerialPort::StopBits stopbits=QSerialPort::OneStop;
    QSerialPort::FlowControl flowcontrol=QSerialPort::NoFlowControl;
    QSerialPort::Parity parity=QSerialPort::NoParity;
    //波特率
    if(ui->baudrateCb->currentText()=="115200"){
        baudrate=QSerialPort::Baud115200;
    }else if(ui->baudrateCb->currentText()=="9600"){
        baudrate=QSerialPort::Baud9600;
    }else if(ui->baudrateCb->currentText()=="19200"){
        baudrate=QSerialPort::Baud19200;
    }else if(ui->baudrateCb->currentText()=="38400"){
        baudrate=QSerialPort::Baud38400;
    }else if(ui->baudrateCb->currentText()=="57600"){
        baudrate=QSerialPort::Baud57600;
    }
    //数据位
    if(ui->databitCb->currentText()=="8"){
        databits=QSerialPort::Data8;
    }else if(ui->databitCb->currentText()=="7"){
        databits=QSerialPort::Data7;
    }else if(ui->databitCb->currentText()=="6"){
        databits=QSerialPort::Data6;
    }else if(ui->databitCb->currentText()=="5"){
        databits=QSerialPort::Data5;
    }
    //校验位
    if(ui->checkbitCb->currentText()=="None"){
        parity=QSerialPort::NoParity;
    }else if(ui->checkbitCb->currentText()=="Even"){
        parity=QSerialPort::EvenParity;
    }else if(ui->checkbitCb->currentText()=="Odd"){
        parity=QSerialPort::OddParity;
    }else if(ui->checkbitCb->currentText()=="Mark"){
        parity=QSerialPort::MarkParity;
    }else if(ui->checkbitCb->currentText()=="Space"){
        parity=QSerialPort::SpaceParity;
    }
    //停止位
    if(ui->stopbitCb->currentText()=="1"){
        stopbits=QSerialPort::OneStop;
    }else if(ui->stopbitCb->currentText()=="2"){
        stopbits=QSerialPort::TwoStop;
    }else if(ui->stopbitCb->currentText()=="1.5"){
        stopbits=QSerialPort::OneAndHalfStop;
    }
    //流控制
    if(ui->flowctrCb->currentText()=="无"){
        flowcontrol=QSerialPort::NoFlowControl;
    }else if(ui->flowctrCb->currentText()=="硬件"){
        flowcontrol=QSerialPort::HardwareControl;
    }else if(ui->flowctrCb->currentText()=="软件"){
        flowcontrol=QSerialPort::SoftwareControl;
    }
    //串口设置
    serialport->setPortName(ui->serialCb->currentText());
    serialport->setBaudRate(baudrate);
    serialport->setDataBits(databits);
    serialport->setStopBits(stopbits);
    serialport->setParity(parity);
    serialport->setFlowControl(flowcontrol);
}
//串口进阶设置
void Widget::Serial_ProSet()
{
    datahead=QByteArray::fromHex(ui->dataHeadEdit->text().remove(" ").toUtf8());
    datatail=QByteArray::fromHex(ui->dataTailEdit->text().remove(" ").toUtf8());
    dataFormLen=datahead.size()+datatail.size()+2+ui->dataCount->value();

}
//打开串口
void Widget::on_openBt_clicked()
{
    Serial_Set();
    Serial_ProSet();
    if(serialport->open(QIODevice::ReadWrite)==true){
        ui->DataFrameMode->setEnabled(false);
        ui->TextRxEdit->appendPlainText("[串口打开]");
    }else{
        QMessageBox::critical(this,"提示","串口打开失败");
    }
}
//关闭串口
void Widget::on_closeBt_clicked()
{
    serialport->close();
    ui->DataFrameMode->setEnabled(true);
    ui->TextRxEdit->appendPlainText("[串口关闭]");
}
//数据处理普通模式
void Widget::DataProc_common()
{
    RX_buf=serialport->readAll();
    if(ui->RxformCb->currentText()=="ASCII"){
        Rx_str+=QString::fromLatin1(RX_buf);
    }else if(ui->RxformCb->currentText()=="HEX"){
        Rx_str+=RX_buf.toHex(' ').toUpper();
    }else if(ui->RxformCb->currentText()=="UTF-8"){
        Rx_str+=QString::fromUtf8(RX_buf);
    }else if(ui->RxformCb->currentText()=="GBK"){
        Rx_str+=QString::fromLocal8Bit(RX_buf);
    }
    rx_count=0;
}
//数据处理数据帧模式
void Widget::DataProc_format()
{
    RX_buf.append(serialport->readAll());
    while(RX_buf.size()>=dataFormLen)
    {
        int pos=RX_buf.indexOf(datahead);
        if(pos==-1){RX_buf.remove(0,RX_buf.size()-datahead.size());break;}
        if(pos>0)RX_buf.remove(0,pos);
        if(dataFormLen>RX_buf.size())break;
        QByteArray frame=RX_buf.mid(0,dataFormLen);
        if(data_check(frame)==true){
            RX_buf.remove(0,dataFormLen);
            ui->TextRxEdit->appendPlainText("[接收]"+frame.toHex(' ').toUpper());
            Chart_Mode(frame);
        }else{
            RX_buf.remove(0,datahead.size()-1);
        }
    }
}
//数据帧检查
bool Widget::data_check(QByteArray frame)
{
    //查长度
    if(frame.size()!=dataFormLen)return false;
    if(frame[datahead.size()]!=ui->dataCount->value())return false;
    //查包头
    for(int i=0;i<datahead.size();i++){
        if(frame[i]!=datahead[i])return false;
    }
    //查包尾
    for(int i=0;i<datatail.size();i++){
        if(frame[dataFormLen+i-datatail.size()]!=datatail[i])return false;
    }
    //查校验位
    uint8_t check=0;
    for(int i=0;i<ui->dataCount->value();i++){
        if(ui->dataCheckCb->currentText()=="异或校验"){
            check^=frame[datahead.size()+1+i];
        }else{
            check+=frame[datahead.size()+1+i];
        }
    }
    int check_data_pos=static_cast<int>(ui->dataCount->value())+datahead.size()+1;
    if(static_cast<uint8_t>(frame[check_data_pos])!=check)return false;
    return true;
}
//数据接收
void Widget::serial_ready_read_slots()
{
    if(!serialport || !serialport->isOpen()){return;}
    if(ui->DataFrameMode->isChecked()){
        DataProc_format();
    }else{
        DataProc_common();
    }

}
//定时器
void Widget::timerEvent(QTimerEvent *event)
{
    if(Rx_str!="[接收]"){
        rx_count++;
        if(rx_count>=50){
            rx_count=0;
            ui->TextRxEdit->appendPlainText(Rx_str);
            Rx_str="[接收]";
            RX_buf=0;
        }
        if(Rx_str.size()>999){
            ui->TextRxEdit->appendPlainText(Rx_str);
            Rx_str="[接收]";
            RX_buf=0;
        }
    }
    if(Tx_time_flag==1){
        tx_count++;
        if(tx_count>=ui->IntervalTimeCount->value()){
            tx_count=0;
            Tx_data_time();
        }
    }

    chart_count++;
    if(chart_count>=30){
        chart_count=0;
        if(chart_flag==1){
            chart_flag=0;
            Series->replace(datasPoints);                                           //替换数据（更新图表）
            axisX->setRange(0,datasPoints.size()-1);                                //设置长度
        }
    }

}
//发送一次数据
void Widget::Tx_data_Once()
{
    if(serialport->isOpen()!=true)return;
    if(serialport->isWritable()!=true){ui->TextRxEdit->appendPlainText("[发送失败]");return;}
    QString Tx_buf=ui->TextTxEdit->toPlainText();
    if(Tx_buf.isEmpty()){ui->TextRxEdit->appendPlainText("[发送内容不能为空]");return;}
    //转换数据类型
    QByteArray Tx_data=Tx_buf.toLatin1();
    if(ui->TxformCb->currentText()=="ASCII"){
        Tx_data=Tx_buf.toLatin1();
    }else if(ui->TxformCb->currentText()=="HEX"){
        Tx_buf.remove(" ");
        Tx_data=QByteArray::fromHex(Tx_buf.toUtf8());
    }else if(ui->TxformCb->currentText()=="UTF-8"){
        Tx_data=Tx_buf.toUtf8();
    }else if(ui->TxformCb->currentText()=="GBK"){
        Tx_data=Tx_buf.toLocal8Bit();
    }
    //发送数据并显示
    serialport->write(Tx_data);
    if(ui->TxformCb->currentText()=="ASCII"){
        QString textStr="[发送]"+QString::fromLatin1(Tx_data);
        ui->TextRxEdit->appendPlainText(textStr);
    }else if(ui->TxformCb->currentText()=="HEX"){
        QString hexStr="[发送]"+Tx_data.toHex(' ').toUpper();
        ui->TextRxEdit->appendPlainText(hexStr);
    }else if(ui->TxformCb->currentText()=="UTF-8"){
        QString textStr="[发送]"+QString::fromUtf8(Tx_data);
        ui->TextRxEdit->appendPlainText(textStr);
    }else if(ui->TxformCb->currentText()=="GBK"){
        QString textStr="[发送]"+QString::fromLocal8Bit(Tx_data);
        ui->TextRxEdit->appendPlainText(textStr);
    }
}
//定时发送一次数据
void Widget::Tx_data_time()
{
    if(serialport->isOpen()!=true)return;
    if(serialport->isWritable()!=true){ui->TextRxEdit->appendPlainText("[发送失败]");return;}
    QString Tx_buf=ui->TextTxEdit->toPlainText();
    if(Tx_buf.isEmpty()){ui->TextRxEdit->appendPlainText("[发送内容不能为空]");return;}
    //转换数据类型
    QByteArray Tx_data=Tx_buf.toLatin1();
    if(ui->TxformCb->currentText()=="ASCII"){
        Tx_data=Tx_buf.toLatin1();
    }else if(ui->TxformCb->currentText()=="HEX"){
        Tx_buf.remove(" ");
        Tx_data=QByteArray::fromHex(Tx_buf.toUtf8());
    }else if(ui->TxformCb->currentText()=="UTF-8"){
        Tx_data=Tx_buf.toUtf8();
    }else if(ui->TxformCb->currentText()=="GBK"){
        Tx_data=Tx_buf.toLocal8Bit();
    }
    //发送数据并显示
    serialport->write(Tx_data);
    if(ui->TxformCb->currentText()=="ASCII"){
        QString textStr="[定时发送]"+QString::fromLatin1(Tx_data);
        ui->TextRxEdit->appendPlainText(textStr);
    }else if(ui->TxformCb->currentText()=="HEX"){
        QString hexStr="[定时发送]"+Tx_data.toHex(' ').toUpper();
        ui->TextRxEdit->appendPlainText(hexStr);
    }else if(ui->TxformCb->currentText()=="UTF-8"){
        QString textStr="[定时发送]"+QString::fromUtf8(Tx_data);
        ui->TextRxEdit->appendPlainText(textStr);
    }else if(ui->TxformCb->currentText()=="GBK"){
        QString textStr="[定时发送]"+QString::fromLocal8Bit(Tx_data);
        ui->TextRxEdit->appendPlainText(textStr);
    }
}
//发送
void Widget::on_TextSendRxBt_clicked()
{
    if(Tx_time_flag==0){
        Tx_data_Once();
    }
}
//定时发送勾选
void Widget::on_TimeSendMode_checkStateChanged(const Qt::CheckState &arg1)
{
    if(arg1==Qt::Checked){
        Tx_time_flag=1;
        tx_count=0;
    }else{
        Tx_time_flag=0;
    }
}
//清空接收区
void Widget::on_ClearTextTxBt_clicked()
{
    ui->TextRxEdit->clear();
}
//清空发送区
void Widget::on_ClearTextRxBt_clicked()
{
    ui->TextTxEdit->clear();
}
//清空
void Widget::on_clearAllBt_clicked()
{
    ui->TextRxEdit->clear();
    ui->TextTxEdit->clear();
    datasPoints.clear();
    Series->replace(datasPoints);
    ui->ChartRxEdit->clear();
    ui->ChartXYEdit->clear();
}
//有效数据变形
quint8 Widget::make8_be(QByteArray frame)
{
    return static_cast<uint8_t>(frame[datahead.size()+1]);
}
quint16 Widget::make16_be(QByteArray frame)
{
    return static_cast<uint8_t>(frame[datahead.size()+1])<<8|static_cast<uint8_t>(frame[datahead.size()+2]);
}
quint32 Widget::make32_be(QByteArray frame)
{
    return static_cast<uint8_t>(frame[datahead.size()+1])<<24|static_cast<uint8_t>(frame[datahead.size()+2])<<16|static_cast<uint8_t>(frame[datahead.size()+3])<<8|static_cast<uint8_t>(frame[datahead.size()+4]);
}
int Widget::makeint_be(QByteArray frame)
{
    union { int i; quint8 b[4]; } u;
    u.b[0] = static_cast<uint8_t>(frame[datahead.size()+1]);
    u.b[1] = static_cast<uint8_t>(frame[datahead.size()+2]);
    u.b[2] = static_cast<uint8_t>(frame[datahead.size()+3]);
    u.b[3] = static_cast<uint8_t>(frame[datahead.size()+4]);
    return u.i;
}
float Widget::makefloat_be(QByteArray frame)
{
    union { float f; quint8 b[4]; } u;
    u.b[0] = static_cast<uint8_t>(frame[datahead.size()+1]);
    u.b[1] = static_cast<uint8_t>(frame[datahead.size()+2]);
    u.b[2] = static_cast<uint8_t>(frame[datahead.size()+3]);
    u.b[3] = static_cast<uint8_t>(frame[datahead.size()+4]);
    return u.f;
}
double Widget::makedouble_be(QByteArray frame)
{
    union { double d; quint8 b[8]; } u;
    u.b[0] = static_cast<uint8_t>(frame[datahead.size()+1]);
    u.b[1] = static_cast<uint8_t>(frame[datahead.size()+2]);
    u.b[2] = static_cast<uint8_t>(frame[datahead.size()+3]);
    u.b[3] = static_cast<uint8_t>(frame[datahead.size()+4]);
    u.b[4] = static_cast<uint8_t>(frame[datahead.size()+5]);
    u.b[5] = static_cast<uint8_t>(frame[datahead.size()+6]);
    u.b[6] = static_cast<uint8_t>(frame[datahead.size()+7]);
    u.b[7] = static_cast<uint8_t>(frame[datahead.size()+8]);
    return u.d;
}
//初始化图表
void Widget::Chart_Init()
{
    //1.#实例化一个数据曲线类型对象并命名
    Series=new QLineSeries();
    Series->setName("ADC波形显示");
    //2.#实例化一个图表对象并配置
    chart= new QChart();
    chart->addSeries(Series);                           //添加数据曲线
    chart->setTitle("实时波形显示");                      //命名标题
    chart->legend()->setVisible(true);                  //图表曲线图例可见性
    chart->legend()->setAlignment(Qt::AlignBottom);     //图表曲线图例位置：底部
    //2.#实例化一个X轴对象并配置
    axisX=new QValueAxis();
    axisX->setTitleText("采样点");                       //命名X轴
    axisX->setRange(0,100);                             //X轴范围（0，0）
    chart->addAxis(axisX,Qt::AlignBottom);              //添加到图表底部
    Series->attachAxis(axisX);                          //数据曲线绑定X轴
    //3.#实例化一个Y轴对象并配置
    axisY=new QValueAxis();
    axisY->setTitleText("电压值");                       //命名Y轴
    axisY->setRange(-100,4300);                         //Y轴范围（-10,4100）
    chart->addAxis(axisY,Qt::AlignLeft);                //添加到图表左部
    Series->attachAxis(axisY);                          //数据曲线绑定Y轴
    //4.#实例化游标X对象并命名
    cursorX=new QLineSeries();
    cursorX->setPen(QPen(Qt::red,0.5));
    chart->addSeries(cursorX);                           //添加到图表
    cursorX->attachAxis(axisY);                          //绑定到Y轴
    cursorX->attachAxis(axisX);                          //绑定到X轴
    //5.#实例化游标Y对象并命名
    cursorY=new QLineSeries();
    cursorY->setPen(QPen(Qt::red,0.5));
    chart->addSeries(cursorY);                           //添加到图表
    cursorY->attachAxis(axisY);                          //绑定到Y轴
    cursorY->attachAxis(axisX);                          //绑定到X轴
    //4.将图表显示在ui并优化视觉效果
    ui->waveView->setChart(chart);                      //ui显示图表道waveView
    ui->waveView->setRenderHint(QPainter::Antialiasing);//优化：抗锯齿
}
//更新数据并更新图表
void Widget::Update_Chart(double valid_data)
{

    while (datasPoints.size()>=ui->XmaxCount->value()){                         //判断是否越界
        datasPoints.removeFirst();  //移除首项
        for(int i=0;i<datasPoints.size();i++){                              //每项前移
            datasPoints[i].setX(i);
        }
    }
    datasPoints.append(QPointF(datasPoints.size(),valid_data));             //添加最新项
    chart_flag=1;
}
//图表模式
void Widget::Chart_Mode(QByteArray frame)
{
    if(ui->ChartMode->isChecked()){
        double valid_data=0;
        if(ui->dataTypeCb->currentText()=="uint8_t"){
            valid_data=(double)make8_be(frame);
        }else if(ui->dataTypeCb->currentText()=="uint16_t"){
            valid_data=(double)make16_be(frame);
        }else if(ui->dataTypeCb->currentText()=="uint32_t"){
            valid_data=(double)make32_be(frame);
        }else if(ui->dataTypeCb->currentText()=="int"){
            valid_data=(double)makeint_be(frame);
        }else if(ui->dataTypeCb->currentText()=="float"){
            valid_data=(double)makefloat_be(frame);
        }else if(ui->dataTypeCb->currentText()=="double"){
            valid_data=makedouble_be(frame);
        }
        QString str=frame.toHex(' ').toUpper()+"  有效数据  "+QString::number(valid_data);
        ui->ChartRxEdit->appendPlainText(str);
        Update_Chart(valid_data);
    }
}
//改变Y轴最大值
void Widget::on_YmaxCount_valueChanged(double arg1)
{
    if(arg1-ui->YminCount->value()<1){ui->YmaxCount->setValue(ui->YminCount->value()+2);return;}
    axisY->setRange(ui->YminCount->value(),arg1);
}
//改变Y轴最小值
void Widget::on_YminCount_valueChanged(double arg1)
{
    if(ui->YmaxCount->value()-arg1<1){ui->YminCount->setValue(ui->YmaxCount->value()-2);return;}
    axisY->setRange(arg1,ui->YminCount->value());
}
//游标事件
void Widget::Cursor_Move(QObject *watched, QEvent *event)
{
    QMouseEvent *mouse=static_cast<QMouseEvent*>(event);
    //1.#获取坐标并转化位图表上的坐标
    QPointF mouse_pos=ui->waveView->chart()->mapToValue(mouse->pos(),Series);
    cur_cursor_X=mouse_pos.x();
    QList<QPointF> points=Series->points();
    //2.#计算对应曲线Y轴坐标数据
    for(int i=0;i<points.size()-2;i++)
    {
        if(cur_cursor_X >=points[i].x()  &&cur_cursor_X <= points[i+1].x())
        {
            cur_cursor_Y=points[i].y() + (points[i+1].y() - points[i].y())*(cur_cursor_X - points[i].x()) / ( points[i+1].x() -points[i].x());
            break;
        }
    }
    //3.#改变游标Y的位置
    cursorY->clear();
    cursorY->append(cur_cursor_X,axisY->max());
    cursorY->append(cur_cursor_X,axisY->min());
    //4.#改变游标X的位置
    cursorX->clear();
    cursorX->append(axisX->min(),cur_cursor_Y);
    cursorX->append(axisX->max(),cur_cursor_Y);
    //5.#显示当前数据坐标
    ui->ChartRealTimXYEdit->setText("("+QString::number(cur_cursor_X,'f',2)+" , "+QString::number(cur_cursor_Y,'f',2)+")");
}
//获得改变值
void Widget::get_change()
{
    if(get_cursor_flag==0)
    {
        get_cursor_flag=1;
        first_cursor_X=cur_cursor_X;
        first_cursor_Y=cur_cursor_Y;
        ui->ChartXYEdit->appendPlainText("【第一次采样】");
        ui->ChartXYEdit->appendPlainText("("+QString::number(first_cursor_X,'f',2)+" , "+QString::number(first_cursor_Y,'f',2)+")");
    }
    else
    {
        get_cursor_flag=0;
        next_cursor_X=cur_cursor_X;
        next_cursor_Y=cur_cursor_Y;
        ui->ChartXYEdit->appendPlainText("【第二次采样】");
        ui->ChartXYEdit->appendPlainText("("+QString::number(next_cursor_X,'f',2)+" , "+QString::number(next_cursor_Y,'f',2)+")");
        ui->ChartXYEdit->appendPlainText("【改变值】");
        ui->ChartXYEdit->appendPlainText("ChangeX:"+QString::number(next_cursor_X-first_cursor_X,'f',2));
        ui->ChartXYEdit->appendPlainText("ChangeY:"+QString::number(next_cursor_Y-first_cursor_Y,'f',2));
        ui->ChartXYEdit->appendPlainText(" ");
    }
}
//事件
bool Widget::eventFilter(QObject *watched, QEvent *event)
{
    //鼠标在waveView上移动事件
    if(watched==ui->waveView->viewport() && event->type()==QEvent::MouseMove)
    {
        Cursor_Move(watched, event);
    }
    //鼠标在waveView上点击事件
    if(watched==ui->waveView->viewport() && event->type()==QEvent::MouseButtonPress)
    {
        get_change();
    }
    return QWidget::eventFilter(watched,event);
}
//清空图表
void Widget::on_ClearChartBt_clicked()
{
    datasPoints.clear();
    Series->replace(datasPoints);
}
//清空图表数据区
void Widget::on_ClearChartRxBt_clicked()
{
    ui->ChartRxEdit->clear();
}
//清空图表计算区
void Widget::on_ClearChartChangeBt_clicked()
{
    ui->ChartXYEdit->clear();
}
//打开串口
void Widget::on_openBt_1_clicked()
{
    Serial_Set();
    Serial_ProSet();
    if(serialport->open(QIODevice::ReadWrite)==true){
        ui->DataFrameMode->setEnabled(false);
        ui->TextRxEdit->appendPlainText("[串口打开]");
    }else{
        QMessageBox::critical(this,"提示","串口打开失败");
    }
}
//关闭串口
void Widget::on_closeBt_1_clicked()
{
    serialport->close();
    ui->DataFrameMode->setEnabled(true);
    ui->TextRxEdit->appendPlainText("[串口关闭]");
}
//清空
void Widget::on_clearAllBt_1_clicked()
{
    ui->TextRxEdit->clear();
    ui->TextTxEdit->clear();
    datasPoints.clear();
    Series->replace(datasPoints);
    ui->ChartRxEdit->clear();
    ui->ChartXYEdit->clear();
}
//刷新
void Widget::on_refreshBt_1_clicked()
{
    serialport->close();
    ui->TextRxEdit->clear();
    ui->TextTxEdit->clear();
    datasPoints.clear();
    Series->replace(datasPoints);
    ui->ChartRxEdit->clear();
    ui->ChartXYEdit->clear();
    ui->ChartMode->setCheckState(Qt::Unchecked);
    ui->DataFrameMode->setCheckState(Qt::Unchecked);
    ui->TimeSendMode->setCheckState(Qt::Unchecked);
    RX_buf=0;
    Rx_str="[接收]";
    tx_count=0;
    Tx_time_flag=0;
    chart_flag=0;
    chart_count=0;
    get_cursor_flag=0;
    Serial_Scan();
}
//刷新
void Widget::on_refreshBt_clicked()
{
    serialport->close();
    ui->TextRxEdit->clear();
    ui->TextTxEdit->clear();
    datasPoints.clear();
    Series->replace(datasPoints);
    ui->ChartRxEdit->clear();
    ui->ChartXYEdit->clear();
    ui->ChartMode->setCheckState(Qt::Unchecked);
    ui->DataFrameMode->setCheckState(Qt::Unchecked);
    ui->TimeSendMode->setCheckState(Qt::Unchecked);
    RX_buf=0;
    Rx_str="[接收]";
    tx_count=0;
    Tx_time_flag=0;
    chart_flag=0;
    chart_count=0;
    get_cursor_flag=0;
    Serial_Scan();
}

