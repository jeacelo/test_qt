#include "guipanel.h"
#include "ui_guipanel.h"
#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie
#include <QMessageBox>      // Se deben incluir cabeceras a los componentes que se vayan a crear en la clase

#include <QJsonObject>
#include <QJsonDocument>

// y que no estén incluidos en el interfaz gráfico. En este caso, la ventana de PopUp <QMessageBox>
// que se muestra al recibir un PING de respuesta

#include<stdint.h>      // Cabecera para usar tipos de enteros con tamaño
#include<stdbool.h>     // Cabecera para usar booleanos

#define SAMPLES 20

int i=0;
int j=0;

GUIPanel::GUIPanel(QWidget *parent) :  // Constructor de la clase
    QWidget(parent),
    ui(new Ui::GUIPanel)               // Indica que guipanel.ui es el interfaz grafico de la clase
  , transactionCount(0)
{
    ui->setupUi(this);                // Conecta la clase con su interfaz gráfico.
    setWindowTitle(tr("Interfaz de Control")); // Título de la ventana

    _client=new QMQTT::Client("localhost", 1883); //localhost y lo otro son valores por defecto


    connect(_client, SIGNAL(connected()), this, SLOT(onMQTT_Connected()));
    connect(_client, SIGNAL(connacked(quint8)), this, SLOT(onMQTT_Connacked(quint8)));
    //connect(_client, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onMQTT_error(QAbstractSocket::SocketError)));
    //connect(_client, SIGNAL(published(QMQTT::Message &)), this, SLOT(onMQTT_Published(QMQTT::Message &)));
    //connect(_client, SIGNAL(pubacked(quint8, quint16)), this, SLOT(onMQTT_Pubacked(quint8, quint16)));
    connect(_client, SIGNAL(received(const QMQTT::Message &)), this, SLOT(onMQTT_Received(const QMQTT::Message &)));
    connect(_client, SIGNAL(subscribed(const QString &)), this, SLOT(onMQTT_subscribed(const QString &)));
    connect(_client, SIGNAL(subacked(quint16, quint8)), this, SLOT(onMQTT_subacked(quint16, quint8)));
    //connect(_client, SIGNAL(unsubscribed(const QString &)), this, SLOT(onMQTT_unsubscribed(const QString &)));
    //connect(_client, SIGNAL(unsubacked(quint16)), this, SLOT(onMQTT_unsubacked(quint16)));
    //connect(_client, SIGNAL(pong()), this, SLOT(onMQTT_Pong()));
    //connect(_client, SIGNAL(disconnected()), this, SLOT(onMQTT_disconnected()));

    connected=false;                 // Todavía no hemos establecido la conexión USB

    ui->tempPlot->setTitle("Temperatura"); // Titulo de la grafica
    ui->tempPlot->setAxisScale(QwtPlot::yLeft, -10, 50); // Escala fija
    ui->tempPlot->setAxisScale(QwtPlot::xBottom,0,SAMPLES-1);
    ui->tempPlot->setAxisTitle(QwtPlot::yLeft, "ºC");
    ui->tempPlot->setAxisTitle(QwtPlot::xBottom, "Muestras");

    curve_temp = new QwtPlotCurve();
    curve_temp->attach(ui->tempPlot);

    for(int i = 0; i < SAMPLES; i++)
    {
        xValDig_temp[i]=i;
        yValDig_temp[i]=0;
    }

    curve_temp->setRawSamples(xValDig_temp,yValDig_temp,SAMPLES);

    curve_temp->setPen(QPen(Qt::red));

    m_GridDig = new QwtPlotGrid();     // Rejilla de puntos
    m_GridDig->attach(ui->tempPlot);    // que se asocia al objeto QwtPl

    ui->tempPlot->setAutoReplot(false); //Desactiva el autoreplot (mejora la eficiencia)

    ui->accPlot->setTitle("Aceleracion"); // Titulo de la grafica
    ui->accPlot->setAxisScale(QwtPlot::yLeft, -100, 100); // Escala fija
    ui->accPlot->setAxisScale(QwtPlot::xBottom,0,SAMPLES-1);
    ui->accPlot->setAxisTitle(QwtPlot::yLeft, "g");
    ui->accPlot->setAxisTitle(QwtPlot::xBottom, "Muestras");

    curve_accx = new QwtPlotCurve();
    curve_accy = new QwtPlotCurve();
    curve_accz = new QwtPlotCurve();
    curve_accx->attach(ui->accPlot);
    curve_accy->attach(ui->accPlot);
    curve_accz->attach(ui->accPlot);

    for(int i = 0; i < SAMPLES; i++)
    {
        xValDig_acc[i]=i;
        yValDig_acc_x[i]=0;
        yValDig_acc_y[i]=0;
        yValDig_acc_z[i]=0;
    }

    curve_accx->setRawSamples(xValDig_acc,yValDig_acc_x,SAMPLES);
    curve_accy->setRawSamples(xValDig_acc,yValDig_acc_y,SAMPLES);
    curve_accz->setRawSamples(xValDig_acc,yValDig_acc_z,SAMPLES);

    curve_accx->setPen(QPen(Qt::red));
    curve_accy->setPen(QPen(Qt::green));
    curve_accz->setPen(QPen(Qt::blue));

    m_GridDig_2 = new QwtPlotGrid();     // Rejilla de puntos
    m_GridDig_2->attach(ui->accPlot);    // que se asocia al objeto QwtPl

    ui->accPlot->setAutoReplot(false); //Desactiva el autoreplot (mejora la eficiencia)
}

GUIPanel::~GUIPanel() // Destructor de la clase
{
    delete ui;   // Borra el interfaz gráfico asociado a la clase
}


// Establecimiento de la comunicación USB serie a través del interfaz seleccionado en la comboBox, tras pulsar el
// botón RUN del interfaz gráfico. Se establece una comunicacion a 9600bps 8N1 y sin control de flujo en el objeto
// 'serial' que es el que gestiona la comunicación USB serie en el interfaz QT
void GUIPanel::startClient()
{
    _client->setHost(ui->leHost->text());
    _client->setPort(1883);
    _client->setKeepAlive(300);
    _client->setCleansess(true);

    _client->connect();
}

// Funcion auxiliar de procesamiento de errores de comunicación (usada por startSlave)
void GUIPanel::processError(const QString &s)
{
    activateRunButton(); // Activa el botón RUN
    // Muestra en la etiqueta de estado la razón del error (notese la forma de pasar argumentos a la cadena de texto)
    ui->statusLabel->setText(tr("Status: Not running, %1.").arg(s));
}

// Funcion de habilitacion del boton de inicio/conexion
void GUIPanel::activateRunButton()
{
    ui->runButton->setEnabled(true);
}

// SLOT asociada a pulsación del botón RUN
void GUIPanel::on_runButton_clicked()
{
    startClient();
}


void GUIPanel::onMQTT_subscribed(const QString &topic)
{
     ui->statusLabel->setText(tr("subscribed %1").arg(topic));
}

void GUIPanel::onMQTT_subacked(quint16 msgid, quint8 qos)
{
     ui->statusLabel->setText(tr("subacked: msgid=%1, qos=%2").arg(msgid).arg(qos));
}

void GUIPanel::on_pushButton_clicked()
{
    ui->statusLabel->setText(tr(""));
}


void GUIPanel::onMQTT_Received(const QMQTT::Message &message)
{
    if (connected)
    {
        if (message.topic() == "/cc3200/Temp")
        {
            //Deshacemos el escalado
            QJsonParseError error;
            QJsonDocument mensaje=QJsonDocument::fromJson(message.payload(),&error);

            if ((error.error==QJsonParseError::NoError)&&(mensaje.isObject()))
            { //Tengo que comprobar que el mensaje es del tipo adecuado y no hay errores de parseo...

                QJsonObject objeto_json=mensaje.object();
                QJsonValue entrada=objeto_json["Temperature"]; //Obtengo la entrada led. Esto lo puedo hacer porque el operador [] está sobreescrito
                ui->thermometer->setValue(entrada.toDouble());
                if (i>19)
                {
                    for (int k = 0; k < 19; k++)
                    {
                        yValDig_temp[k]=yValDig_temp[k+1];
                    }
                    yValDig_temp[i-1] = entrada.toDouble();
                }
                else
                {
                    yValDig_temp[i] = entrada.toDouble();
                    i++;
                }
                ui->tempPlot->replot(); //Refresca la grafica una vez actualizados los valores
            }
        }
        if (message.topic() == "/cc3200/Acc")
        {
            //Deshacemos el escalado
            QJsonParseError error;
            QJsonDocument mensaje=QJsonDocument::fromJson(message.payload(),&error);

            if ((error.error==QJsonParseError::NoError)&&(mensaje.isObject()))
            { //Tengo que comprobar que el mensaje es del tipo adecuado y no hay errores de parseo...

                QJsonObject objeto_json=mensaje.object();
                QJsonValue entrada_X=objeto_json["AccX"]; //Obtengo la entrada led. Esto lo puedo hacer porque el operador [] está sobreescrito
                QJsonValue entrada_Y=objeto_json["AccY"]; //Obtengo la entrada led. Esto lo puedo hacer porque el operador [] está sobreescrito
                QJsonValue entrada_Z=objeto_json["AccZ"]; //Obtengo la entrada led. Esto lo puedo hacer porque el operador [] está sobreescrito
                ui->accX->display(entrada_X.toDouble());
                ui->accY->display(entrada_Y.toDouble());
                ui->accZ->display(entrada_Z.toDouble());
                if (j>19)
                {
                    for (int k = 0; k < 19; k++)
                    {
                        yValDig_acc_x[k]=yValDig_acc_x[k+1];
                        yValDig_acc_y[k]=yValDig_acc_y[k+1];
                        yValDig_acc_z[k]=yValDig_acc_z[k+1];
                    }
                    yValDig_acc_x[j-1] = entrada_X.toDouble();
                    yValDig_acc_y[j-1] = entrada_Y.toDouble();
                    yValDig_acc_z[j-1] = entrada_Z.toDouble();
                }
                else
                {
                    yValDig_acc_x[j] = entrada_X.toDouble();
                    yValDig_acc_y[j] = entrada_Y.toDouble();
                    yValDig_acc_z[j] = entrada_Z.toDouble();
                    j++;
                }
                ui->accPlot->replot(); //Refresca la grafica una vez actualizados los valores
            }
        }
    }
}


/* -----------------------------------------------------------
 MQTT Client Slots
 -----------------------------------------------------------*/
void GUIPanel::onMQTT_Connected()
{
    QString topic("/cc3200/");
    topic.append(ui->topic->text());
    QString topic_temp("/cc3200/Temp");
    QString topic_acc("/cc3200/Acc");
    ui->runButton->setEnabled(false);

    // Se indica que se ha realizado la conexión en la etiqueta 'statusLabel'
    ui->statusLabel->setText(tr("Ejecucion, conectado al servidor"));

    connected=true;

    _client->subscribe(topic,0); //Se suscribe al mismo topic en el que publica...//MOD
    _client->subscribe(topic_temp,0);
    _client->subscribe(topic_acc,0);
}

void GUIPanel::onMQTT_Connacked(quint8 ack)
{
    QString ackStatus;
    switch(ack) {
    case QMQTT::CONNACK_ACCEPT:
        ackStatus = "Connection Accepted";
        break;
    case QMQTT::CONNACK_PROTO_VER:
        ackStatus = "Connection Refused: unacceptable protocol version";
        break;
    case QMQTT::CONNACK_INVALID_ID:
        ackStatus = "Connection Refused: identifier rejected";
        break;
    case QMQTT::CONNACK_SERVER:
        ackStatus = "Connection Refused: server unavailable";
        break;
    case QMQTT::CONNACK_CREDENTIALS:
        ackStatus = "Connection Refused: bad user name or password";
        break;
    case QMQTT::CONNACK_AUTH:
        ackStatus = "Connection Refused: not authorized";
        break;
    }

    ui->statusLabel->setText(tr("connacked: %1, %2").arg(ack).arg(ackStatus));
}

void GUIPanel::on_pushButton_3_clicked()
{
    QString topic ("/cc3200/");

    QJsonObject objeto_json
    {
        {ui->message->text(), 1}
    };
    QJsonDocument mensaje(objeto_json); //crea un objeto de tivo QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)

    topic.append(ui->topic->text());
    QMQTT::Message msg(0, topic, mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON//MOD

    //Publica el mensaje
    _client->publish(msg);
}

/*void GUIPanel::on_greenKnob_valueChanged(double value)
{
    ui->green_lcdNumber->setValue(value);
}

void GUIPanel::on_green_lcdNumber_valueChanged(int arg1)
{
    ui->greenKnob->setValue(arg1);
}

void GUIPanel::on_pushButton_4_clicked()
{
    QString topic ("/cc3200/");

    QJsonObject objeto_json;
    objeto_json["r"]=ui->redKnob->value();
    objeto_json["y"]=ui->yellowKnob->value();
    objeto_json["g"]=ui->greenKnob->value();

    QJsonDocument mensaje(objeto_json); //crea un objeto de tivo QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)

    topic.append(ui->topic->text());

    QMQTT::Message msg(0, topic, mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON//MOD

    //Publica el mensaje
    _client->publish(msg);
}*/

void GUIPanel::on_redCheck_toggled(bool checked)
{
    QByteArray cadena;
    QString topic ("/cc3200/");
    topic.append(ui->topic->text());

    QJsonObject objeto_json;
    objeto_json["redLed"]=checked;
    QJsonDocument mensaje(objeto_json); //crea un objeto de t-ivo QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)
    QMQTT::Message msg(0, topic, mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON//MOD

    //Publica el mensaje
    _client->publish(msg);
}

void GUIPanel::on_orangeCheck_toggled(bool checked)
{
    QByteArray cadena;
    QString topic ("/cc3200/");
    topic.append(ui->topic->text());

    QJsonObject objeto_json;
    objeto_json["orangeLed"]=checked;
    QJsonDocument mensaje(objeto_json); //crea un objeto de t-ivo QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)
    QMQTT::Message msg(0, topic, mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON//MOD

    //Publica el mensaje
    _client->publish(msg);
}

void GUIPanel::on_greenCheck_toggled(bool checked)
{
    QByteArray cadena;
    QString topic ("/cc3200/");
    topic.append(ui->topic->text());

    QJsonObject objeto_json;
    objeto_json["greenLed"]=checked;
    QJsonDocument mensaje(objeto_json); //crea un objeto de t-ivo QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)
    QMQTT::Message msg(0, topic, mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON//MOD

    //Publica el mensaje
    _client->publish(msg);
}

void GUIPanel::on_pushButton_4_released()
{
    QByteArray cadena;
    QString topic ("/cc3200/");
    topic.append(ui->topic->text());

    QJsonObject objeto_json;
    objeto_json["LED"]=ui->ledKnob->value();
    objeto_json["R"]=ui->colorWheel->color().red();
    objeto_json["G"]=ui->colorWheel->color().green();
    objeto_json["B"]=ui->colorWheel->color().blue();
    QJsonDocument mensaje(objeto_json); //crea un objeto de t-ivo QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)
    QMQTT::Message msg(0, topic, mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON//MOD

    //Publica el mensaje
    _client->publish(msg);
}

void GUIPanel::on_pushButton_5_released()
{
    for (int i = 0; i < ui->ledKnob->maximum()+1; i++)
    {
        QByteArray cadena;
        QString topic ("/cc3200/");
        topic.append(ui->topic->text());

        QJsonObject objeto_json;
        objeto_json["LED"]=i;
        objeto_json["R"]=0;
        objeto_json["G"]=0;
        objeto_json["B"]=0;
        QJsonDocument mensaje(objeto_json); //crea un objeto de t-ivo QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)
        QMQTT::Message msg(0, topic, mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON//MOD

        //Publica el mensaje
        _client->publish(msg);
    }
}

void GUIPanel::on_run_temp_released()
{
    QByteArray cadena;
    QString topic ("/cc3200/Sensors");

    QJsonObject objeto_json;
    objeto_json["TEMP"]=ui->ref_temp->value();
    objeto_json["ACC"]=0;

    QJsonDocument mensaje(objeto_json); //crea un objeto de t-ivo QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)
    QMQTT::Message msg(0, topic, mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON//MOD

    //Publica el mensaje
    _client->publish(msg);
}

void GUIPanel::on_run_acc_released()
{
    QByteArray cadena;
    QString topic ("/cc3200/Sensors");

    QJsonObject objeto_json;
    objeto_json["ACC"]=ui->ref_acc->value();
    objeto_json["TEMP"]=0;

    QJsonDocument mensaje(objeto_json); //crea un objeto de t-ivo QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)
    QMQTT::Message msg(0, topic, mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON//MOD

    //Publica el mensaje
    _client->publish(msg);
}
