#ifndef GUIPANEL_H
#define GUIPANEL_H

#include <QWidget>
#include <QtSerialPort/qserialport.h>
#include "qmqtt.h"
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>

namespace Ui {
class GUIPanel;
}

//QT4:QT_USE_NAMESPACE_SERIALPORT

class GUIPanel : public QWidget
{
    Q_OBJECT
    
public:
    //GUIPanel(QWidget *parent = 0);
    explicit GUIPanel(QWidget *parent = 0);
    ~GUIPanel(); // Da problemas
    
private slots:

    void on_runButton_clicked();
    void on_pushButton_clicked();

    void onMQTT_Received(const QMQTT::Message &message);
    void onMQTT_Connected(void);
    void onMQTT_Connacked(quint8 ack);
    void onMQTT_subscribed(const QString &topic);
    void onMQTT_subacked(quint16 msgid, quint8 qos);

    void on_pushButton_3_clicked();

    void on_pushButton_4_released();

    void on_pushButton_5_released();

    void on_run_temp_released();

    void on_run_acc_released();

private: // funciones privadas
//    void pingDevice();
    void startClient();
    void processError(const QString &s);
    void activateRunButton();
    void cambiaLEDs();
private:
    Ui::GUIPanel *ui;
    int transactionCount;
    QMQTT::Client *_client;
    bool connected;

    double xValDig_temp[20]; //valores eje X
    double yValDig_temp[20]; //valores ejes Y
    double xValDig_acc[20]; //valores eje X
    double yValDig_acc_x[20]; //valores ejes Y
    double yValDig_acc_y[20]; //valores ejes Y
    double yValDig_acc_z[20]; //valores ejes Y

    QwtPlotCurve *curve_temp; //Curvas
    QwtPlotCurve *curve_accx; //Curvas
    QwtPlotCurve *curve_accy; //Curvas
    QwtPlotCurve *curve_accz; //Curvas

    QwtPlotGrid  *m_GridDig; //Cuadricula
    QwtPlotGrid  *m_GridDig_2; //Cuadricula
};

#endif // GUIPANEL_H
