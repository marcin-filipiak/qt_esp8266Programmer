#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFileDialog>
#include <QTextStream>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QString path;

private slots:
    void on_port_currentIndexChanged(int index);
    void refresh();
    void on_commandLine_returnPressed();
    void on_enterButton_clicked();
    void on_Upload_clicked();
    void timer_receive();
    void on_fOpen_clicked();
    void on_fSave_clicked();

    void on_Reset_clicked();

    void on_Format_clicked();

    void on_System_currentIndexChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;

    void searchDevices();
    void addTextToConsole(QString text, bool sender=false);

    void send(QString msg);
    void receive();
};

#endif // MAINWINDOW_H
