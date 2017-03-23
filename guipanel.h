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

    void on_redCheck_toggled(bool checked);

    void on_orangeCheck_toggled(bool checked);

    void on_greenCheck_toggled(bool checked);

    void on_pushButton_4_released();

    void on_pushButton_5_released();

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

    double xValDig[20]; //valores eje X
    double yValDig[20]; //valores ejes Y
    QwtPlotCurve *ChannelsDig; //Curvas
    QwtPlotGrid  *m_GridDig; //Cuadricula
};

#endif // GUIPANEL_H
