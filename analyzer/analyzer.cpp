#include "analyzer.h"
#include "popupindicator.h"
#include "customanalyzer.h"


Analyzer::Analyzer(QObject *parent) : QObject(parent),
    m_hidAnalyzer(nullptr),
    m_comAnalyzer(nullptr),
    m_analyzerModel(0),
    m_comAnalyzerFound(false),
    m_hidAnalyzerFound(false),
    m_chartCounter(0),
    m_isMeasuring(false),
    m_isContinuos(false),
    m_dotsNumber(100),
    m_downloader(nullptr),
    m_updateDialog(nullptr),
    m_pfw(nullptr),
    m_INFOSIZE(512),
    m_MAGICAA230Z(0xFE02A185),
    m_MAGICHID(0x5c620202),
    m_INTERNALMAGICAA230Z(0x87654321),
    m_MAGICAA30ZERO(0x5c623002),
    m_calibrationMode(false)
{
    m_pfw = new QByteArray;
}

Analyzer::~Analyzer()
{
    if(m_downloader)
    {
        delete m_downloader;
        m_downloader = nullptr;
    }
    delete m_pfw;
    if(m_hidAnalyzer)
    {
        delete m_hidAnalyzer;
        m_hidAnalyzer = nullptr;
    }
    if(m_comAnalyzer)
    {
        comAnalyzer* tmp = m_comAnalyzer;
        m_comAnalyzer = nullptr;
        delete tmp;
    }
}

double Analyzer::getVersion() const
{
    if(m_comAnalyzerFound && m_comAnalyzer != nullptr)
    {
        return m_comAnalyzer->getVersion().toDouble();
    }else if (m_hidAnalyzerFound && m_hidAnalyzer != nullptr)
    {
        return m_hidAnalyzer->getVersion().toDouble();
    }
    return 0;
}

void Analyzer::on_downloadInfoComplete()
{
    QString ver = m_downloader->version();
    if(ver.isEmpty())
    {
        QMessageBox::information(nullptr, tr("Latest version"),
                             tr("Can not get the latest version.\nPlease try later."));
    }else
    {
        double internetVersion = ver.toDouble();//ver.remove(".").toInt();
        m_updateDialog = new UpdateDialog();
        m_updateDialog->setAttribute(Qt::WA_DeleteOnClose);
        m_updateDialog->setWindowTitle(tr("Updating"));
        connect(m_updateDialog,SIGNAL(update()),this,SLOT(on_internetUpdate()));
        connect(this, SIGNAL(updatePercentChanged(int)),m_updateDialog,SLOT(on_percentChanged(qint32)));
        if(internetVersion > getVersion())
        {
            m_updateDialog->setMainText(tr("New version of firmware is available now!"));
        }else
        {
            m_updateDialog->setMainText(tr("You have the latest version of firmware."));
        }
        m_updateDialog->exec();
    }
}

void Analyzer::on_downloadFileComplete()
{
    *m_pfw = m_downloader->file();
    QBuffer fwdata(m_pfw);

    fwdata.open(QIODevice::ReadOnly);
    fwdata.seek(m_INFOSIZE);

    updateFirmware(&fwdata);
}

void Analyzer::on_internetUpdate()
{
    m_downloader->startDownloadFw();
    m_updateDialog->setStatusText(tr("Downloading firmware..."));
}

void Analyzer::readFile(QString pathToFw)
{
    QFile file(pathToFw);
    bool state = true;

    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(nullptr, tr("Warning"), tr("Can not open firmware file."));
        return;
    }

    *m_pfw = file.readAll();

    if (m_pfw->isEmpty())
    {
        QMessageBox::warning(nullptr, tr("Warning"), tr("Can not read firmware file."));
        state = false;
    }

    file.close();

    if(state)
    {        
        //m_updateDialog->setStatusText(tr("Updating, please wait..."));
        QBuffer fwdata(m_pfw);
        fwdata.open(QIODevice::ReadOnly);
        fwdata.seek(m_INFOSIZE);
        updateFirmware(&fwdata);
    }
}

bool Analyzer::checkFile(QString path)
{
    QFile fwfile(path);
    QByteArray arr;
    QByteArray fw;
    quint32 magic = 0;
    quint32 len = 0;
    quint32 readCrc = 0;
    quint32 revision = 0;
    const char *pd;

    if (!fwfile.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(nullptr, tr("Warning"),
                             tr("Firmware file can not open"));
        return false;
    }

    arr = fwfile.read(m_INFOSIZE);

    if ((qint32)arr.length() < m_INFOSIZE)
    {
        fwfile.close();
        return false;
    }

    pd = arr.constData();

    magic = qFromLittleEndian<quint32>(*((quint32*)pd));
    len = qFromLittleEndian<quint32>(*((quint32*)&pd[4]));
    readCrc = qFromLittleEndian<quint32>(*((quint32*)&pd[8]));
    revision = qFromLittleEndian<quint32>(*((quint32*)&pd[20]));

    if(m_hidAnalyzer)
    {
        //m_hidAnalyzer->setRevision(revision); // old
        m_hidAnalyzer->setRevision(QString::number(revision));
    }

    QString prototype = CustomAnalyzer::currentPrototype();
    if(((names[getAnalyzerModel()] == "AA-230 ZOOM" || prototype == "AA-230 ZOOM") && (magic == m_MAGICAA230Z)) ||
            ((names[getAnalyzerModel()] == "AA-55 ZOOM" || prototype == "AA-55 ZOOM") && (magic == m_MAGICHID)) ||
            ((names[getAnalyzerModel()] == "AA-35 ZOOM" || prototype == "AA-35 ZOOM") && (magic == m_MAGICHID)) ||
            ((names[getAnalyzerModel()] == "AA-30 ZERO" || prototype == "AA-30 ZERO") && (magic == m_MAGICAA30ZERO)) ||
            ((names[getAnalyzerModel()] == "AA-30.ZERO" || prototype == "AA-30.ZERO") && (magic == m_MAGICAA30ZERO)))
    {
    }else
    {
        QMessageBox::warning(nullptr, tr("Warning"),
                             tr("Firmware file has wrong format"));
        fwfile.close();
        return false;
    }

    if (!fwfile.seek(m_INFOSIZE))
    {
        QMessageBox::warning(nullptr, tr("Warning"),
                             tr("Firmware file is too short."));
        fwfile.close();
        return false;
    }

    fw = fwfile.readAll();

    fwfile.close();

    if ((quint32)fw.length() != len) {
        QMessageBox::warning(nullptr, tr("Warning"),
                             tr("Firmware file has wrong length."));
        return false;
    }

    if (readCrc != CRC32::crc(0xffffffff, fw)) {
        QMessageBox::warning(nullptr, tr("Warning"),
                             tr("Firmware file has wrong CRC."));
        return false;
    }

    return true;
}

QString Analyzer::getModelString( void )
{    
    return CustomAnalyzer::customized() ? CustomAnalyzer::currentPrototype() : names[m_analyzerModel];
}

quint32 Analyzer::getModel( void )
{
    return m_analyzerModel;
}

quint32 Analyzer::getHidModel( void )
{
    return m_hidAnalyzer->getModel();
}

QString Analyzer::getSerialNumber(void) const
{
    if(m_hidAnalyzerFound)
    {
        return m_hidAnalyzer->getSerial();
    }else if(m_comAnalyzerFound)
    {
        return m_comAnalyzer->getSerial();
    }
    return QString();
}
void Analyzer::on_measure (qint64 fqFrom, qint64 fqTo, qint32 dotsNumber)
{
    if(!m_isMeasuring)
    {
        m_isMeasuring = true;
        QDateTime datetime = QDateTime::currentDateTime();
        emit newMeasurement(datetime.toString("##dd.MM.yyyy-hh:mm:ss"));
        m_dotsNumber = dotsNumber;
        m_chartCounter = 0;
        if(m_comAnalyzerFound)
        {
            m_comAnalyzer->startMeasure(fqFrom,fqTo,dotsNumber);
        }else if (m_hidAnalyzerFound)
        {
            m_hidAnalyzer->startMeasure(fqFrom,fqTo,dotsNumber);
        }
        PopUpIndicator::setIndicatorVisible(true);
    } else {
        on_stopMeasure();
    }
}

void Analyzer::on_measureContinuous(qint64 fqFrom, qint64 fqTo, qint32 dotsNumber)
{
    if(!m_isMeasuring)
    {
        m_isMeasuring = true;
        //QThread::msleep(500);
        emit continueMeasurement(fqFrom, fqTo, dotsNumber);
        m_dotsNumber = dotsNumber;
        m_chartCounter = 0;
        if(m_comAnalyzerFound)
        {
            m_comAnalyzer->startMeasure(fqFrom,fqTo,dotsNumber);
        }else if (m_hidAnalyzerFound)
        {
            m_hidAnalyzer->startMeasure(fqFrom,fqTo,dotsNumber);
        }
        PopUpIndicator::setIndicatorVisible(true);
    } else {
        on_stopMeasure();
    }
}

void Analyzer::on_stopMeasure()
{
    PopUpIndicator::setIndicatorVisible(false);
    m_isMeasuring = false;
    m_chartCounter = 0;
    if(m_comAnalyzerFound)
    {
        m_comAnalyzer->stopMeasure();
    }else if (m_hidAnalyzerFound)
    {
        m_hidAnalyzer->stopMeasure();
    }
    emit measurementComplete();
}

void Analyzer::updateFirmware (QIODevice *fw)
{
    if(m_comAnalyzerFound)
    {
        m_comAnalyzer->update(fw);
    }else if (m_hidAnalyzerFound)
    {
        m_hidAnalyzer->update(fw);
    }
}

void Analyzer::setAutoCheckUpdate( bool state)
{
    m_autoCheckUpdate = state;
}

void Analyzer::makeScreenshot()
{
    if(!m_isMeasuring)
    {
        if(m_comAnalyzerFound)
        {
            QTimer::singleShot(100, m_comAnalyzer, SLOT(makeScreenshot()));
        }else if (m_hidAnalyzerFound)
        {
            QTimer::singleShot(100, m_hidAnalyzer, SLOT(makeScreenshot()));
        }
    }
}

void Analyzer::on_hidAnalyzerFound (quint32 analyzerNumber)
{
    if(m_comAnalyzer)
    {
        comAnalyzer* tmp = m_comAnalyzer;
        m_comAnalyzer = nullptr;
        delete tmp;
    }
    m_hidAnalyzerFound = true;
    m_analyzerModel = analyzerNumber;
    QString str = CustomAnalyzer::customized() ? CustomAnalyzer::currentAlias() : names[m_analyzerModel];
    emit analyzerFound(str);

    //if(m_autoCheckUpdate)
//    {
//        QString url = "https://www.rigexpert.com/getfirmware?model=";
//        url += names[m_analyzerModel].toLower().remove(" ").remove("-");
//        url += "&sn=";
//        url += m_hidAnalyzer->getSerial();
//        url += "&revision=";
//        url += "1";
//        m_downloader->startDownloadInfo(QUrl(url));
//    }
}

void Analyzer::on_hidAnalyzerDisconnected ()
{
    if(!m_comAnalyzer)
    {
        m_comAnalyzer = new comAnalyzer(this);
        connect(m_comAnalyzer,SIGNAL(newData(rawData)),this,SLOT(on_newData(rawData)));
        connect(m_comAnalyzer,SIGNAL(analyzerFound(quint32)),this,SLOT(on_comAnalyzerFound(quint32)));
        connect(m_comAnalyzer,SIGNAL(analyzerDisconnected()),this,SLOT(on_comAnalyzerDisconnected()));
        connect(m_comAnalyzer,SIGNAL(analyzerDataStringArrived(QString)),this,SLOT(on_analyzerDataStringArrived(QString)));
        connect(m_comAnalyzer,SIGNAL(analyzerScreenshotDataArrived(QByteArray)),this,SLOT(on_analyzerScreenshotDataArrived(QByteArray)));
        connect(m_comAnalyzer,SIGNAL(updatePercentChanged(int)),this,SLOT(on_updatePercentChanged(int)));
        connect(this, SIGNAL(screenshotComplete()),m_comAnalyzer,SLOT(on_screenshotComplete()));
        connect(this, SIGNAL(measurementComplete()), m_comAnalyzer, SLOT(on_measurementComplete()), Qt::QueuedConnection);
        connect(m_comAnalyzer,SIGNAL(aa30bootFound()),this,SIGNAL(aa30bootFound()));
        connect(m_comAnalyzer, SIGNAL(aa30updateComplete()), this, SIGNAL(aa30updateComplete()));
        connect(m_comAnalyzer, &comAnalyzer::signalFullInfo, this, &Analyzer::slotFullInfo);
    }
    m_hidAnalyzerFound = false;
    m_analyzerModel = 0;
    emit analyzerDisconnected();
}

void Analyzer::on_comAnalyzerFound (quint32 analyzerNumber)
{
    if(m_hidAnalyzer)
    {
        delete m_hidAnalyzer;
        m_hidAnalyzer = nullptr;
    }
    m_comAnalyzerFound = true;
    m_analyzerModel = analyzerNumber;
    QString str = CustomAnalyzer::customized() ? CustomAnalyzer::currentPrototype() : names[m_analyzerModel];
    emit analyzerFound(str);

//    //if(m_autoCheckUpdate)
//    {
//        QString url = "https://www.rigexpert.com/getfirmware?model=";
//        url += names[m_analyzerModel].toLower().remove(" ").remove("-");
//        url += "&sn=";
//        url += m_hidAnalyzer->getSerial();
//        url += "&revision=";
//        url += "1";
//        m_downloader->startDownloadInfo(QUrl(url));
//    }
}

void Analyzer::on_comAnalyzerDisconnected ()
{
    if(!m_hidAnalyzer)
    {
        m_hidAnalyzer = new hidAnalyzer(this);
        connect(m_hidAnalyzer,SIGNAL(newData(rawData)),this,SLOT(on_newData(rawData)));
        connect(m_hidAnalyzer,SIGNAL(analyzerFound(quint32)),this,SLOT(on_hidAnalyzerFound(quint32)));
        connect(m_hidAnalyzer,SIGNAL(analyzerDisconnected()),this,SLOT(on_hidAnalyzerDisconnected()));
        connect(m_hidAnalyzer,SIGNAL(analyzerDataStringArrived(QString)),this,SLOT(on_analyzerDataStringArrived(QString)));
        connect(m_hidAnalyzer,SIGNAL(analyzerScreenshotDataArrived(QByteArray)),this,SLOT(on_analyzerScreenshotDataArrived(QByteArray)));
        connect(this, SIGNAL(screenshotComplete()),m_hidAnalyzer,SLOT(on_screenshotComplete()));
        connect(this, SIGNAL(measurementComplete()), m_hidAnalyzer, SLOT(on_measurementComplete()), Qt::QueuedConnection);
        connect(m_hidAnalyzer, &hidAnalyzer::signalFullInfo, this, &Analyzer::slotFullInfo);
    }
    m_comAnalyzerFound = false;
    m_analyzerModel = 0;
    if(m_comAnalyzer != nullptr)
    {
        m_comAnalyzer->setAnalyzerModel(0);
    }
    emit analyzerDisconnected();
}

void Analyzer::on_newData(rawData _rawData)
{
    if(++m_chartCounter == m_dotsNumber+1 || !m_isMeasuring)
    {
        emit newData (_rawData);
        m_isMeasuring = false;
        m_chartCounter = 0;
        PopUpIndicator::setIndicatorVisible(false);
        if(!m_calibrationMode)
        {
            emit measurementComplete();
        }
    }else
    {
        emit newData (_rawData);
    }
}

void Analyzer::on_analyzerDataStringArrived(QString str)
{
    emit analyzerDataStringArrived(str);
}

void Analyzer::getAnalyzerData()
{
    if(!m_isMeasuring)
    {
        if(m_comAnalyzerFound)
        {
            QTimer::singleShot(100, m_comAnalyzer, SLOT(getAnalyzerData()));
        }else if (m_hidAnalyzerFound)
        {
            QTimer::singleShot(100, m_hidAnalyzer, SLOT(getAnalyzerData()));
        }
    }
}

void Analyzer::on_itemDoubleClick(QString number, QString dotsNumber, QString name)
{
    setIsMeasuring(true);
    if(m_comAnalyzerFound)
    {
        m_chartCounter = 0;
        m_dotsNumber = dotsNumber.toInt();
        emit newMeasurement(name);
        m_comAnalyzer->getAnalyzerData(number);
    }else if (m_hidAnalyzerFound)
    {
        m_chartCounter = 0;
        m_dotsNumber = dotsNumber.toInt();
        emit newMeasurement(name);
        m_hidAnalyzer->getAnalyzerData(number);
    }
}

void Analyzer::on_dialogClosed()
{
    //setIsMeasuring(false);
}

void Analyzer::on_stopMeasuring()
{
    setIsMeasuring(false);
}

void Analyzer::on_analyzerScreenshotDataArrived(QByteArray arr)
{
    emit analyzerScreenshotDataArrived(arr);
}

void Analyzer::on_screenshotComplete(void)
{
    emit screenshotComplete();
}

void Analyzer::on_updatePercentChanged(int number)
{
    if (m_updateDialog != nullptr)
        m_updateDialog->on_percentChanged(number);
    emit updatePercentChanged(number);
}

void Analyzer::on_checkUpdatesBtn_clicked()
{
    if(m_downloader == nullptr)
    {
        m_downloader = new Downloader();
        connect(m_downloader, SIGNAL(downloadInfoComplete()),
                this, SLOT(on_downloadInfoComplete()));
        connect(m_downloader, SIGNAL(downloadFileComplete()),
                this, SLOT(on_downloadFileComplete()));
        connect(m_downloader, SIGNAL(progress(qint64,qint64)),
                this, SLOT(on_progress(qint64,qint64)));
    }
    QString url = "https://www.rigexpert.com/getfirmware?model=";
    QString prototype = CustomAnalyzer::customized() ? CustomAnalyzer::currentPrototype() : names[m_analyzerModel];
    //url += names[m_analyzerModel].toLower().remove(" ").remove("-");
    url += prototype.toLower().remove(" ").remove("-");
    url += "&sn=";
    if(m_hidAnalyzerFound)
    {
        url += m_hidAnalyzer->getSerial();
        url += "&revision=";
        url += m_hidAnalyzer->getRevision();
    }else if (m_comAnalyzerFound)
    {
        url += m_comAnalyzer->getSerial();
        url += "&revision=";
        url += m_comAnalyzer->getRevision();
    }
    if (m_mapFullInfo.contains("MAC"))
    {
        url += "&mac=" + m_mapFullInfo["MAC"];
    }
    if (m_mapFullInfo.contains("SN"))
    {
        url += "&s_n=" + m_mapFullInfo["SN"];
    }
    m_downloader->startDownloadInfo(QUrl(url));
}

void Analyzer::on_progress(qint64 downloaded,qint64 total)
{
    int percent = downloaded*100/total;
    if (percent == 100)
    {
        emit updatePercentChanged(0);
        m_updateDialog->setStatusText(tr("Updating, please wait..."));
    }else
    {
        emit updatePercentChanged(percent);
    }
}

bool Analyzer::openComPort(const QString& portName, quint32 portSpeed)
{
    return m_comAnalyzer->openComPort(portName, portSpeed);
}

void Analyzer::closeComPort()
{
    if(m_comAnalyzer != nullptr)
    {
        m_comAnalyzer->closeComPort();
    }
}

void Analyzer::setAnalyzerModel (int model)
{
    if(m_comAnalyzerFound)
    {
        m_analyzerModel = model;
        m_comAnalyzer->setAnalyzerModel(model);
        m_comAnalyzer->setIsMeasuring(true);
    }
}

void Analyzer::on_measureCalib(int dotsNumber)
{
    m_isMeasuring = true;
    m_dotsNumber = dotsNumber;
    qint64 minFq_ = minFq[m_analyzerModel].toULongLong()*1000;
    qint64 maxFq_ = maxFq[m_analyzerModel].toULongLong()*1000;
    if (CustomAnalyzer::customized()) {
        CustomAnalyzer* ca = CustomAnalyzer::getCurrent();
        if (ca != nullptr) {
            minFq_ = ca->minFq().toULongLong();
            maxFq_ = ca->maxFq().toULongLong();
        }
    }
    if(m_comAnalyzerFound)
    {
        m_comAnalyzer->startMeasure(minFq_, maxFq_, dotsNumber);
    }else if (m_hidAnalyzerFound)
    {
        m_hidAnalyzer->startMeasure(minFq_, maxFq_, dotsNumber);
    }
}

void Analyzer::setCalibrationMode(bool enabled)
{
    m_calibrationMode = enabled;
}

void Analyzer::on_changedAutoDetectMode(bool state)
{
    if(state)
    {
        if(m_hidAnalyzer == nullptr)
        {
            m_hidAnalyzer = new hidAnalyzer(this);
            connect(m_hidAnalyzer,SIGNAL(newData(rawData)),this,SLOT(on_newData(rawData)));
            connect(m_hidAnalyzer,SIGNAL(analyzerFound(quint32)),this,SLOT(on_hidAnalyzerFound(quint32)));
            connect(m_hidAnalyzer,SIGNAL(analyzerDisconnected()),this,SLOT(on_hidAnalyzerDisconnected()));
            connect(m_hidAnalyzer,SIGNAL(analyzerDataStringArrived(QString)),this,SLOT(on_analyzerDataStringArrived(QString)));
            connect(m_hidAnalyzer,SIGNAL(analyzerScreenshotDataArrived(QByteArray)),this,SLOT(on_analyzerScreenshotDataArrived(QByteArray)));
            connect(m_hidAnalyzer,SIGNAL(updatePercentChanged(int)),this,SLOT(on_updatePercentChanged(int)));
            connect(m_hidAnalyzer, &hidAnalyzer::signalFullInfo, this, &Analyzer::slotFullInfo);
        }
    }else
    {
        if(m_hidAnalyzer != nullptr)
        {
            delete m_hidAnalyzer;
            m_hidAnalyzer = nullptr;
        }
    }

    if(m_comAnalyzer == nullptr)
    {
        m_comAnalyzer = new comAnalyzer(this);
        connect(m_comAnalyzer,SIGNAL(newData(rawData)),this,SLOT(on_newData(rawData)));
        connect(m_comAnalyzer,SIGNAL(analyzerFound(quint32)),this,SLOT(on_comAnalyzerFound(quint32)));
        connect(m_comAnalyzer,SIGNAL(analyzerDisconnected()),this,SLOT(on_comAnalyzerDisconnected()));
        connect(m_comAnalyzer,SIGNAL(analyzerDataStringArrived(QString)),this,SLOT(on_analyzerDataStringArrived(QString)));
        connect(m_comAnalyzer,SIGNAL(analyzerScreenshotDataArrived(QByteArray)),this,SLOT(on_analyzerScreenshotDataArrived(QByteArray)));
        connect(m_comAnalyzer,SIGNAL(updatePercentChanged(int)),this,SLOT(on_updatePercentChanged(int)));
        connect(this, SIGNAL(screenshotComplete()),m_comAnalyzer,SLOT(on_screenshotComplete()));
        connect(this, SIGNAL(measurementComplete()), m_comAnalyzer, SLOT(on_measurementComplete()), Qt::QueuedConnection);
        connect(m_comAnalyzer, SIGNAL(aa30bootFound()), this, SIGNAL(aa30bootFound()));
        connect(m_comAnalyzer, SIGNAL(aa30updateComplete()), this, SIGNAL(aa30updateComplete()));
        connect(m_comAnalyzer, &comAnalyzer::signalFullInfo, this, &Analyzer::slotFullInfo);

        QTimer::singleShot(1000, m_comAnalyzer, SLOT(searchAnalyzer()));
    }

    m_comAnalyzer->on_changedAutoDetectMode(state);
}

void Analyzer::on_changedSerialPort(QString portName)
{
    if(m_comAnalyzer != nullptr)
    {
        m_comAnalyzer->on_changedSerialPort(portName);
    }
}

void Analyzer::setIsMeasuring (bool _isMeasuring)
{
    m_isMeasuring = _isMeasuring;
    if(m_comAnalyzer != nullptr)
    {
        m_comAnalyzer->setIsMeasuring(_isMeasuring);
    }
    if(m_hidAnalyzer != nullptr)
    {
        m_hidAnalyzer->setIsMeasuring(_isMeasuring);
    }
    PopUpIndicator::setIndicatorVisible(m_isMeasuring);
}

void Analyzer::slotFullInfo(QString str)
{
    QStringList list = str.split("\t");
    if (list.size() < 2)
        return;
    m_mapFullInfo.insert(list[0], list[1]);
}

void Analyzer::searchAnalyzer()
{
    if (!isMeasuring())
    {
        if (m_hidAnalyzer != nullptr)
            m_hidAnalyzer->searchAnalyzer(true);
        if (m_comAnalyzer != nullptr)
            m_comAnalyzer->searchAnalyzer();
    }
}


