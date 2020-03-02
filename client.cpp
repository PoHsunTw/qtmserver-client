#include "client.h"
#include "ui_client.h"

#include <QPixmap>
#include <QPainter>
#include <QMessageBox>
#include <QDialog>
#include <QString>
#include <QFileDialog>
#include <QScrollBar>



client::client(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::client)
{
    ui->setupUi(this);
    ui->stopButton->setEnabled(false);
    ui->linkButton->setEnabled(true);
    ui->remoteCtlGB->setEnabled(false);
    is_connected = false;
    fileIn = false;
    is_connected = false;
    MCStatusUpdateCon = false;
    MCLogsUpdateCon  = false;
    mcServerLogsSizeToReceive = NULL;
    eventLock="";
    MCGetLogsTimeoutTime=2000;
}

client::~client()
{
    delete ui;
}

QString client::htmlColor(const QString &msg, const QString &color)
{
    return QString("<font color=\"%1\">%2</font>").arg(color).arg(msg);
}

QString client::htmlBlue(const QString &msg)
{
    return htmlColor(msg, "blue");
}

QString client::htmlRed(const QString &msg)
{
    return htmlColor(msg, "red");
}

QString client::htmlGreen(const QString &msg)
{
    return htmlColor(msg, "green");
}

QString client::htmlPurple(const QString &msg)
{
    return htmlColor(msg, "purple");
}

QString client::getTime(){
    return QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ");
}

void client::initialize()
{
    statusLedLabel = new QLabel;
    if(statusLedLabel)
    {
        statusBar()->addWidget(statusLedLabel);
        statusLedLabel->setPixmap(QPixmap("://images/led-red.png"));
    }

    statusLabel = new QLabel;
    if(statusLabel)
    {
        statusBar()->addWidget(statusLabel);
        statusLabel->setText(tr("Remote Server: Disconnected"));
    }

    mcStatusLedLabel = new QLabel;
    if(mcStatusLedLabel)
    {
        statusBar()->addWidget(mcStatusLedLabel);
        mcStatusLedLabel->setPixmap(QPixmap("://images/led-red.png"));
    }
    mcStatusLabel = new QLabel;
    if(mcStatusLabel)
    {
        statusBar()->addWidget(mcStatusLabel);
        mcStatusLabel->setText(tr("Minecraft Server: Unknow"));
    }
    dev_mode = false;
    debug_mode = false;
    ui->MCStatusUpdateCB->setEnabled(false);
    ui->MCLogsUpdateCB->setEnabled(false);
    MCStatusMSec = 0;
    MCLogsMSec = 0;
}

void client::closeConnection()
{
    QString remoteLog = getTime();
    if(judge==true)
    {
        remoteLog.append(htmlRed("connection stopped"));
    }
    else
    {
        remoteLog.append(htmlRed("connection had been stopped!"));
    }
    ui->stopButton->setEnabled(false);
    ui->linkButton->setEnabled(true);
    ui->remoteCtlGB->setEnabled(false);
    is_connected = false;

    statusLedLabel->setPixmap(QPixmap("://images/led-red.png"));
    statusLabel->setText("Remote Server: Disconnected");
    ui->remoteLogTextEdit->append(remoteLog);
    disconnect(tcpClient,SIGNAL(connected()),this,SLOT(try_connect()));
    disconnect(tcpClient,SIGNAL(disconnected()),this,SLOT(stopTransfer()));
    disconnect(tcpClient,SIGNAL(readyRead()),this,SLOT(slot_readMessage()));
    tcpClient->close();
}

void client::on_linkButton_clicked()
{
    qDebug()<<"[PIT]on_linkButton_clicked";
    totalBytes   = 0;
    byteReceived = 0;
    fileNameSize = 0;
    ip = ui->ipEdit->text();
    port = ui->portEdit->text().toUInt();
    if(!ui->keyEdit->text().isEmpty()){
        connectKeyBA  = ui->keyEdit->text().toLatin1();
    }else{
        keyAlgorithm();
    }
    QHostInfo info=QHostInfo::fromName(ip);  //初始動作
    QHostAddress address=info.addresses().first();  //返回第一個address
    ip=address.toString();    //轉成字串
    QString remoteLog = getTime();
    remoteLog.append(tr("connecting...."));
    ui->remoteLogTextEdit->append(remoteLog);
    ui->linkButton->setEnabled(false);
    ui->stopButton->setEnabled(true);
    tcpClient = new QTcpSocket(this);
    tcpClient->connectToHost(QHostAddress(ip), port);
    connect(tcpClient,SIGNAL(connected()),this,SLOT(try_connect()));
    connect(tcpClient,SIGNAL(disconnected()),this,SLOT(stopTransfer()));
    connect(tcpClient,SIGNAL(readyRead()),this,SLOT(slot_readMessage()));
}

void client::on_stopButton_clicked()
{
    judge = true;
    closeConnection();
}

void client::on_cmdlinkButton_clicked()
{
    QString cmdStr = ui->cmdEdit->text();
    QString remoteLog = getTime();
    if(cmdStr=="pohsun"){
        dev_mode=!dev_mode;
        QString dev_mode_str = dev_mode?"on":"off";
        remoteLog.append(htmlPurple("dev_mode: "+ dev_mode_str));
        ui->remoteLogTextEdit->append(remoteLog);
        return;
    }
    if(statusLabel->text()=="Remote Server: Disconnected"){
        return;
    }
    QString strHead = "command|";
    QString strSend = strHead + cmdStr;
    tcpClient->write(strSend.toLatin1());
    tcpClient->waitForBytesWritten();

    if(dev_mode||debug_mode){
        remoteLog = getTime();
        remoteLog.append("<< "+strSend);
        ui->remoteLogTextEdit->append(remoteLog);
    }
    ui->cmdEdit->clear();
}

void client::slot_readMessage() // receive msg by socket tcpClient
{
    qDebug()<<"[PIT]slot_readMessage";
    if(fileIn)
    {
        getfile();
        return;
    }
    QByteArray strBA = tcpClient->readAll();
    QString strIN=QVariant(strBA).toString();

    QStringList strINList = strIN.split('|');
    QString strType="";
    QString strCommand="";
    if(eventLock != ""){
        if(eventLock=="mcServerLogs"){
            strType="mcServerLogs";
            strCommand =strIN;
        }
    }else if(strINList.length()>=2){
        strType=strINList[0];
        strCommand=strINList[1];
    }
    qDebug()<<"[Debug-len]strINsize:"<<strIN.size()<<endl<<"[Debug-InType]"<<strType;
    QString remoteLog = getTime();
    if(dev_mode||debug_mode){
        remoteLog.append(">> "+strIN);
        ui->remoteLogTextEdit->append(remoteLog);
    }
    remoteLog = getTime();
    if(strType=="reason"){
        remoteLog.append(htmlRed(strCommand));
        eventLock = "";
    }else if(strType=="file"){
        ui->remoteConGB->setEnabled(false);
        ui->remoteCtlGB->setEnabled(false);
        ui->MCLogsConfigGB->setEnabled(false);
        getfile();
    }else if(strType=="remote"){
        QString connectInfo = "connect to-"+ip+":"+QString::number(port);
        if(strCommand=="success"){
            eventLock="";
            ui->linkButton->setEnabled(false);
            ui->stopButton->setEnabled(true);
            ui->remoteCtlGB->setEnabled(true);
            is_connected = true;
            statusLedLabel->setPixmap(QPixmap("://images/led-green.png"));
            statusLabel->setText(connectInfo);
            remoteLog.append(htmlGreen("========success"+connectInfo+"========"));
            connect(this,SIGNAL(labelChanged()),this,SLOT(initial_info()));
            on_statusRefreshButton_clicked();
        }else{
            QString strFailReason = strINList[2];
            remoteLog.append(htmlPurple("["+strCommand+"] "+strFailReason));
        }
    }else if(strType=="mcServerStatus"){
        qDebug()<<"[Debug]mcServerStatus:"<<strCommand;
        if(mcStatusLabel->text()!=strCommand){
            emit labelChanged();
        }
        mcStatusLabel->setText(strCommand);
        QString ledPath = "://images/led-red.png";

        if(strCommand.contains("Running")){
            qDebug()<< "mcServer is Running";
            ledPath = "://images/led-green.png";
        }
        mcStatusLedLabel->setPixmap(QPixmap(ledPath));
        return;
    }else if(strType=="mcServerLogs"){
        // TODO  timer check
        if(mcServerLogsSizeToReceive!=NULL){
            //stage 2 get data & chk
            quint64 receiveSize =  strCommand.size();
            mcServerLogsSizeHasReceive += receiveSize;
            mcServerLogsTemp += strCommand;
            qDebug()<<"[Debug]mcServerLogsSizeToReceive:"<<mcServerLogsSizeToReceive;
            qDebug()<<"[Debug]mcServerLogsSizeHasReceive:"<<mcServerLogsSizeHasReceive;
            if(mcServerLogsSizeHasReceive == mcServerLogsSizeToReceive){
                //finish
                qDebug()<<"[PIT]mcServerLogs-finished";
                if(dev_mode||debug_mode){
                    remoteLog=getTime();
                    remoteLog.append("<< mcServerLogs|finished");
                    ui->remoteLogTextEdit->append(remoteLog);
                }
                tcpClient->write("mcServerLogs|finished");
                tcpClient->waitForBytesWritten();

                MCGetLogsTimer->destroyed();
                disconnect(MCGetLogsTimer,SIGNAL(timeout()),this,SLOT(mcGetLogsTimeout()));

                mcServerLogsSizeToReceive  = NULL;
                eventLock  = "";
                ui->mcServerLogTextEdit->clear();
                ui->mcServerLogTextEdit->setHtml(mcServerLogsTemp);
                ui->mcServerLogTextEdit->verticalScrollBar()->setValue(ui->mcServerLogTextEdit->verticalScrollBar()->maximum());
                remoteLog=getTime();
                remoteLog.append(htmlGreen("successfully get logs"));
                ui->remoteLogTextEdit->append(remoteLog);
                mcServerLogsTemp = "";
            }else{
                //need more
                qDebug()<<"[PIT]mcServerLogs-more";
                if(dev_mode||debug_mode){
                    remoteLog=getTime();
                    remoteLog.append("<< mcServerLogs|continue receive");
                    ui->remoteLogTextEdit->append(remoteLog);
                }
            }
        }else{
            //stage 1 get lens
            mcServerLogsSizeToReceive = strCommand.toInt();
            mcServerLogsSizeHasReceive = 0;
            mcServerLogsTemp = "";
            qDebug()<<"[Debug]mcServerLogsSizeToReceive:"<<mcServerLogsSizeToReceive;
            eventLock = "mcServerLogs";
            tcpClient->write("mcServerLogs|start");
            tcpClient->waitForBytesWritten();
            if(dev_mode||debug_mode){
                remoteLog=getTime();
                remoteLog.append("<< mcServerLogs|start");
                ui->remoteLogTextEdit->append(remoteLog);
            }
            // TODO QMutexLocker

        }
        return;
    }else if(strType=="mcLogsUpdate"){
        ui->mcServerLogTextEdit->append(strCommand);
        return;
    }
    else{
        qDebug()<<"[Debug]bug:"<<strIN;
        return;
    }
    ui->remoteLogTextEdit->append(remoteLog);
}

void client::keyAlgorithm()
{
    QString dateNow = QDateTime::currentDateTime().toString("yyyy/MM/dd-HH:mm");
    QByteArray dateBA = dateNow.toLatin1();
    connectKeyBA = QCryptographicHash::hash(dateBA,QCryptographicHash::Sha3_512);
}

void client::on_remoteStartButton_clicked()
{
    QString strSend = "button|start";

    QString remoteLog = getTime();
    if(dev_mode||debug_mode){
        remoteLog.append("<< "+strSend);
        ui->remoteLogTextEdit->append(remoteLog);
    }
    tcpClient->write(strSend.toLatin1());
    tcpClient->waitForBytesWritten();
}

void client::on_remoteStopButton_clicked()
{
    QString strSend = "button|stop";

    QString remoteLog = getTime();
    if(dev_mode||debug_mode){
        remoteLog.append("<< "+strSend);
        ui->remoteLogTextEdit->append(remoteLog);
    }
    tcpClient->write(strSend.toLatin1());
    tcpClient->waitForBytesWritten();
}
void client::try_connect()
{
    QString strConnect = "try connect to-"+ip+" : "+QString::number(port);
    QString remoteLog = getTime();
    remoteLog.append(htmlBlue(strConnect));
    ui->remoteLogTextEdit->append(remoteLog);

    QByteArray psw = "key|";
    psw.append(connectKeyBA);
    tcpClient->write(psw);//將資料寫入server端

    remoteLog = getTime();
    if(dev_mode||debug_mode){
        remoteLog.append("<< "+psw);
        ui->remoteLogTextEdit->append(remoteLog);
    }

}

void client::on_getFileButton_clicked()
{
    QString strSend = "file|";

    address =  QFileDialog::getExistingDirectory(this, tr("開啟檔案"),"/home",
                                                 QFileDialog::ShowDirsOnly|
                                                 QFileDialog::DontResolveSymlinks);
    if(address.isEmpty()){
        QString remoteLog = getTime();
        remoteLog.append("Get logs canceled");
        ui->remoteLogTextEdit->append(remoteLog);
        return;
    }
    tcpClient->write(strSend.toLatin1());   // Exception
    tcpClient->waitForBytesWritten();
    fileIn=true;

    QString remoteLog = getTime();
    if(dev_mode||debug_mode){
        remoteLog.append("<< "+strSend+" & try to save at:"+address);
    }else{
        remoteLog.append(htmlPurple("try to save at:"+address));
    }
    ui->remoteLogTextEdit->append(remoteLog);
}
void client::getfile()//
{
    QDataStream in(tcpClient);
    in.setVersion(QDataStream::Qt_4_6);
    //Step 1:先讀表頭(header)
    if(byteReceived <= sizeof(qint64)*2)
    {
        if((fileNameSize == 0) &&
           (tcpClient->bytesAvailable() >= sizeof(qint64)*2))
        {
            in >> totalBytes >> fileNameSize;
            byteReceived += sizeof(qint64)*2;
        }
        if((fileNameSize != 0) &&
           (tcpClient->bytesAvailable() >= fileNameSize))
        {
            in >> fileName;
            byteReceived += fileNameSize;
            QString newFileName=address;
            newFileName.append("/"+fileName);
            localFile =new QFile(newFileName);
            if(!localFile->open(QFile::WriteOnly))
            {
                QMessageBox::warning(this,QStringLiteral("伺服器"),
                                     QStringLiteral("無法開啟檔案 %1:\n%2").arg(fileName)
                                     .arg(localFile->errorString()));
                return;
            }
        }
        else
        {
            return;
        }
    }
    //Step 2:讀資料欄位
    if(byteReceived < totalBytes)
    {
        byteReceived += tcpClient->bytesAvailable();
        inBlock = tcpClient->readAll();
        localFile->write(inBlock);
        inBlock.resize(0);
    }
    /*//Step 3:更新狀態標籤與進度條
    serverProgressBar->setMaximum(totalBytes);
    serverProgressBar->setValue(byteReceived);
    serverStatusLabel->setText(QStringLiteral("以接收 %1 Bytes")
                               .arg(byteReceived));*/
    //Step 4:接收結束
    if(byteReceived == totalBytes)
    {
        ui->remoteConGB->setEnabled(true);
        ui->remoteCtlGB->setEnabled(true);
        ui->MCLogsConfigGB->setEnabled(true);
        fileIn=false;
        localFile->close();
        QString remoteLog = getTime();
        if(dev_mode||debug_mode){
            remoteLog.append(htmlPurple(">> get file:"+address));
        }else{
            remoteLog.append(htmlPurple("get file at:"+address));
        }
        ui->remoteLogTextEdit->append(remoteLog);
    }
}
void client::stopTransfer()
{
    closeConnection();
    judge=false;
}

void client::on_statusRefreshButton_clicked()
{
    QString strSend = "mcServerStatus|";
    tcpClient->write(strSend.toLatin1());
    tcpClient->waitForBytesWritten();

    QString remoteLog = getTime();
    if(dev_mode||debug_mode){
        remoteLog.append("<< "+strSend);
        ui->remoteLogTextEdit->append(remoteLog);
    }
}

void client::on_MCLogsRefreshButton_clicked()
{
    //TODO QMutexLocker
    QString strSend = "mcServerLogs|";

    MCGetLogsTimer = new QTimer;
    MCGetLogsTimer->start(MCGetLogsTimeoutTime);
    connect(MCGetLogsTimer,SIGNAL(timeout()),this,SLOT(mcGetLogsTimeout()));

    tcpClient->write(strSend.toLatin1());
    tcpClient->waitForBytesWritten();
    QString remoteLog = getTime();
    if(dev_mode||debug_mode){
        remoteLog.append("<< "+strSend);
        ui->remoteLogTextEdit->append(remoteLog);
    }
}

void client::on_MCStatusMSecLE_textChanged(const QString &arg1)
{
    if(!arg1.isEmpty()){
        MCStatusMSec = ui->MCStatusMSecLE->text().toUInt();
        ui->MCStatusUpdateCB->setEnabled(true);
    }else{
        MCStatusMSec = 0;
        ui->MCStatusUpdateCB->setEnabled(false);
    }
}

void client::on_MCLogsMSecLE_textChanged(const QString &arg1)
{
    if(!arg1.isEmpty()){
        MCLogsMSec = ui->MCLogsMSecLE->text().toUInt();
        ui->MCLogsUpdateCB->setEnabled(true);
    }else{
        MCLogsMSec = 0;
        ui->MCLogsUpdateCB->setEnabled(false);
        MCLogsUpdateCon  = false;
    }
}

void client::on_MCStatusUpdateCB_toggled(bool checked)
{
    QString remoteLog = getTime();
    if(is_connected||MCStatusUpdateCon){
        if(checked && MCStatusMSec){
            MCStatusTimer = new QTimer;
            MCStatusTimer->start(MCStatusMSec);
            connect(MCStatusTimer,SIGNAL(timeout()),this,SLOT(on_statusRefreshButton_clicked()));
            MCStatusUpdateCon = true;

        }else{
            disconnect(MCStatusTimer,SIGNAL(timeout()),this,SLOT(on_statusRefreshButton_clicked()));
            MCStatusUpdateCon = false;
        }
        QString checkedStr = checked?"on":"off";
        remoteLog.append(htmlBlue("mcStatusAutoupdate:"+checkedStr));
        ui->remoteLogTextEdit->append(remoteLog);
    }
}

void client::on_MCLogsUpdateCB_toggled(bool checked)
{
    QString remoteLog = getTime();
    if(is_connected||MCLogsUpdateCon){
        qDebug()<<"[PIT]mcLogsAutoupdate";
        if(checked && MCLogsMSec){
            MCLogsTimer = new QTimer;
            MCLogsTimer->start(MCLogsMSec);
            connect(MCLogsTimer,SIGNAL(timeout()),this,SLOT(mcLogsUpdate_timer()));
            MCLogsUpdateCon  = true;
        }else{
            MCLogsTimer->destroyed();
            disconnect(MCLogsTimer,SIGNAL(timeout()),this,SLOT(mcLogsUpdate_timer()));
            MCLogsUpdateCon  = false;
        }
        QString checkedStr = checked?"on":"off";
        remoteLog.append(htmlBlue("mcLogsAutoupdate:"+checkedStr));
        ui->remoteLogTextEdit->append(remoteLog);
    }
}

void client::mcLogsUpdate_timer(){
    QString remoteLog = getTime();
    remoteLog.append("---mcLogsAutoupdate---");
    ui->mcServerLogTextEdit->append(remoteLog);

    on_MCLogsRefreshButton_clicked();
}

void client::initial_info(){
    qDebug()<<"[PIT]initial_info";
    disconnect(this,SIGNAL(labelChanged()),this,SLOT(initial_info()));
    on_MCLogsRefreshButton_clicked();
}

void client::mcGetLogsTimeout(){
    QString remoteLog = getTime();
    if(eventLock=="mcServerLogs"){
        remoteLog.append(htmlPurple("Get logs fail"));
    }else{
        remoteLog.append(htmlPurple("somethig ran out wrong"));
    }

    ui->mcServerLogTextEdit->append(remoteLog);
    MCGetLogsTimer->destroyed();
    disconnect(MCGetLogsTimer,SIGNAL(timeout()),this,SLOT(mcGetLogsTimeout()));
}
