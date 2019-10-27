#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QScrollBar>
#include <QMessageBox>

#include <QTimer>
#include <QThread>

class Sleeper : public QThread
{
public:
    static void usleep(unsigned long usecs){QThread::usleep(usecs);}
    static void msleep(unsigned long msecs){QThread::msleep(msecs);}
    static void sleep(unsigned long secs){QThread::sleep(secs);}
};


// globalne zmienne
QList <QSerialPortInfo> available_port;     // lista portów pod którymi są urządzenia
const QSerialPortInfo *info;                // obecnie wybrany serial port

QSerialPort port;   // obecnie otwarty port
QString plng; //jezyk programowania


// funkcje
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    info = NULL;

    searchDevices();

    connect(ui->action_refreshPorts,SIGNAL(triggered()),this,SLOT(refresh()));

    QTimer *timer = new QTimer(); // obiekt klasy QTimer
    timer->setInterval(500); // ustawiam interwał czasowy na 500 milisekund
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(timer_receive()));
    timer->start(); // uruchamiam zegar

}

MainWindow::~MainWindow()
{
    if(port.isOpen())
        port.close();

    delete ui;
}


// aktualizowanie listy z urządzeniami
void MainWindow::searchDevices()
{
    // dodanie ich do listy
    available_port = QSerialPortInfo::availablePorts();

    int porty = available_port.size();
    QString message = QString::number(porty) + (porty == 1 ? " port is ready to use" : " ports are ready to use");

    // wyświetlamy wiadomość z informacją ile znaleźliwśmy urządzeń gotowych do pracy
    ui->statusBar->showMessage(message,3000);

    // aktualizujemy
    info = NULL;
    ui->port->clear();
    ui->port->addItem("NULL");
    for(int i=0;i<porty;i++)
    {
        ui->port->addItem(available_port.at(i).portName());
    }
}

void MainWindow::on_port_currentIndexChanged(int index)
{
    if(port.isOpen()) port.close();
    QString txt = "NULL";
    if (index > 0){
        info = &available_port.at(index-1);
        txt = info->portName();

        port.setPort(available_port.at(index-1));
        port.setBaudRate(port.Baud115200);
        port.setDataBits(port.Data8);
        port.setStopBits(port.OneStop);
        port.setParity(port.NoParity);

        if(!port.open(QIODevice::ReadWrite))
            QMessageBox::warning(this,"Device error","Unable to open port.");
    }
    else
        info = NULL;

    ui->statusBar->showMessage("Selected port: " + txt,2000);
}


void MainWindow::refresh()
{
    searchDevices();
}


void MainWindow::on_commandLine_returnPressed()
{
    /*
    // jeżeli nie wybrano żadnego urządzenia nie wysyłamy
    if(info == NULL) {
        ui->statusBar->showMessage("Not port selected",3000);
        return;
    }

    //addTextToConsole(ui->commandLine->text(),true);
    send(ui->commandLine->text()+"\n");
    ui->commandLine->clear();
    */
}


//klik w wyslanie polecenia
void MainWindow::on_enterButton_clicked()
{
    // jeżeli nie wybrano żadnego urządzenia nie wysyłamy
    if(info == NULL) {
        ui->statusBar->showMessage("Not port selected",3000);
        return;
    }

    //addTextToConsole(ui->commandLine->text(),true);
    send(ui->commandLine->text()+"\n\r"); //w upython \n\r
    ui->commandLine->clear();
}


void MainWindow::addTextToConsole(QString msg,bool sender)
{
    if(msg.isEmpty()) return;

    QString line = msg + "";
    ui->output->setPlainText(ui->output->toPlainText() + line);

    // auto scroll
    QScrollBar *scroll = ui->output->verticalScrollBar();
    scroll->setValue(scroll->maximum());
}


void MainWindow::send(QString msg)
{
    // jeżeli nie wybrano żadnego urządzenia nie wysyłamy
    if(info == NULL) {
        ui->statusBar->showMessage("Not port selected",3000);
        return;
    }

    if(port.isOpen()) {
        port.write(msg.toStdString().c_str());
        port.waitForBytesWritten(-1);
    }

    // spodziewamy się odpowiedzi więc odbieramy dane
    receive();
}

void MainWindow::timer_receive(){
    // jeżeli nie wybrano żadnego urządzenia nie odbieramy
    if(info != NULL) {
        receive();
    }
}

void MainWindow::receive()
{

    QByteArray r_data = port.readAll();
    QString str(r_data);
    addTextToConsole(str);

    /*
            // czekamy 2 sekund na odpowiedź
            if(port.waitForReadyRead(20)) {
                QByteArray r_data = port.readAll();


                // sprawdzamy czy nie dojdą żadne nowe dane w 10ms
                while(port.waitForReadyRead(10)) {
                    r_data += port.readAll();
                }

                QString str(r_data);
                //str.remove("");

                addTextToConsole(str);

            }
       */

}


//wgrywanie pliku do esp8266
void MainWindow::on_Upload_clicked()
{
    // jeżeli nie wybrano żadnego urządzenia nie wysyłamy
    if(info == NULL) {
        ui->statusBar->showMessage("Not port selected",3000);
        return;
    }

    //pobranie samej nazwy ze sciezki pliku
    QString fn = path.section("/",-1,-1);
    QString content = ui->code->toPlainText();

    //programowanie w MicroPython
    if (plng == "MicroPython"){
        //przerwanie dzialania ewentualnego skryptu
        QString bs = QByteArray(1, 3);
        send(bs+"\n");
        Sleeper::msleep(100);
        //tworzenie pliku na esp8266
        send("f = open(\""+fn+"\", \"w+\")\n\r");

        send("f.write('''\ \n\r");
        //pobranie kodu i wyslanie go do pliku na esp8266
        QStringList lines = ui->code->toPlainText().split('\n', QString::SkipEmptyParts);
        int lc = lines.count();
        for (int x=0; x<lc;x++){
            QString  t = lines.at(x);
            t.replace("\t","   "); //zamienic taby na spacje
            send(t+" \r");
            Sleeper::msleep(100);

        }
        send("''')\n\r");

        //zamkniecie pliku na esp8266
        send("f.close()\n\r");
    }
    //programowanie w NodeMCU
    else{
        //tworzenie pliku na esp8266
        send("if file.open(\""+fn+"\", \"w+\") then\n");

        //pobranie kodu i wyslanie go do pliku na esp8266
        QStringList lines = ui->code->toPlainText().split('\n', QString::SkipEmptyParts);
        int lc = lines.count();
        for (int x=0; x<lc;x++){
            QString  t = lines.at(x);
            t.remove("\n");
            send("file.writeline([["+t+"]])\n");
            Sleeper::msleep(50);

        }

        //zamkniecie pliku na esp8266
        send("file.close()\n");
        send("end\n");
     }


}


//otwieranie pliku z kodem
void MainWindow::on_fOpen_clicked()
{
    path = QFileDialog::getOpenFileName(this,tr("Open File"), path, tr("all types (*)"));
    QFile file(path);

    file.open(QFile::ReadOnly | QFile::Text);

    QTextStream ReadFile(&file);
    //zamienic taby na spacje
    QString tcod = ReadFile.readAll();
    tcod.replace("\t","   ");
    ui->code->setPlainText(tcod);

    //pobranie samej nazwy ze sciezki pliku
    QString fn = path.section("/",-1,-1);
    ui->statusBar->showMessage("File:"+fn,3000);
}

//zapisanie pliku z kodem
void MainWindow::on_fSave_clicked()
{
    QFile codeFile(path);
        //otwieranie pliku do zapisu tryb tekstowy ladnie tez zrobi EOF
        if (codeFile.open(QFile::WriteOnly | QFile::Text)){
            QTextStream out(&codeFile);
            out << ui->code->toPlainText();
            codeFile.close();
        }
}

//reset ESP8266
void MainWindow::on_Reset_clicked()
{
    // jeżeli nie wybrano żadnego urządzenia nie wysyłamy
    if(info == NULL) {
        ui->statusBar->showMessage("Not port selected",3000);
        return;
    }

    send("node.restart()\n");
    ui->statusBar->showMessage("Restart ESP8266",3000);
}

//formatowanie pamieci ESP8266
void MainWindow::on_Format_clicked()
{
    // jeżeli nie wybrano żadnego urządzenia nie wysyłamy
    if(info == NULL) {
        ui->statusBar->showMessage("Not port selected",3000);
        return;
    }

    send("file.format()\n");
    ui->statusBar->showMessage("Erasing memory",3000);
}

//zmiana systemu NodeMCU/MicroPython
void MainWindow::on_System_currentIndexChanged(const QString &arg1)
{
     ui->statusBar->showMessage("Programming in "+arg1);
     plng = arg1;
}
