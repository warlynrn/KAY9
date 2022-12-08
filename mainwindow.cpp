#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QWidget>
#include <QFileDialog>
#include <QProcess>
#include <QSysInfo>
#include <QDebug>
#include "scan.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //set logo
    QPixmap pix(".\\include\\kay9.png");
    ui->logo->setPixmap(pix);

    //fucntions overload
    connect(ui->btnStopScan,&QPushButton::clicked,&m_scan,&Scan::stop);
    connect(ui->btnStopScan_FQ,&QPushButton::clicked,&m_scan,&Scan::stop);
    connect(ui->stopbut_ms,&QPushButton::clicked,&m_scan,&Scan::stop);

    connect(&m_scan,&Scan::output,this,&MainWindow::output);
    connect(&m_scan,&Scan::infected_ms_output,this,&MainWindow::infected_ms_output);
    connect(&m_scan,&Scan::infected_fq_output,this,&MainWindow::infected_fq_output);
    connect(&m_scan,&Scan::pbar_output,this,&MainWindow::pbar_output);
    connect(&m_scan,&Scan::currently_running_output,this,&MainWindow::currently_running_output);

    //set progress bars = 0
    pbar_output(0);

    //variables and flags initializations
    timer  = new QTimer();
    Scheduled_Path = "";
    Scheduled_scantype ="";
    Scheduled_recursive = true;
    scheduled = false;
}

MainWindow::~MainWindow()
{
    m_scan.killProcess(); //kill process
    delete ui;
}

//=============== Start ==================//

//Schedule fucntionality
void MainWindow::on_setintbut_clicked()
{
    //===== Set type of scan/update 
    QString scantype = "";
    if(ui->rBFullScan_SS->isChecked()) {Scheduled_scantype = "fullscan"; scantype="Full Scan";}
    else if(ui->rBQuickScan_SS->isChecked()) {Scheduled_scantype = "quickscan";scantype="Quick Scan";}
    else if(ui->rBRDB_SS->isChecked()) {Scheduled_scantype = "rdb";scantype="Refresh Database";}
    else{
        Scheduled_Path = ui->fileselect_2->toPlainText();   //display on ui
        Scheduled_Path.replace("/","\\");   //syntax correction
        Scheduled_scantype="fdscan";
        if(ui->cB_SelectedDirectory_2->isChecked()) Scheduled_recursive = false;   //recursive option
        scantype="Manual Scan";
    } //if end

    //=== collect values
    int hrs = ui->hoursel->value();
    int min = ui->minsel->value();
    std::string display = "Hour(s): " + std::to_string(hrs) + " / Minute(s): " + std::to_string(min);
    ui->intdisplay->setText(scantype+" every "+QString::fromStdString(display));    //display interval set

    //====== transfor to miliseconds
    int mshrs = hrs*60*60*1000;
    int msmin = min*60*1000;
    int total = mshrs + msmin;

    //==== set interval
    timer->setInterval(total);  //set interval

    //=== connect the timer to the trigger
    connect(timer, &QTimer::timeout,this,[=](){
        if(!m_scan.m_running){      //run only if it is not running already
            //===== collect data saved
            scheduled = true;
            m_scan.setPath(Scheduled_Path);
            m_scan.m_scantype = Scheduled_scantype;

            //==== start 
            m_scan.start();
        }
    });
    //starts timer
    timer->start();
} //

//quick-full scan start
void MainWindow::on_btnStartScan_FQ_clicked()
{
    //set scan type
    if(ui->rBFullScan->isChecked()) m_scan.m_scantype = "fullscan";
    else m_scan.m_scantype = "quickscan";

    scheduled = false;

    //start
    m_scan.start();
} //

//manual scan start
void MainWindow::on_scanbut_clicked()
{
    m_scan.m_scantype="fdscan";
    if(ui->cB_SelectedDirectory->isChecked()) m_scan.m_recursive = false;   //recursive option
    scheduled = false;
    m_scan.start();
    ui->scanstatus->setValue(0);
} //

//Refresh DB/Clam start
void MainWindow::on_btnRefreshDB_clicked()
{
    scheduled=false;
    m_scan.m_scantype="rdb";
    m_scan.start();
} //

//once the process is completed
void MainWindow::completed()
{
    //collect howmany virus were found
    QString virusesnum = "";
    QString ProcessType = "";
    bool quarantine  = false;
    if(m_scan.m_scantype=="quickscan" || m_scan.m_scantype=="fullscan"){
        virusesnum=QString::number(m_scan.infected_amount_fq);
        if(m_scan.infected_amount_fq>0) {
            quarantine = true;
            virus_alert_change("Viruses were found. Please visit Quarantine");
        }
        if(m_scan.m_scantype=="quickscan") ProcessType="Quick Scan";
        if(m_scan.m_scantype=="fullscan") ProcessType="Full Scan";
    } else if(m_scan.m_scantype=="fdscan") {
        virusesnum=QString::number(m_scan.infected_amount_ms);
        ProcessType="Manual Scan Database";
        if(m_scan.infected_amount_ms>0) {
            quarantine = true;
            virus_alert_change("Viruses were found. Please visit Quarantine");
        }
    } //end if

    //if it was a completed scheduled scan do not 
    //show dialog box
    if(!scheduled && m_scan.m_scantype!="rdb"){
        QMessageBox completedmsg;
        completedmsg.setText(ProcessType+" Finished");
        completedmsg.setIcon(QMessageBox::Information);
        completedmsg.setInformativeText(virusesnum+" Virus were found");
        
        //==== if virus were found show quarantine button
        if(quarantine){
            QAbstractButton *pBtnQuarantine = completedmsg.addButton(tr("Quarantine"),QMessageBox::NoRole);
            completedmsg.exec();
            if(completedmsg.clickedButton()==pBtnQuarantine){
                insertToQList();
                ui->stackedWidget->setCurrentIndex(3);
            }
            virus_alert_change(" ");

        } else {
            completedmsg.exec();
        } //end if
    } else if(!scheduled){
        ProcessType="Refresh Database";
        QMessageBox completedmsg;
        completedmsg.setText(ProcessType+" Finished");
        completedmsg.setIcon(QMessageBox::Information);
        completedmsg.setInformativeText("Database updated correctly.");
        completedmsg.exec();
    } //end if

} //completed()

//============== Support ===============//
//on_fileselect_textChanged allows the user to
//select a file
void MainWindow::on_fileselect_textChanged(){
    QString c_path = ui->fileselect->toPlainText();
    c_path.replace("/","\\");
    m_scan.setPath(c_path);
}

//allows the user to select a file
void MainWindow::on_filebut_clicked()
{
    QString filename = QFileDialog::getOpenFileNames(
        this,
        tr("Open File"),
        "C://",
        "All Files (*.*);;"
    ).join("");
    ui ->fileselect->setText(filename.toUtf8());
}

//=== insertToQList inserts all file from QVirus to
//the widget list in quarantine
void MainWindow::insertToQList(){
    //quarantineList
    ui->quarantineList->clear();
    QDir dir(".\\QVirus");

    //for each file
    for(const QFileInfo &file : dir.entryInfoList(QDir::Files)){
        ui->quarantineList->addItem(file.fileName());
    }
}//insertToQList

// on_quarantine_delet_all_clicked deletes the virus
// under QVirus
void MainWindow::on_quarantine_delet_all_clicked()
{
    //for (item in range(ui->quarantineList->count()){}
    for(int item;item < ui->quarantineList->count();item++){
        QString filepath = ".\\QVirus\\";
        filepath.append(ui->quarantineList->item(item)->text());
        QFile file(filepath);
        file.remove();  //remove local file
    }
    insertToQList();    //update widget list
}

//deletes selected virus from quarantine
void MainWindow::on_quarantine_delete_selected_clicked()
{
    QString filepath = ".\\QVirus\\";
    filepath.append(ui->quarantineList->currentItem()->text());
    QFile file(filepath);
    file.remove();
    insertToQList();    //updates list
}

//stop button
void MainWindow::on_btnStopScan_FQ_clicked()
{
    m_scan.stop();
}

//updates all virus alert text
void MainWindow::virus_alert_change(QString data){
    ui->virus_alert->setText(data);
    ui->virus_alert_2->setText(data);
    ui->virus_alert_3->setText(data);
    ui->virus_alert_4->setText(data);
    ui->virus_alert_5->setText(data);
    ui->virus_alert_6->setText(data);
    ui->virus_alert_7->setText(data);
}

//allows the user to select a directory
void MainWindow::on_dirbut_clicked()
{
    QString filename = QFileDialog::getExistingDirectory(this,"Choose Folder");
    ui ->fileselect->setText(filename.toUtf8());   //update text
}

void MainWindow::on_dirbut_2_clicked()
{
    QString filename = QFileDialog::getExistingDirectory(this,"Choose Folder");
    ui ->fileselect_2->setText(filename.toUtf8());
}

//============== most buttons ============//
void MainWindow::on_open_clicked()
{
    ui->stackedWidget->setCurrentIndex(5);
}

void MainWindow::on_exit_clicked()
{
    this->close();
}

void MainWindow::on_backbut_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::on_backbut2_clicked()
{
    ui->stackedWidget->setCurrentIndex(5);
}


void MainWindow::on_scheduledbut_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::on_manualscanbut_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::on_reportsbut_clicked()
{
    ui->stackedWidget->setCurrentIndex(4);
}

void MainWindow::on_quickscanbut_clicked()
{
    ui->stackedWidget->setCurrentIndex(6);
}

void MainWindow::on_btnquarantine_FQ_clicked()
{
    virus_alert_change(" ");
    ui->stackedWidget->setCurrentIndex(3);
}

void MainWindow::on_backbut_7_clicked()
{
    ui->stackedWidget->setCurrentIndex(5);
}

void MainWindow::on_quarantinebut_clicked()
{
    insertToQList();
    virus_alert_change(" ");
    ui->stackedWidget->setCurrentIndex(3);
}

void MainWindow::on_backbut_8_clicked()
{
    ui->stackedWidget->setCurrentIndex(5);
}

void MainWindow::on_backbut_4_clicked()
{
    ui->stackedWidget->setCurrentIndex(5);
}

void MainWindow::on_backbut_2_clicked()
{
    ui->stackedWidget->setCurrentIndex(5);
}

void MainWindow::on_btnLogs_FQ_clicked()
{
    ui->stackedWidget->setCurrentIndex(4);
}

void MainWindow::on_cancelbut_clicked()
{

    timer->stop();
    ui->intdisplay->setText("");
}

//===================== outputs ======================//
//update Bar in all screens
void MainWindow::pbar_output(int data)
{
    ui->scanstatus->setValue(data);
    ui->scanstatus_3->setValue(data);
    ui->scanstatus_4->setValue(data);
    ui->scanstatus_5->setValue(data);
    ui->scanstatus_6->setValue(data);
    ui->scanstatus_7->setValue(data);
    emit output("curent %: "+QString::number(data));
}

//updates running_process in all the screens
void MainWindow::currently_running_output()
{
    QString _running;
    if(m_scan.m_running==false){
        _running = " ";
        pbar_output(0);

        completed();
    } else if(m_scan.m_scantype=="quickscan"){
        _running = "Quick Scan Is Running";
    } else if (m_scan.m_scantype=="fullscan") {
        _running = "Full Scan Is Running";
    } else if (m_scan.m_scantype=="fdscan") {
        _running = "Manual Scan Is Running";
    } else if (m_scan.m_scantype=="rdb") {
        _running = "Refresh Database Is Running";
    }
    ui->Currently_running->setText(_running);
    ui->Currently_running_2->setText(_running);
    ui->Currently_running_3->setText(_running);
    ui->Currently_running_4->setText(_running);
    ui->Currently_running_5->setText(_running);
    ui->Currently_running_6->setText(_running);
}

void MainWindow::output2(QString data){
    ui->plainTextEdit_2->appendPlainText(data);
}

void MainWindow::output(QString data)
{
    ui->maninfo->appendPlainText(data);
    ui->plainTextEdit_2->appendPlainText(data);

}

void MainWindow::infected_ms_output(QString data)
{
    ui->text_infectedFiles_ms->setText(data);
}

void MainWindow::infected_fq_output(QString data)
{
    ui->text_infectedFiles_fq->setText(data);
}
