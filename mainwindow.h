#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "scan.h"
#include <QObject>
#include <QSysInfo>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void output(QString data);
    void output2(QString data);
    void output3(QString data);
    void infected_ms_output(QString data);
    void infected_fq_output(QString data);
    void pbar_output(int data);
    void currently_running_output();
    void completed();
    void virus_alert_change(QString data);

    QTimer* timer;
    QString Scheduled_Path;
    QString Scheduled_scantype;
    bool Scheduled_recursive;
    bool scheduled;

private slots:

    void on_fileselect_textChanged();

    void on_open_clicked();

    void on_exit_clicked();

    void on_backbut_clicked();

    void on_dirbut_clicked();

    void on_filebut_clicked();

    void on_backbut2_clicked();

    void on_scheduledbut_clicked();

    void on_manualscanbut_clicked();

    void on_reportsbut_clicked();

    void on_quickscanbut_clicked();

    void on_setintbut_clicked();

    void on_cancelbut_clicked();

    void on_backbut_2_clicked();

    void on_scanbut_clicked();

    void on_btnLogs_FQ_clicked();

    void on_quarantinebut_clicked();

    void on_btnquarantine_FQ_clicked();

    void on_backbut_7_clicked();

    void on_btnStartScan_FQ_clicked();

    void on_btnStopScan_FQ_clicked();

    void insertToQList();

    void on_backbut_8_clicked();

    void on_quarantine_delete_selected_clicked();

    void on_quarantine_delet_all_clicked();

    void on_backbut_4_clicked();

    void on_btnRefreshDB_clicked();

    void on_dirbut_2_clicked();

private:
    Ui::MainWindow *ui;
    Scan m_scan;
};
#endif // MAINWINDOW_H
