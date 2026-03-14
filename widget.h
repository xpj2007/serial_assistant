#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtCharts>
#include <QString>
#include <QPointF>
#include <QTimer>
#include <QMouseEvent>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_openBt_clicked();

    void on_closeBt_clicked();

    void serial_ready_read_slots();

    void on_TextSendRxBt_clicked();

    void on_TimeSendMode_checkStateChanged(const Qt::CheckState &arg1);

    void on_ClearTextTxBt_clicked();

    void on_ClearTextRxBt_clicked();

    void on_clearAllBt_clicked();

    void on_YmaxCount_valueChanged(double arg1);

    void on_YminCount_valueChanged(double arg1);

    void on_ClearChartBt_clicked();

    void on_ClearChartRxBt_clicked();

    void on_ClearChartChangeBt_clicked();

    void on_openBt_1_clicked();

    void on_closeBt_1_clicked();

    void on_clearAllBt_1_clicked();

    void on_refreshBt_1_clicked();

    void on_refreshBt_clicked();

private:
    Ui::Widget *ui;

    QSerialPort *serialport;
    QByteArray RX_buf=0;
    QString Rx_str="[接收]";
    int rx_count=0;
    QByteArray datahead=0;
    QByteArray datatail=0;
    int dataFormLen=4;
    int timer_1_id;
    int tx_count=0;
    int Tx_time_flag=0;
    QChart *chart;                 //图表
    QLineSeries *Series;           //数据显示方式：折线
    QValueAxis *axisX;             //X轴
    QValueAxis *axisY;             //Y轴
    QVector<QPointF> datasPoints;  //储存坐标数据（X,Y）
    int chart_flag=0;
    int chart_count=0;
    QLineSeries *cursorX;          //游标X
    QLineSeries *cursorY;          //游标Y
    double  cur_cursor_X=0;        //当前游标x
    double  cur_cursor_Y=0;        //当前游标y
    quint8  get_cursor_flag=0;     //采集步骤标志
    double  first_cursor_X=0;      //第一次采样x
    double  first_cursor_Y=0;      //第一次采样y
    double  next_cursor_X=0;       //第二次采样x
    double  next_cursor_Y=0;       //第二次采样y
private:
    void Serial_Scan();
    void Serial_Set();
    void Serial_ProSet();
    void DataProc_common();
    void DataProc_format();
    bool data_check(QByteArray frame);
    void timerEvent(QTimerEvent *event)override;
    void Tx_data_Once();
    void Tx_data_time();
    quint8 make8_be(QByteArray frame);
    quint16 make16_be(QByteArray frame);
    quint32 make32_be(QByteArray frame);
    int makeint_be(QByteArray frame);
    float makefloat_be(QByteArray frame);
    double makedouble_be(QByteArray frame);
    void Chart_Init();
    void Update_Chart(double valid_data);
    void Chart_Mode(QByteArray frame);
    void Cursor_Move(QObject *watched, QEvent *event);
    void get_change();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

};
#endif // WIDGET_H
