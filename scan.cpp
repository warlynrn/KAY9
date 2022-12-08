#include "scan.h"
#include <QFileDialog>

Scan::Scan(QObject *parent)
    : QObject{parent}
{
    //Functions overload
    connect(&m_process,&QProcess::errorOccurred,this,&Scan::errorOccurred);
    connect(&m_process,&QProcess::readyReadStandardError,this,&Scan::readyReadStandardError);
    connect(&m_process,&QProcess::readyReadStandardOutput,this,&Scan::readyReadStandardOutput);
    connect(&m_process,&QProcess::started,this,&Scan::started);
    connect(&m_process,&QProcess::readyRead,this,&Scan::readyRead);
    connect(&m_process,&QProcess::stateChanged,this,&Scan::stateChanged);
    connect(&m_process,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,&Scan::finished);

    //variable initiation
    m_running = false;
    m_path = "";
    m_scantype = "";
    m_recursive = true;
    infected_amount_ms = 0;
    infected_amount_fq = 0;
    total_files = 0;
    file_counter = 0;
}

QString Scan::operatingSystem()
{
    //return out the operating system
    return QSysInfo::prettyProductName();
}

//returns the current targeted directory's or file's path
QString Scan::getPath() const
{
    return m_path;
}

//sets the current targeted directory's or file's path
void Scan::setPath(const QString &path)
{
    m_path = path;
}

//start
void Scan::start()
{
    //debug
    //print out the function and where we are in the file
    if(m_running) stop();
    qInfo() << Q_FUNC_INFO; //print to console
    m_running = true;
    m_process.start(getProcess());
}

//stop
void Scan::stop()
{
    //debug
    //print out the function and where we are in the file
    emit pbar_output(0); //reset progress bar

    qInfo() << Q_FUNC_INFO;  //print to console
    m_running = false;
    m_process.kill();
    m_process.close();
}

//catches error
void Scan::errorOccurred(QProcess::ProcessError error)
{
    if (!m_running) return; //if not running just break out
    qInfo() << Q_FUNC_INFO << error;
    emit output("Error");
}

//once process is finished // overload
void Scan::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_running) return; //if not running just break out
    qInfo() << Q_FUNC_INFO;
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    emit output("Completed");
}

//catches standard error // overload
void Scan::readyReadStandardError()
{
    if(!m_running) return; //if not running just break
    qInfo() << Q_FUNC_INFO; //func info print to console
    QByteArray data = m_process.readAllStandardError(); //read error
    QString message = "Standard Error: ";
    message.append(data);
    emit output(message);  //output the erros
}

//catches standard output
void Scan::readyReadStandardOutput()
{
    if(!m_running) return; //if not running just break
    qInfo() << Q_FUNC_INFO; //func info
    QByteArray data = m_process.readAllStandardOutput(); //read error
    emit output(data.trimmed());  //output
}

//confirms to the console when process started
void Scan::started()
{
    qInfo() << Q_FUNC_INFO; //confirm that the process was called
}

//state chance overload
void Scan::stateChanged(QProcess::ProcessState newState)
{
    qInfo() << Q_FUNC_INFO;
    switch(newState){
        //always in one of this 3 cases
        case QProcess::NotRunning:
            m_running = false;
            emit output("Not Running");
            emit currently_running_output();
            break;
        case QProcess::Starting:
            emit output("Starting...");
            break;
        case QProcess::Running:
            emit output("Running");
            startScan();    //start the scan
            break;

    }
}

//infected_found updates the views
//on the virus counter
void Scan::infected_found(){
    if(m_scantype=="fdscan") {
        infected_amount_ms += 1;
        emit infected_ms_output(QString::number(infected_amount_ms));
    } else if (m_scantype=="fullscan" || m_scantype=="quickscan") {
        infected_amount_fq += 1;
        emit infected_fq_output(QString::number(infected_amount_fq));
    }
}

//Reads clear outputs from the process
void Scan::readyRead()
{
    if(!m_running) return; //dont run if process not running
    qInfo() << Q_FUNC_INFO; //console
    QByteArray data = m_process.readAll().trimmed();
    qInfo() << data;
    emit output(data);

    //if virus is found
    if(data.contains(" FOUND")){
        infected_found();

    //if clamscan is done
    } else if(data.contains("----------- SCAN SUMMARY -----------") || data.contains("Can't create temporary directory")|| data.contains("daily.cld updated") || data.contains("daily.cld database is up-to-date")){
        this->stop();
        emit output("Completed");
    }

    //***************    count current scanned files  ***************
    if(m_scantype!="rdb"){
        if(data.contains("\n")){file_counter+=data.count("\n")+1;}
        else {file_counter+=+1;}
        emit output("file_counter %: "+QString::number(file_counter));
        emit output("TOTAL %: "+QString::number(total_files));
        int Percent = ((file_counter*100)/(total_files));
        if(m_running) emit pbar_output(Percent);
    }
}

//returns the current process to use
QString Scan::getProcess()
{
    qInfo() << Q_FUNC_INFO;
    if(QSysInfo::productType()=="windows") return "cmd";
    return "bash"; //for anyhthing else
}

//computes the right command
//to run in the process
QString Scan::cmd_ret()
{
    QByteArray command;
    QByteArray clamavPath = ".\\clamav\\";


    //================= FULL SCAN =============//
    if(m_scantype=="fullscan"){
        infected_amount_fq = 0;                                             //reset counter
        emit infected_fq_output(QString::number(infected_amount_fq));
        command.append(clamavPath+"clamscan -r --move=.\\QVirus C:\\");
        if(QSysInfo::productType()=="windows") command.append("\r");
        command.append("\n");
        total_files += countfiles("\\",true);

    //============= FILE/DIRECTORY SCAN ==========//
    } else if(m_scantype == "fdscan") {
        infected_amount_ms = 0;                                             //reset counter
        emit infected_ms_output(QString::number(infected_amount_ms));       //updates output
        command.append(clamavPath+"clamscan --move=.\\QVirus ");
        if(m_recursive){ command.append("-r ");}
        command.append(m_path.toUtf8());
        if(QSysInfo::productType()=="windows") command.append("\r");
        command.append("\n");
        total_files = countfiles(m_path.toUtf8(),m_recursive)+5;            //count how many files will be scanned

    //================= QUICK SCAN =============//
    } else if(m_scantype == "quickscan") {
        infected_amount_fq = 0;                                             //reset counter
        emit infected_fq_output(QString::number(infected_amount_fq));
        QString _userprofile(qgetenv("USERPROFILE")); //getting path user's path
        QString _systemdrive(qgetenv("SYSTEMDRIVE")); //getting localdrive

        QString paths=" ";
        QFile file(".\\qs_dirs.txt");  //get QUICKSCAN directories list

        //for eachline/directory
        if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
            QTextStream stream(&file);
            while(!stream.atEnd()){
                QString line = stream.readLine();                     //make it a single line
                line.remove("\n");
                line.remove("\r");
                line.replace("%USERPROFILE%",_userprofile); //replace place holder
                line.replace("%SYSTEMDRIVE%",_systemdrive); //replace place holder
                paths.append(line+" ");
                total_files += countfiles(line,true);
            }
            total_files += 5;
            file.close();
        }

        command.append(clamavPath+"clamscan -r --move=.\\QVirus "+paths.toUtf8());
        if(QSysInfo::productType()=="windows") command.append("\r");
        command.append("\n");

    //================= FRESHCLAM =============//
    } else if(m_scantype == "rdb") {
        command.append(clamavPath+"freshclam");
        if(QSysInfo::productType()=="windows") command.append("\r");
        command.append("\n");
    } else {
        return "";
    }

    return command;
}

void Scan::startScan()
{
    total_files = 0;    //restart counters
    file_counter = 0;
    emit currently_running_output(); //update which process is running

    //write command
    m_process.write(cmd_ret().toUtf8());    //write in chars bytes

}

//count files Recursively
int Scan::countfiles(QString dirpath,bool recursive = false){
    int files_inDir = 0;        //local counter
    QDir dir(dirpath);
    if(!dir.exists()){qWarning()<<"Path not Found";return 0;}   //invalid path

    //all entities excluding parent directory(..) and current directory(.)
    QFileInfoList list = dir.entryInfoList(QDir::Filter::NoDotAndDotDot | QDir::Filter::AllEntries);

    //for each entity
    foreach(QFileInfo fi,list){
        if(fi.isDir() && recursive){    //if directory
            files_inDir+=countfiles(fi.absoluteFilePath(),recursive); //next recursive call

        } else if(fi.isFile()) {    //increase counter
            files_inDir += 1;
        }
    }
    return files_inDir;
}

void Scan::killProcess()
{
    m_process.kill();
    m_process.close();
}


