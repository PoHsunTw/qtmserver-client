#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QWidget>
#include <QtNetwork>
#include <QDialog>
#include <QLabel>

namespace Ui {
class client;
}

class client : public QMainWindow
{
    Q_OBJECT

public:
    explicit client(QWidget *parent = 0);
    ~client();

    void initialize();
    void closeConnection();
    void getfile();
    void keyAlgorithm();

    QString htmlColor(const QString& msg, const QString& color);
    QString htmlBlue(const QString& msg);
    QString htmlRed(const QString& msg);
    QString htmlGreen(const QString& msg);
    QString htmlPurple(const QString& msg);
    QString getTime();

signals:
    void labelChanged();

private slots:
    void on_linkButton_clicked();

    void on_stopButton_clicked();

    void on_cmdlinkButton_clicked();

    void slot_readMessage();

    void on_remoteStartButton_clicked();

    void on_remoteStopButton_clicked();
    void try_connect();//void startTransfer();
    void stopTransfer();
    void on_getFileButton_clicked();

    void on_statusRefreshButton_clicked();

    void on_MCLogsRefreshButton_clicked();

    void on_MCStatusMSecLE_textChanged(const QString &arg1);

    void on_MCLogsMSecLE_textChanged(const QString &arg1);

    void on_MCStatusUpdateCB_toggled(bool checked);

    void on_MCLogsUpdateCB_toggled(bool checked);

    void mcLogsUpdate_timer();

    void initial_info();

    void mcGetLogsTimeout();

private:
    Ui::client *ui;

    QLabel     *statusLabel;
    QLabel     *statusLedLabel;
    QLabel     *mcStatusLabel;
    QLabel     *mcStatusLedLabel;

    QString    ip;//ip變數
    quint16    port;//port變數
    qint64     bytesWritten;
    quint16    players;
    QTcpSocket *tcpClient;
    QString    command;
    QByteArray connectKeyBA;
    bool       fileIn;

    qint64     totalBytes;
    qint64     byteReceived;
    qint64     fileNameSize;
    QString    fileName;
    QFile      *localFile;
    QByteArray inBlock;
    bool       judge=false;
    QString    address;
    bool       dev_mode;
    bool       debug_mode;
    bool       is_connected;
    quint64    MCStatusMSec;
    quint64    MCLogsMSec;
    QTimer*    MCStatusTimer;//for auto update
    QTimer*    MCLogsTimer;//for auto update
    QTimer*    MCGetLogsTimer;
    quint16    MCGetLogsTimeoutTime;

    bool       MCStatusUpdateCon;
    bool       MCLogsUpdateCon;

    QString    eventLock;
    quint64    mcServerLogsSizeHasReceive;
    quint64    mcServerLogsSizeToReceive;
    QString    mcServerLogsTemp;

};

#endif // CLIENT_H
