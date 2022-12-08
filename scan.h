#ifndef SCAN_H
#define SCAN_H

#include <QObject>
#include <QDebug>
#include <QSysInfo>
#include <QProcess>
#include <QRegularExpression>

class Scan : public QObject
{
    Q_OBJECT
public:
    explicit Scan(QObject *parent = nullptr);
    //Flags:
    bool m_recursive;
    bool m_running;         //running true or false

    //support local vars
    QString m_scantype;         //what type of command to run
    int infected_amount_ms;
    int infected_amount_fq;
    int total_files;
    int file_counter;

    //support functions
    QString cmd_ret();          //calculates what cmd to call
    QString operatingSystem();  //returns the operating system
    QString getPath() const;    //gets m_path
    void setPath(const QString &path); //changes m_path
    void infected_found();
    int countfiles(QString dirpath, bool recursive);
    void killProcess();

signals:
    void output(QString data);
    void infected_ms_output(QString data);
    void infected_fq_output(QString data);
    void pbar_output(int data);
    void currently_running_output();

public slots:
    void start();   //starts process
    void stop();    //stops process

private slots:
    void errorOccurred(QProcess::ProcessError error);   //handles error occurance
    void finished(int exitCode, QProcess::ExitStatus exitStatus); //exit
    void readyReadStandardError();                        //reads process output
    void readyReadStandardOutput();                       //reads errors from process
    void started();                                       //confirms if process was called
    void stateChanged(QProcess::ProcessState newState);   //updates state(runing, not running, starting...)
    void readyRead();
private:
    QProcess m_process;     //Prorcess initializer
    QString m_path;         //Selected File path
    QString getProcess();   //get's process info
    void startScan();       //starts the process when its ready

};

#endif // SCAN_H
