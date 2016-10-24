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
    bool previousblockinstate,checked;
    if (connected)
    {
        //Deshacemos el escalado
        previousblockinstate=ui->pushButton_2->blockSignals(true);   //Esto es para evitar que el cambio de valor
                                                             //provoque otro envio al topic por el que he recibido

        QJsonParseError error;
        QJsonDocument mensaje=QJsonDocument::fromJson(message.payload(),&error);

        if ((error.error==QJsonParseError::NoError)&&(mensaje.isObject()))
        { //Tengo que comprobar que el mensaje es del tipo adecuado y no hay errores de parseo...

            QJsonObject objeto_json=mensaje.object();
            QJsonValue entrada=objeto_json["led"]; //Obtengo la entrada led. Esto lo puedo hacer porque el operador [] está sobreescrito


            if (entrada.isBool())
            {   //Compruebo que es booleano...

                checked=entrada.toBool(); //Leo el valor de objeto (si fuese entero usaria toInt(), toDouble() si es doble....

                ui->pushButton_2->setChecked(checked);
                if (checked)
                {
                    ui->pushButton_2->setText("Apaga");

                }
                else
                {
                    ui->pushButton_2->setText("Enciende");
                }
            }
        }

        ui->pushButton_2->blockSignals(previousblockinstate);
    }
}


/* -----------------------------------------------------------
 MQTT Client Slots
 -----------------------------------------------------------*/
void GUIPanel::onMQTT_Connected()
{
    QString topic(ui->topic->text());

    ui->runButton->setEnabled(false);

    // Se indica que se ha realizado la conexión en la etiqueta 'statusLabel'
    ui->statusLabel->setText(tr("Ejecucion, conectado al servidor"));

    connected=true;

    _client->subscribe(topic,0); //Se suscribe al mismo topic en el que publica...//MOD
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

void GUIPanel::on_pushButton_2_toggled(bool checked)
{
    QByteArray cadena;


    QJsonObject objeto_json;
    objeto_json["led"]=checked; //Añade un campo "led" al objeto JSON, con el valor (true o false) contenido en checked
                                //Puedo hacer ["led"] porque el operador [] está sobreescrito.
    QJsonDocument mensaje(objeto_json); //crea un objeto de tivo QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)

    QMQTT::Message msg(0, ui->topic->text(), mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON//MOD

    //Publica el mensaje
    _client->publish(msg);

    if (checked)
    {
        ui->pushButton_2->setText("Apaga");

    }
    else
    {
        ui->pushButton_2->setText("Enciende");
    }
}
