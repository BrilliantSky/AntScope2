#include "measurements.h"


Measurements::Measurements(QObject *parent) : QObject(parent),
    m_currentIndex(0),
    m_graphHint(NULL),
    m_graphBriefHint(NULL),
    m_swrLine(NULL),
    m_swrLine2(NULL),
    m_phaseLine(NULL),
    m_phaseLine2(NULL),
    m_rsLine(NULL),
    m_rpLine(NULL),
    m_rlLine(NULL),
    m_rlLine2(NULL),
    m_tdrLine(NULL),
    m_settings(NULL),
    m_calibration(NULL),
    m_graphHintEnabled(true),
    m_graphBriefHintEnabled(true),
    m_calibrationMode(false),
    m_Z0(50),
    m_dotsNumber(50),
    m_smithTracer(NULL)
{    
    QString path = Settings::setIniFile();
    m_settings = new QSettings(path,QSettings::IniFormat);
    m_settings->beginGroup("Measurements");
    m_graphHintEnabled = m_settings->value("GraphHintEnabled",true).toBool();
    m_graphBriefHintEnabled = m_settings->value("GraphBriefHintEnabled",true).toBool();
    m_settings->endGroup();

    m_settings->beginGroup("Cable");
    m_cableVelFactor = m_settings->value("VelFactor",0.66 ).toDouble();
    m_settings->endGroup();

    m_pdTdrImp =  new double[TDR_MAXARRAY];
    m_pdTdrStep =  new double[TDR_MAXARRAY];

    if(m_graphHint == NULL)
    {
        m_graphHint = new PopUp();
        m_graphHint->setHiding(false);
        m_graphHint->setPopupText(tr("Frequency = \n"
                                     "SWR = \n"
                                     "RL = \n"
                                     "Z = \n"
                                     "|Z| = \n"
                                     "|rho| = \n"
                                     "C = \n"
                                     "Zpar = \n"
                                     "Cpar = \n"
                                     "Cable: "));
        if(m_graphHintEnabled)
        {
            m_graphHint->show();
        }
        m_graphHint->setName(tr("Hint"));
    }

    if(m_graphBriefHint == NULL)
    {
        m_graphBriefHint = new PopUp();
        m_graphBriefHint->setHiding(false);
        m_graphBriefHint->setPopupText("0\n0");
        m_graphBriefHint->setName(tr("BriefHint"));
    }
}

Measurements::~Measurements()
{
    m_settings->beginGroup("Measurements");
    m_settings->setValue("GraphHintEnabled",m_graphHintEnabled);
    m_settings->setValue("GraphBriefHintEnabled",m_graphBriefHintEnabled);
    m_settings->endGroup();

    delete m_pdTdrImp;
    delete m_pdTdrStep;

    if(m_graphHint)
    {
        delete m_graphHint;
    }
    if (m_graphBriefHint)
    {
        delete m_graphBriefHint;
    }
}

void Measurements::setWidgets(QCustomPlot * swr,   QCustomPlot * phase,
                              QCustomPlot * rs,    QCustomPlot * rp,
                              QCustomPlot * rl,    QCustomPlot * tdr,
                              QCustomPlot * smith, QTableWidget * table)
{
    m_swrWidget = swr;
    m_phaseWidget = phase;
    m_rsWidget = rs;
    m_rsWidget->legend->setVisible(true);
    m_rsWidget->legend->removeAt(0);
    m_rpWidget = rp;
    m_rpWidget->legend->setVisible(true);
    m_rpWidget->legend->removeAt(0);
    m_rlWidget = rl;
    m_tdrWidget = tdr;
    m_tdrWidget->legend->setVisible(true);
    m_tdrWidget->legend->removeAt(0);
    m_smithWidget = smith;
    m_tableWidget = table;
    drawSmithImage();

    if(m_graphBriefHint != NULL)
    {
        m_graphBriefHint->setPenColor(QColor(0,0,0,0));
        m_graphBriefHint->setBackgroundColor(QColor(0,0,0,0));
        m_graphBriefHint->setTextColor("black");
    }
}

void Measurements::setCalibration(Calibration * _calibration)
{
    m_calibration = _calibration;
}

bool Measurements::getCalibrationEnabled(void)
{
    return m_calibration->getCalibrationEnabled();
}

void Measurements::deleteRow(int row)
{
    m_tableNames.remove(row, 1);
    m_tableWidget->removeRow(row);

    for(int j = 0; j < 5; ++j)
    {
        if(m_tableNames.length() > j)
        {
            QTableWidgetItem *item = m_tableWidget->item(j,0);
            if(item == NULL)
            {
                item = new QTableWidgetItem();
                QString str = m_tableNames.at(j);
                item->setText(str);
                m_tableWidget->setItem(j,0, item);
            }else
            {
                QString str = m_tableNames.at(j);
                item->setText(str);
            }
        }else
        {
            QTableWidgetItem *item = m_tableWidget->item(j,0);
            if(item != NULL)
            {
                delete item;
            }
        }
    }
    int count = m_swrWidget->graphCount();
    if(count)
    {
        delete m_measurements[m_measurements.length() - 1 - row].smithCurve;
        m_measurements.removeAt(m_measurements.length() - 1 - row);
        m_viewMeasurements.removeAt(m_viewMeasurements.length() - 1 - row);
        m_farEndMeasurementsAdd.removeAt(m_farEndMeasurementsAdd.length() - 1 - row);
        m_farEndMeasurementsSub.removeAt(m_farEndMeasurementsSub.length() - 1 - row);

        m_swrWidget->removeGraph(m_swrWidget->graphCount()-1 - row);
        m_phaseWidget->removeGraph(m_phaseWidget->graphCount()-1 - row);
        m_rsWidget->removeGraph(m_rsWidget->graphCount()-1 - (row*3));
        m_rsWidget->removeGraph(m_rsWidget->graphCount()-1 - (row*3));
        m_rsWidget->removeGraph(m_rsWidget->graphCount()-1 - (row*3));
        m_rpWidget->removeGraph(m_rpWidget->graphCount()-1 - (row*3));
        m_rpWidget->removeGraph(m_rpWidget->graphCount()-1 - (row*3));
        m_rpWidget->removeGraph(m_rpWidget->graphCount()-1 - (row*3));
        m_rlWidget->removeGraph(m_rlWidget->graphCount()-1 - row);
        m_tdrWidget->removeGraph(m_tdrWidget->graphCount()-1 - (row*2));
        m_tdrWidget->removeGraph(m_tdrWidget->graphCount()-1 - (row*2));

        count = m_swrWidget->graphCount();
        if(row == m_tableNames.length())
        {
            QModelIndex myIndex = m_tableWidget->model()->index( row-1, 0,
                                                                 QModelIndex());
            m_tableWidget->selectionModel()->select(myIndex,
                                                    QItemSelectionModel::Select);

            myIndex = m_tableWidget->model()->index( row-1, 1, QModelIndex());
            m_tableWidget->selectionModel()->select(myIndex,
                                                    QItemSelectionModel::Select);

        }else
        {
            QModelIndex myIndex = m_tableWidget->model()->index( row, 0,
                                                                 QModelIndex());
            m_tableWidget->selectionModel()->select(myIndex,
                                                    QItemSelectionModel::Select);

            myIndex = m_tableWidget->model()->index( row, 1, QModelIndex());
            m_tableWidget->selectionModel()->select(myIndex,
                                                    QItemSelectionModel::Select);

        }
    }
    replot();
    m_tableWidget->setRowCount(m_tableNames.length());
}

void Measurements::on_newMeasurement(QString name)
{
    if(m_graphBriefHintEnabled)
    {
        m_graphBriefHint->show();
    }
    m_tableNames.insert(0,name);
    if(m_tableNames.length() == MAX_MEASUREMENTS+1)
    {
        m_tableNames.remove(MAX_MEASUREMENTS,1);
    }
    if(m_tableNames.length() > m_tableWidget->rowCount())
    {
        m_tableWidget->setRowCount(m_tableNames.length());
    }
    for(int i = 0; i < m_tableNames.length(); ++i)
    {
        QTableWidgetItem *item = m_tableWidget->item(i,0);
        if(item == NULL)
        {
            item = new QTableWidgetItem();
            QString str = m_tableNames.at(i);
            item->setText(str);
            m_tableWidget->setItem(i,0, item);
        }else
        {
            QString str = m_tableNames.at(i);
            item->setText(str);
        }
    }

    m_tableWidget->reset();
    QModelIndex myIndex = m_tableWidget->model()->index( 0, 0, QModelIndex());
    m_tableWidget->selectionModel()->select(myIndex,QItemSelectionModel::Select);

    if(m_measurements.length() == MAX_MEASUREMENTS)
    {
        delete m_measurements[m_measurements.length() - MAX_MEASUREMENTS].smithCurve;
        delete m_viewMeasurements[m_measurements.length() - MAX_MEASUREMENTS].smithCurve;
        delete m_farEndMeasurementsAdd[m_farEndMeasurementsAdd.length() - MAX_MEASUREMENTS].smithCurve;
        delete m_farEndMeasurementsSub[m_farEndMeasurementsSub.length() - MAX_MEASUREMENTS].smithCurve;
        m_measurements.removeFirst();
        m_viewMeasurements.removeFirst();
        m_farEndMeasurementsAdd.removeFirst();
        m_farEndMeasurementsSub.removeFirst();
        //------------------------------------
        m_swrWidget->removeGraph(1);
        m_phaseWidget->removeGraph(1);
        m_rsWidget->removeGraph(1);
        m_rsWidget->removeGraph(1);
        m_rsWidget->removeGraph(1);
        m_rpWidget->removeGraph(1);
        m_rpWidget->removeGraph(1);
        m_rpWidget->removeGraph(1);
        m_rlWidget->removeGraph(1);
        m_tdrWidget->removeGraph(1);
        m_tdrWidget->removeGraph(1);
    }
    m_measurements.append( measurement());
    m_viewMeasurements.append( measurement());
    m_farEndMeasurementsAdd.append( measurement());
    m_farEndMeasurementsSub.append( measurement());

    QPen pen;
    if(m_swrWidget->graphCount() > 1)
    {
        pen = m_swrWidget->graph()->pen();
        pen.setWidth(3);
        m_swrWidget->graph()->setPen(pen);
        m_phaseWidget->graph()->setPen(pen);
        m_rlWidget->graph()->setPen(pen);
        m_smithWidget->graph()->setPen(pen);
        m_measurements.at(m_measurements.length()-2).smithCurve->setPen(pen);
    }
    m_swrWidget->addGraph();
    m_swrWidget->graph()->setAntialiasedFill(false);
    m_phaseWidget->addGraph();
    m_rsWidget->addGraph();
    m_rsWidget->graph()->setName("R");
    m_rsWidget->addGraph();
    m_rsWidget->graph()->setName("X");
    m_rsWidget->addGraph();
    m_rsWidget->graph()->setName("|Z|");
    m_rpWidget->addGraph();
    m_rpWidget->graph()->setName("Rp");
    m_rpWidget->addGraph();
    m_rpWidget->graph()->setName("Xp");
    m_rpWidget->addGraph();
    m_rpWidget->graph()->setName("|Zp|");
    m_rlWidget->addGraph();
    m_tdrWidget->addGraph();
    m_tdrWidget->graph()->setName(tr("Impulse response"));
    m_tdrWidget->addGraph();
    m_tdrWidget->graph()->setName(tr("Step response"));
    m_measurements.last().smithCurve = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    m_viewMeasurements.last().smithCurve = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    m_farEndMeasurementsAdd.last().smithCurve = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    m_farEndMeasurementsSub.last().smithCurve = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);

    if(++m_currentIndex == MAX_MEASUREMENTS+1)
    {
        m_currentIndex = 1;
    }
    switch (m_currentIndex) {
    case 1:
        pen.setColor(QColor(30, 40, 255, 150));
        break;
    case 2:
        pen.setColor(QColor(30, 255, 40, 150));
        break;
    case 3:
        pen.setColor(QColor(255, 30, 40, 150));
        break;
    case 4:
        pen.setColor(QColor(255, 255, 40, 255));
        break;
    case 5:
        pen.setColor(QColor(255, 40, 255, 150));
        break;
    default:
        break;
    }
    pen.setWidth(5);

    m_swrWidget->setBackgroundScaled(true);

    m_swrWidget->graph()->setPen(pen);
    m_phaseWidget->graph()->setPen(pen);
    m_rlWidget->graph()->setPen(pen);
    m_smithWidget->graph()->setPen(pen);
    m_measurements.last().smithCurve->setPen(pen);

    int rsGraphCount = m_rsWidget->graphCount();
    int tdrGraphCount = m_tdrWidget->graphCount();

    QPen rpen;
    rpen.setColor(QColor(30, 40, 255, 150));
    rpen.setWidthF(3);
    QPen gpen;
    gpen.setColor(QColor(30, 255, 40, 150));
    gpen.setWidthF(3);
    QPen bpen;
    bpen.setColor(QColor(255, 30, 40, 150));
    bpen.setWidthF(3);

    m_rsWidget->graph(rsGraphCount-3)->setPen(bpen);
    m_rsWidget->graph(rsGraphCount-2)->setPen(gpen);
    m_rsWidget->graph(rsGraphCount-1)->setPen(rpen);

    m_rpWidget->graph(rsGraphCount-3)->setPen(bpen);
    m_rpWidget->graph(rsGraphCount-2)->setPen(gpen);
    m_rpWidget->graph(rsGraphCount-1)->setPen(rpen);

    m_tdrWidget->graph(tdrGraphCount-2)->setPen(gpen);
    m_tdrWidget->graph(tdrGraphCount-1)->setPen(rpen);
}

void Measurements::on_newData(rawData _rawData)
{
    if(m_calibrationMode)
    {
        return;
    }

    m_measurements.last().dataRX.append(_rawData);

    double VSWR;
    double RL;
    if(!computeSWR(_rawData.fq, m_Z0,_rawData.r,_rawData.x,&VSWR,&RL))
    {
        if(m_measurements.last().swrGraph.size() > 0)
        {
            VSWR = m_measurements.last().swrGraph.last().value;
            RL = m_measurements.last().rlGraph.last().value;
        }else
        {
            return;
        }
    }
    double maxSwr = m_swrWidget->yAxis->range().upper;
    double maxRs = m_rsWidget->yAxis->range().upper;
    double maxRp = m_rpWidget->yAxis->range().upper;

    QVector <double> x,y;
    double fq = _rawData.fq*1000;
    x.append(fq);
    x.append(fq);
    y.append(0);
    y.append(MAX_SWR);

    QCPData data;
    data.key = fq;
    data.value = VSWR;
    m_measurements.last().swrGraph.insert(data.key,data);

    m_swrWidget->graph(0)->setData(x,y);

    y.clear();
    y.append(m_phaseWidget->yAxis->getRangeLower());
    y.append(m_phaseWidget->yAxis->getRangeUpper());
    m_phaseWidget->graph(0)->setData(x,y);

    y.clear();
    y.append(m_rsWidget->yAxis->getRangeLower());
    y.append(m_rsWidget->yAxis->getRangeUpper());
    m_rsWidget->graph(0)->setData(x,y);

    y.clear();
    y.append(m_rpWidget->yAxis->getRangeLower());
    y.append(m_rpWidget->yAxis->getRangeUpper());
    m_rpWidget->graph(0)->setData(x,y);

    y.clear();
    y.append(m_rlWidget->yAxis->getRangeLower());
    y.append(m_rlWidget->yAxis->getRangeUpper());
    m_rlWidget->graph(0)->setData(x,y);
//------------------------------------------------------------------------------
//------------------RXZ---------------------------------------------------------
//------------------------------------------------------------------------------
    double R = _rawData.r;
    double X = _rawData.x;
    double Z = computeZ(R, X);

    data.value = R;
    m_measurements.last().rsrGraph.insert(data.key,data);
    if( R > maxRs )
    {
        data.value = maxRs;
    }else if( R < (-maxRs) )
    {
        data.value = -maxRs;
    }
    m_viewMeasurements.last().rsrGraph.insert(data.key,data);

    data.value = X;
    m_measurements.last().rsxGraph.insert(data.key,data);
    if( X > maxRs )
    {
        data.value = maxRs;
    }else if( X < (-maxRs) )
    {
        data.value = -maxRs;
    }
    m_viewMeasurements.last().rsxGraph.insert(data.key,data);

    data.value = Z;
    m_measurements.last().rszGraph.insert(data.key,data);
    if( Z > maxRs )
    {
        data.value = maxRs;
    }else if( Z < (-maxRs) )
    {
        data.value = -maxRs;
    }
    m_viewMeasurements.last().rszGraph.insert(data.key,data);
//------------------------------------------------------------------------------
//------------------RXZ par-----------------------------------------------------
//------------------------------------------------------------------------------
    if (_isnan(R) || (R<0.001) )
    {
        R = 0.01;
    }
    if (_isnan(X))
    {
        X = 0;
    }
    double Rpar = R*(1+X*X/R/R);
    double Xpar = X*(1+R*R/X/X);
    double Zpar = computeZ(R, X);

    data.value = Rpar;
    m_measurements.last().rprGraph.insert(data.key,data);
    if( Rpar > maxRp )
    {
        data.value = maxRp;
    }else if( Rpar < (-maxRp) )
    {
        data.value = -maxRp;
    }
    m_viewMeasurements.last().rprGraph.insert(data.key,data);

    data.value = Xpar;
    m_measurements.last().rpxGraph.insert(data.key,data);
    if( Xpar > maxRp )
    {
        data.value = maxRp;
    }else if( Xpar < (-maxRp) )
    {
        data.value = -maxRp;
    }
    m_viewMeasurements.last().rpxGraph.insert(data.key,data);

    data.value = Zpar;
    m_measurements.last().rpzGraph.insert(data.key,data);
    if( Zpar > maxRp )
    {
        data.value = maxRp;
    }else if( Zpar < (-maxRp) )
    {
        data.value = -maxRp;
    }
    m_viewMeasurements.last().rpzGraph.insert(data.key,data);

    data.value = RL;
    m_measurements.last().rlGraph.insert(data.key,data);

//------------------------------------------------------------------------------
//----------------------calc phase----------------------------------------------
//------------------------------------------------------------------------------

    if (_isnan(_rawData.r) || (_rawData.r<0.001) )
    {
        _rawData.r = 0.01;
    }
    if (_isnan(_rawData.x))
    {
        _rawData.x = 0;
    }
    double Rnorm = _rawData.r/m_Z0;
    double Xnorm = _rawData.x/m_Z0;
    double Denom = (Rnorm+1)*(Rnorm+1)+Xnorm*Xnorm;
    double RhoReal = ((Rnorm-1)*(Rnorm+1)+Xnorm*Xnorm)/Denom;
    double RhoImag = 2*Xnorm/Denom;
    double RhoPhase = atan2(RhoImag, RhoReal) / M_PI * 180.0;
    double RhoMod = sqrt(RhoReal*RhoReal+RhoImag*RhoImag);
    data.value = RhoPhase;
    m_measurements.last().phaseGraph.insert(data.key,data);
    data.value = RhoMod;
    m_measurements.last().rhoGraph.insert(data.key,data);
//------------------------------------------------------------------------------
//----------------------calc smith----------------------------------------------
//------------------------------------------------------------------------------
    double pointX,pointY;
    NormRXtoSmithPoint(R/m_Z0, X/m_Z0, pointX, pointY);
    double len = m_measurements.last().dataRX.length();
    m_measurements.last().smithGraph.insert(len, QCPCurveData(len, pointX, pointY));
    len = m_measurements.last().dataRX.length()*2 - 1;
    m_measurements.last().smithGraphView.insert(len, QCPCurveData(len, pointX, pointY));

//------------------------------------------------------------------------------
//----------------------Calc calibration if performed---------------------------
//------------------------------------------------------------------------------
    if(m_calibration != NULL)
    {
        if(m_calibration->getCalibrationPerformed())
        {
            R = _rawData.r;
            X = _rawData.x;
            double Gre = (R*R-m_Z0*m_Z0+X*X)/((R+m_Z0)*(R+m_Z0)+X*X);
            double Gim = (2*m_Z0*X)/((R+m_Z0)*(R+m_Z0)+X*X);

            double GreOut;
            double GimOut;

            double SOR = 1; double SOI = 0; // Ideal model
            double SSR = -1; double SSI = 0;
            double SLR = 0; double SLI = 0;

            double CRO, CIO; // CalibrationReOpen, CalibrationImOpen
            double CRS, CIS; // CalibrationReShort, CalibrationImShort
            double CRL, CIL; // CalibrationReLoad, CalibrationImLoad
            bool res = m_calibration->interpolateS(_rawData.fq/1000, CRO, CIO, CRS, CIS, CRL, CIL);

            if (!res)
            {
                qDebug() << "Interpolation Error";
                goto ret;
            }
            m_calibration->applyCalibration(Gre,Gim,  // Measured
                                            CRO,CIO,CRS,CIS,CRL,CIL, // Measured parameters of cal standards
                                            SOR,SOI,SSR,SSI,SLR,SLI, // Actual (Ideal) parameters of cal standards
                                            GreOut,GimOut); // Actual

            double calR = (1-GreOut*GreOut-GimOut*GimOut)/((1-GreOut)*(1-GreOut)+GimOut*GimOut);
            calR *= m_Z0;
            double calX = (2*GimOut)/((1-GreOut)*(1-GreOut)+GimOut*GimOut);
            calX *= m_Z0;
            double calZ = computeZ(calR,calX);

            rawData rawDataCalib = _rawData;
            rawDataCalib.r = calR;
            rawDataCalib.x = calX;
            m_measurements.last().dataRXCalib.append(rawDataCalib);
            computeSWR(_rawData.fq, m_Z0, calR, calX,&VSWR,&RL);

            data.value = VSWR;
            m_measurements.last().swrGraphCalib.insert(data.key,data);
            if( VSWR > maxSwr )
            {
                data.value = maxSwr;
            }
            m_viewMeasurements.last().swrGraphCalib.insert(data.key,data);

            data.value = calR;
            m_measurements.last().rsrGraphCalib.insert(data.key,data);
            if( calR > maxRs )
            {
                data.value = maxRs;
            }else if( calR < (-maxRs) )
            {
                data.value = -maxRs;
            }
            m_viewMeasurements.last().rsrGraphCalib.insert(data.key,data);

            data.value = calX;
            m_measurements.last().rsxGraphCalib.insert(data.key,data);
            if( calX > maxRs )
            {
                data.value = maxRs;
            }else if( calX < (-maxRs) )
            {
                data.value = -maxRs;
            }
            m_viewMeasurements.last().rsxGraphCalib.insert(data.key,data);

            data.value = calZ;
            m_measurements.last().rszGraphCalib.insert(data.key,data);
            if( calZ > maxRs )
            {
                data.value = maxRs;
            }else if( calZ < (-maxRs) )
            {
                data.value = -maxRs;
            }
            m_viewMeasurements.last().rszGraphCalib.insert(data.key,data);


            double calRpar = calR*(1+calX*calX/calR/calR);
            double calZpar = computeZ(calRpar, calX);
            data.value = calRpar;
            m_measurements.last().rprGraphCalib.insert(data.key,data);
            if( calRpar > maxRp )
            {
                data.value = maxRp;
            }else if( calRpar < (-maxRp) )
            {
                data.value = -maxRp;
            }
            m_viewMeasurements.last().rprGraphCalib.insert(data.key,data);

            data.value = calX;
            m_measurements.last().rpxGraphCalib.insert(data.key,data);
            if( calX > maxRp )
            {
                data.value = maxRp;
            }else if( calX < (-maxRp) )
            {
                data.value = -maxRp;
            }
            m_viewMeasurements.last().rpxGraphCalib.insert(data.key,data);

            data.value = calZpar;
            m_measurements.last().rpzGraphCalib.insert(data.key,data);
            if( calZpar > maxRp )
            {
                data.value = maxRp;
            }else if( calZpar < (-maxRp) )
            {
                data.value = -maxRp;
            }
            m_viewMeasurements.last().rpzGraphCalib.insert(data.key,data);

            data.value = RL;
            m_measurements.last().rlGraphCalib.insert(data.key,data);

            //----------------------calc phase---------------------------
            if (_isnan(calR) || (calR<0.001) )
            {
                calR = 0.01;
            }
            if (_isnan(calX))
            {
                calX = 0;
            }
            Rnorm = calR/m_Z0;
            Xnorm = calX/m_Z0;

            Denom = (Rnorm+1)*(Rnorm+1)+Xnorm*Xnorm;
            RhoReal = ((Rnorm-1)*(Rnorm+1)+Xnorm*Xnorm)/Denom;
            RhoImag = 2*Xnorm/Denom;

            RhoPhase = atan2(RhoImag, RhoReal) / M_PI * 180.0;            
            RhoMod = sqrt(RhoReal*RhoReal+RhoImag*RhoImag);

            data.value = RhoPhase;
            m_measurements.last().phaseGraphCalib.insert(data.key,data);
            data.value = RhoMod;
            m_measurements.last().rhoGraphCalib.insert(data.key,data);
            //----------------------calc phase end---------------------------
            //----------------------calc smith-------------------------------

            double pointX,pointY;
            NormRXtoSmithPoint(R/m_Z0, X/m_Z0, pointX, pointY);
            int len = m_measurements.last().dataRX.length();
            m_measurements.last().smithGraphCalib.insert(len, QCPCurveData(len, pointX, pointY));
            //----------------------calc smith end---------------------------
        }
    }
ret:
    on_redrawGraphs();
}

quint32 Measurements::computeSWR(double freq, double Z0, double R, double X, double *VSWR, double *RL)
{
    Q_UNUSED(freq);

    if (R <= 0)
    {
        R = 0.001;
    }
    double SWR, Gamma;
    double XX = X * X;								// always >= 0
    double denominator = (R + Z0) * (R + Z0) + XX;

    if (denominator == 0)
    {
        return 0;
    }
    Gamma = sqrt(((R - Z0) * (R - Z0) + XX) / denominator);
    if (Gamma == 1.0)
    {
        return 0;
    }
    SWR = (1 + Gamma) / (1 - Gamma);

    if ((SWR > 200) || (Gamma > 0.99))
    {
        SWR = 200;
    } else if (SWR < 1)
    {
        SWR = 1;
    }
    if (VSWR)
    {
        *VSWR = SWR;
    }
    if (RL)
    {
        if (Gamma == 0)
        {
            return 0;
        }
        *RL = -20 * log10(Gamma);
    }
    return 1;
}

double Measurements::computeZ (double R, double X)
{
    return sqrt((R*R) + (X*X));
}

void Measurements::on_currentTab(QString name)
{
    m_currentTab = name;
    on_redrawGraphs();
}

void Measurements::setGraphHintEnabled(bool enabled)
{
    m_graphHintEnabled = enabled;
    showHideHints();
}

void Measurements::setGraphBriefHintEnabled(bool enabled)
{
    m_graphBriefHintEnabled = enabled;
    showHideHints();
}

void Measurements::on_focus(bool focus)
{
    m_focus = focus;
    showHideHints();
}

void Measurements::hideGraphBriefHint()
{
    if(m_graphBriefHint)
    {
        m_graphBriefHint->focusHide();
    }
}

void Measurements::showHideHints()
{
    if(m_graphHint)
    {
        if(m_graphHintEnabled && m_focus)
        {
            m_graphHint->focusShow();
        }else
        {
            m_graphHint->focusHide();
        }
    }
    if(m_graphBriefHint)
    {
        if(m_graphBriefHintEnabled && m_focus)
        {
            m_graphBriefHint->focusShow();
        }else
        {
            m_graphBriefHint->focusHide();
        }
    }
}

void Measurements::on_newCursorFq(double x, int number, int mouseX, int mouseY)
{
    updatePopUp(x, number, mouseX, mouseY);
}

void Measurements::on_newCursorSmithPos (double x, double y, int number)
{
    int len = m_measurements.length()-1;
    QCPCurveDataMap map;
    if((m_calibration != NULL) && (m_calibration->getCalibrationEnabled()))
    {
        map = m_measurements.at(len - number).smithGraphCalib;
    }else
    {
        map = m_measurements.at(len - number).smithGraph;
    }
    QList <QCPCurveData> dataList = map.values();
    if(dataList.isEmpty())
    {
        return;
    }
    int findedNum = 0;
    bool finded = false;
    for(double n = 0; n < 6; n+=0.1)
    {
        for(int i = 0; i < dataList.length(); ++i)
        {
            if( ((x >= dataList.at(i).key - n) && (x <= dataList.at(i).key + n)) &&
                 (y >= dataList.at(i).value - n) && (y <= dataList.at(i).value + n))
            {
                findedNum = i;
                finded = true;
                break;
            }
        }
        if(finded)
        {
            break;
        }
    }
    if(m_smithTracer == NULL)
    {
        m_smithTracer = new QCPItemEllipse(m_smithWidget);
    }
    m_smithTracer->setAntialiased(true);
    QPen pen;
    pen.setColor(QColor(250,30,20,180));
    pen.setWidth(4);
    m_smithTracer->setPen(pen);
    m_smithTracer->topLeft->setCoords(dataList.at(findedNum).key-0.1, dataList.at(findedNum).value+0.1);
    m_smithTracer->bottomRight->setCoords(dataList.at(findedNum).key+0.1, dataList.at(findedNum).value-0.1);
    m_smithWidget->replot();



    QCPDataMap *swrmap;
    if((m_calibration != NULL) && (m_calibration->getCalibrationEnabled()))
    {
        swrmap = &(m_measurements[len - number].swrGraphCalib);
    }else
    {
        swrmap = &(m_measurements[len - number].swrGraph);
    }
    QList <double> swrkeys = swrmap->keys();

    double frequency = 0;
    double swr = 0;
    double phase = 0;
    double rho = 0;
    double rl = 0;
    double l = 0;
    double c = 0;
    double lpar = 0;
    double cpar = 0;
    double r = 0;
    double x1 = 0;
    double z = 0;
    double rpar = 0;
    double xpar = 0;
    QString zString;
    QString zparString;

    frequency = swrkeys.at(findedNum);

    if(m_calibration->getCalibrationEnabled())
    {
        if(m_farEndMeasurement == 1)
        {
            rho = m_farEndMeasurementsSub.at(len - number).rhoGraph.value(frequency).value;
            phase = m_farEndMeasurementsSub.at(len - number).phaseGraph.value(frequency).value;
            r = m_farEndMeasurementsSub.at(len - number).rsrGraph.value(frequency).value;//dataRX.at(previousI).r;
            x1 = m_farEndMeasurementsSub.at(len - number).rsxGraph.value(frequency).value;//dataRX.at(previousI).x;
        }else if(m_farEndMeasurement == 2)
        {
            rho = m_farEndMeasurementsAdd.at(len - number).rhoGraph.value(frequency).value;
            phase = m_farEndMeasurementsAdd.at(len - number).phaseGraph.value(frequency).value;
            r = m_farEndMeasurementsAdd.at(len - number).rsrGraph.value(frequency).value;//dataRX.at(previousI).r;
            x1 = m_farEndMeasurementsAdd.at(len - number).rsxGraph.value(frequency).value;//dataRX.at(previousI).x;
        }else
        {
            rho = m_measurements.at(len - number).rhoGraph.value(frequency).value;
            phase = m_measurements.at(len - number).phaseGraphCalib.value(frequency).value;
            r = m_measurements.at(len - number).dataRXCalib.at(findedNum).r;
            x1 = m_measurements.at(len - number).dataRXCalib.at(findedNum).x;
        }
        swr = m_measurements.at(len - number).swrGraphCalib.value(frequency).value;
        rl = m_measurements.at(len - number).rlGraphCalib.value(frequency).value;
        z = m_viewMeasurements.at(len - number).rszGraph.value(frequency).value;
        rpar = m_viewMeasurements.at(len - number).rprGraph.value(frequency).value;
        xpar = m_viewMeasurements.at(len - number).rpxGraph.value(frequency).value;
    }else
    {
        if(m_farEndMeasurement == 1)
        {
            rho = m_farEndMeasurementsSub.at(len - number).rhoGraph.value(frequency).value;
            phase = m_farEndMeasurementsSub.at(len - number).phaseGraph.value(frequency).value;
            r = m_farEndMeasurementsSub.at(len - number).rsrGraph.value(frequency).value;//dataRX.at(previousI).r;
            x1 = m_farEndMeasurementsSub.at(len - number).rsxGraph.value(frequency).value;//dataRX.at(previousI).x;
        }else if(m_farEndMeasurement == 2)
        {
            rho = m_farEndMeasurementsAdd.at(len - number).rhoGraph.value(frequency).value;
            phase = m_farEndMeasurementsAdd.at(len - number).phaseGraph.value(frequency).value;
            r = m_farEndMeasurementsAdd.at(len - number).rsrGraph.value(frequency).value;//dataRX.at(previousI).r;
            x1 = m_farEndMeasurementsAdd.at(len - number).rsxGraph.value(frequency).value;//dataRX.at(previousI).x;
        }else
        {
            rho = m_measurements.at(len - number).rhoGraph.value(frequency).value;
            phase = m_measurements.at(len - number).phaseGraph.value(frequency).value;
            r = m_measurements.at(len - number).dataRX.at(findedNum).r;
            x1 = m_measurements.at(len - number).dataRX.at(findedNum).x;
        }
        swr = m_measurements.at(len - number).swrGraph.value(frequency).value;
        rl = m_measurements.at(len - number).rlGraph.value(frequency).value;
        z = m_viewMeasurements.at(len - number).rszGraph.value(frequency).value;
        rpar = m_viewMeasurements.at(len - number).rprGraph.value(frequency).value;
        xpar = m_viewMeasurements.at(len - number).rpxGraph.value(frequency).value;
    }

    zString+= QString::number(r,'f', 2);
    if(x1 >= 0)
    {
        zString+= " + j";
        zString+= QString::number(x1,'f', 2);
    }else
    {
        zString+= " - j";
        zString+= QString::number((x1 * (-1)),'f', 2);
    }
    zparString+= QString::number(rpar,'f', 2);
    if(x1 >= 0)
    {
        zparString+= " + j";
        zparString+= QString::number(xpar,'f', 2);
    }else
    {
        zparString+= " - j";
        zparString+= QString::number((xpar * (-1)),'f', 2);
    }
    l = 1E9 * x1 / (2*M_PI * frequency * 1E3);//nH
    c = 1E12 / (2*M_PI * frequency * (x1 * (-1)) * 1E3);//pF

    lpar = 1E9 * xpar / (2*M_PI * frequency * 1E3);//nH
    cpar = 1E12 / (2*M_PI * frequency * (xpar * (-1)) * 1E3);//pF

    QString str = QString::number(frequency);
    int index = str.indexOf(".");
    if(index < 0)
    {
        if(str.length() > 6)
        {
            str.insert(str.length()-6," ");
        }
        if(str.length() > 3)
        {
            str.insert(str.length()-3," ");
        }
    }else
    {
        int len = str.length() - index;
        if((str.length()-len) > 6)
        {
            str.insert(str.length()-len-6," ");
        }
        if((str.length()-len) > 3)
        {
            str.insert(str.length()-len-3," ");
        }
    }


    QString text;
    if(x1 > 0)
    {
        text = QString(tr("Frequency = %1 kHz\n"
                          "SWR = %2\n"
                          "RL = %3 dB\n"
                          "Z = %4 Ohm\n"
                          "|Z| = %5 Ohm\n"
                          "|rho| = %6, phase = %7 °\n"
                          "L = %8 nH\n"
                          "Zpar = %9 Ohm\n"
                          "Lpar = %10 nH\n"))
        .arg(str)
        .arg(QString::number(swr,'f', 2))
        .arg(QString::number(rl,'f', 2))
        .arg(zString)
        .arg(QString::number(z,'f', 2))
        .arg(QString::number(rho,'f', 2))
        .arg(QString::number(phase,'f', 2))
        .arg(QString::number(l,'f', 2))
        .arg(zparString)
        .arg(QString::number(lpar,'f', 2));
    }else
    {
        text = QString(tr("Frequency = %1 kHz\n"
                          "SWR = %2\n"
                          "RL = %3 dB\n"
                          "Z = %4 Ohm\n"
                          "|Z| = %5 Ohm\n"
                          "|rho| = %6, phase = %7 °\n"
                          "C = %8 pF\n"
                          "Zpar = %9 Ohm\n"
                          "Cpar = %10 pF\n"))
        .arg(str)
        .arg(QString::number(swr,'f', 2))
        .arg(QString::number(rl,'f', 2))
        .arg(zString)
        .arg(QString::number(z,'f', 2))
        .arg(QString::number(rho,'f', 2))
        .arg(QString::number(phase,'f', 2))
        .arg(QString::number(c,'f', 2))
        .arg(zparString)
        .arg(QString::number(cpar,'f', 2));
    }

    if(!m_farEndMeasurement)
    {
        QString cableString;
        QString lenUnits;
        if(m_measureSystemMetric)
        {
            lenUnits = tr("m");
        }else
        {
            lenUnits = tr("ft");
        }

        double len14 = SPEEDOFLIGHT/4/frequency/1000*m_cableVelFactor;
        double len12 = SPEEDOFLIGHT/2/frequency/1000*m_cableVelFactor;

        if(!m_measureSystemMetric)
        {
            len14 *= FEETINMETER;
            len12 *= FEETINMETER;
        }

        cableString = tr("Cable: length(1/4) = %1 %2, length(1/2) = %3 %4")
                .arg(QString::number(len14,'f',2))
                .arg(lenUnits)
                .arg(QString::number(len12,'f',2))
                .arg(lenUnits);
        text += cableString;
    }

    m_graphHint->setPopupText(text);
}

void Measurements::updatePopUp(double xPos, int number, int mouseX, int mouseY)
{
#define DELTA 5
    static int previousI = 0;
    if(m_graphHint)
    {
        if(m_currentTab == "tab_6")
        {
            QCPDataMap *tdrmap;
            double pdTdrImp;
            double pdTdrStep;
            int len = m_measurements.length()-1;
            if(m_farEndMeasurement == 1)
            {
                if(m_measureSystemMetric)
                {
                    tdrmap = &(m_farEndMeasurementsSub[len - number].tdrImpGraph);
                }else
                {
                    tdrmap = &(m_farEndMeasurementsSub[len - number].tdrImpGraphFeet);
                }
            }else if(m_farEndMeasurement == 2)
            {
                if(m_measureSystemMetric)
                {
                    tdrmap = &(m_farEndMeasurementsAdd[len - number].tdrImpGraph);
                }else
                {
                    tdrmap = &(m_farEndMeasurementsAdd[len - number].tdrImpGraphFeet);
                }
            }else
            {
                if(m_measureSystemMetric)
                {
                    tdrmap = &(m_measurements[len - number].tdrImpGraph);
                }else
                {
                    tdrmap = &(m_measurements[len - number].tdrImpGraphFeet);
                }
            }

            QList <double> tdrkeys = tdrmap->keys();

            bool res = false;
            int start = previousI-DELTA;
            if(start < 0)
            {
                start = 0;
            }
            int stop = previousI+DELTA;
            if (stop > tdrkeys.length()-1)
            {
                stop = tdrkeys.length()-1;
            }
            for(int t = 0; t < 2; ++t)
            {
                if(t == 1)
                {
                    if(res)
                    {
                        break;
                    }
                    start = 0;
                    stop = tdrkeys.length()-1;
                }
                double place;
                for(int i = start; i < stop; ++i)
                {
                    if((tdrkeys.at(i) <= xPos) && (tdrkeys.at(i+1) >= xPos))
                    {
                        double center = (tdrkeys.at(i) + tdrkeys.at(i+1))/2;
                        if( xPos > center )
                        {
                            if(previousI == i+1)
                            {
                                return;
                            }
                            place = tdrkeys.at(i+1);
                            previousI = i+1;
                        }else
                        {
                            if(previousI == i)
                            {
                                return;
                            }
                            place = tdrkeys.at(i);
                            previousI = i;
                        }

                        if(m_farEndMeasurement == 1)
                        {
                            pdTdrImp = m_farEndMeasurementsSub[len - number].tdrImpGraph.value(place).value;
                            pdTdrStep = m_farEndMeasurementsSub[len - number].tdrStepGraph.value(place).value;
                        }else if(m_farEndMeasurement == 2)
                        {
                            pdTdrImp = m_farEndMeasurementsAdd[len - number].tdrImpGraph.value(place).value;
                            pdTdrStep = m_farEndMeasurementsAdd[len - number].tdrStepGraph.value(place).value;
                        }else
                        {
                            pdTdrImp = m_measurements[len - number].tdrImpGraph.value(place).value;
                            pdTdrStep = m_measurements[len - number].tdrStepGraph.value(place).value;
                        }

                        if(!m_tdrLine)
                        {
                            m_tdrLine = new QCPItemStraightLine(m_tdrWidget);
                            m_tdrLine->setAntialiased(false);
                            m_tdrWidget->addItem(m_tdrLine);
                        }
                        m_tdrLine->point1->setCoords(place, -1);
                        m_tdrLine->point2->setCoords(place, 1);

                        double Z = m_Z0*(1+pdTdrStep)/(1-pdTdrStep);
                        if (Z<0)
                        {
                            Z = 0;
                        }

                        QString text;
                        QString distance;

                        QString lenUnits;
                        QString timeNs;
                        distance = QString::number(place,'f',3);
                        double airLen = place/m_cableVelFactor;
                        QString distanceInAir = QString::number(airLen,'f',3);//FEETINMETER
                        if(m_measureSystemMetric)
                        {
                            lenUnits = "m";
                            timeNs = QString::number(airLen/0.299792458,'f',2);
                        }else
                        {
                            lenUnits = "ft";
                            timeNs = QString::number(airLen*FEETINMETER/0.299792458,'f',2);
                        }


                        QString ir = QString::number(pdTdrImp,'f',3);
                        QString sr = QString::number(pdTdrStep,'f',3);

                        QString zStr = QString::number(Z,'f',1);
                        text = QString(tr("Distance = %1 %2\n"
                                          "(distance in the air = %3 %4)\n"
                                          "Time = %5 ns\n"
                                          "Impulse response = %6\n"
                                          "Step response = %7\n"
                                          "|Z| = %8 Ohm"))
                                .arg(distance)//1
                                .arg(lenUnits)//2
                                .arg(distanceInAir)//3
                                .arg(lenUnits)//4
                                .arg(timeNs)//5
                                .arg(ir)//6
                                .arg(sr)//7
                                .arg(zStr);//8

                        m_graphHint->setPopupText(text);

                        res = true;
                        break;
                    }
                }
            }
        }else
        {
            if(m_graphBriefHint != NULL)
            {
                m_graphBriefHint->setPosition(mouseX+1,mouseY+1);
            }

            QCPDataMap *swrmap;
            int len = m_measurements.length()-1;
            if((m_calibration != NULL) && (m_calibration->getCalibrationEnabled()))
            {
                swrmap = &(m_measurements[len - number].swrGraphCalib);
            }else
            {
                swrmap = &(m_measurements[len - number].swrGraph);
            }
            QList <double> swrkeys = swrmap->keys();

            double frequency = 0;
            double swr = 0;
            double phase = 0;
            double rho = 0;
            double rl = 0;
            double l = 0;
            double c = 0;
            double lpar = 0;
            double cpar = 0;
            double r = 0;
            double z = 0;
            double x = 0;
            double rpar = 0;
            double xpar = 0;
            QString zString;
            QString zparString;
            bool res = false;
            int start = previousI-DELTA;
            if(start < 0)
            {
                start = 0;
            }
            int stop = previousI+DELTA;
            if (stop > swrkeys.length()-1)
            {
                stop = swrkeys.length()-1;
            }
            for(int t = 0; t < 2; ++t)
            {
                if(t == 1)
                {
                    if(res)
                    {
                        break;
                    }
                    start = 0;
                    stop = swrkeys.length()-1;
                }
                for(int i = start; i < stop; ++i)
                {
                    if((swrkeys.at(i) <= xPos) && (swrkeys.at(i+1) >= xPos))
                    {
                        double center = (swrkeys.at(i) + swrkeys.at(i+1))/2;
                        if( xPos > center )
                        {
                            if(previousI == i+1)
                            {
                                return;
                            }
                            frequency = swrkeys.at(i+1);
                            previousI = i+1;
                        }else
                        {
                            if(previousI == i)
                            {
                                return;
                            }
                            frequency = swrkeys.at(i);
                            previousI = i;
                        }

                        if(m_calibration->getCalibrationEnabled())
                        {
                            if(m_farEndMeasurement == 1)
                            {
                                rho = m_farEndMeasurementsSub.at(len - number).rhoGraph.value(frequency).value;
                                phase = m_farEndMeasurementsSub.at(len - number).phaseGraph.value(frequency).value;
                                r = m_farEndMeasurementsSub.at(len - number).rsrGraph.value(frequency).value;
                                x = m_farEndMeasurementsSub.at(len - number).rsxGraph.value(frequency).value;
                            }else if(m_farEndMeasurement == 2)
                            {
                                rho = m_farEndMeasurementsAdd.at(len - number).rhoGraph.value(frequency).value;
                                phase = m_farEndMeasurementsAdd.at(len - number).phaseGraph.value(frequency).value;
                                r = m_farEndMeasurementsAdd.at(len - number).rsrGraph.value(frequency).value;
                                x = m_farEndMeasurementsAdd.at(len - number).rsxGraph.value(frequency).value;
                            }else
                            {
                                rho = m_measurements.at(len - number).rhoGraph.value(frequency).value;
                                phase = m_measurements.at(len - number).phaseGraphCalib.value(frequency).value;
                                r = m_measurements.at(len - number).dataRXCalib.at(previousI).r;
                                x = m_measurements.at(len - number).dataRXCalib.at(previousI).x;
                            }
                            swr = m_measurements.at(len - number).swrGraphCalib.value(frequency).value;
                            rl = m_measurements.at(len - number).rlGraphCalib.value(frequency).value;
                            z = m_viewMeasurements.at(len - number).rszGraph.value(frequency).value;
                            rpar = m_viewMeasurements.at(len - number).rprGraph.value(frequency).value;
                            xpar = m_viewMeasurements.at(len - number).rpxGraph.value(frequency).value;
                        }else
                        {
                            if(m_farEndMeasurement == 1)
                            {
                                rho = m_farEndMeasurementsSub.at(len - number).rhoGraph.value(frequency).value;
                                phase = m_farEndMeasurementsSub.at(len - number).phaseGraph.value(frequency).value;
                                r = m_farEndMeasurementsSub.at(len - number).rsrGraph.value(frequency).value;
                                x = m_farEndMeasurementsSub.at(len - number).rsxGraph.value(frequency).value;
                            }else if(m_farEndMeasurement == 2)
                            {
                                rho = m_farEndMeasurementsAdd.at(len - number).rhoGraph.value(frequency).value;
                                phase = m_farEndMeasurementsAdd.at(len - number).phaseGraph.value(frequency).value;
                                r = m_farEndMeasurementsAdd.at(len - number).rsrGraph.value(frequency).value;
                                x = m_farEndMeasurementsAdd.at(len - number).rsxGraph.value(frequency).value;
                            }else
                            {
                                rho = m_measurements.at(len - number).rhoGraph.value(frequency).value;
                                phase = m_measurements.at(len - number).phaseGraph.value(frequency).value;
                                r = m_measurements.at(len - number).dataRX.at(previousI).r;
                                x = m_measurements.at(len - number).dataRX.at(previousI).x;
                            }
                            swr = m_measurements.at(len - number).swrGraph.value(frequency).value;
                            rl = m_measurements.at(len - number).rlGraph.value(frequency).value;
                            z = m_viewMeasurements.at(len - number).rszGraph.value(frequency).value;
                            rpar = m_viewMeasurements.at(len - number).rprGraph.value(frequency).value;
                            xpar = m_viewMeasurements.at(len - number).rpxGraph.value(frequency).value;
                        }

                        zString+= QString::number(r,'f', 2);
                        if(x >= 0)
                        {
                            zString+= " + j";
                            zString+= QString::number(x,'f', 2);
                        }else
                        {
                            zString+= " - j";
                            zString+= QString::number((x * (-1)),'f', 2);
                        }
                        zparString+= QString::number(rpar,'f', 2);
                        if(x >= 0)
                        {
                            zparString+= " + j";
                            zparString+= QString::number(xpar,'f', 2);
                        }else
                        {
                            zparString+= " - j";
                            zparString+= QString::number((xpar * (-1)),'f', 2);
                        }

                        l = 1E9 * x / (2*M_PI * frequency * 1E3);//nH
                        c = 1E12 / (2*M_PI * frequency * (x * (-1)) * 1E3);//pF

                        lpar = 1E9 * xpar / (2*M_PI * frequency * 1E3);
                        cpar = 1E12 / (2*M_PI * frequency * (xpar * (-1)) * 1E3);

                        res = true;
                        if(m_graphBriefHint != NULL)
                        {
                            QString str;
                            str = QString::number(frequency);
                            int index = str.indexOf(".");
                            if(index < 0)
                            {
                                if(str.length() > 6)
                                {
                                    str.insert(str.length()-6," ");
                                }
                                if(str.length() > 3)
                                {
                                    str.insert(str.length()-3," ");
                                }
                            }else
                            {
                                int len = str.length() - index;
                                if((str.length()-len) > 6)
                                {
                                    str.insert(str.length()-len-6," ");
                                }
                                if((str.length()-len) > 3)
                                {
                                    str.insert(str.length()-len-3," ");
                                }
                            }
                            str += "\n";
                            if(m_currentTab == "tab_1")
                            {
                                str += QString::number(swr,'f',2);
                            }else if(m_currentTab == "tab_2")
                            {
                                str += QString::number(phase,'f',2);
                            }else if(m_currentTab == "tab_3")
                            {
                                str += QString::number(computeZ(r,x),'f',2);
                            }else if(m_currentTab == "tab_4")
                            {
                                str += QString::number(computeZ(r,x),'f',2);
                            }else if(m_currentTab == "tab_5")
                            {
                                str += QString::number(rl,'f',2);
                            }
                            m_graphBriefHint->setPopupText(str);
                        }
                        break;
                    }
                }
            }
            if(m_currentTab == "tab_1")
            {
                if(!m_swrLine)
                {
                    m_swrLine = new QCPItemStraightLine(m_swrWidget);
                    m_swrLine->setAntialiased(false);
                    m_swrWidget->addItem(m_swrLine);
                }
                if(!m_swrLine2)
                {
                    m_swrLine2 = new QCPItemStraightLine(m_swrWidget);
                    m_swrLine2->setAntialiased(false);
                    m_swrWidget->addItem(m_swrLine2);
                }
                m_swrLine->point1->setCoords(frequency, 1);
                m_swrLine->point2->setCoords(frequency, MAX_SWR);

                m_swrLine2->point1->setCoords(m_swrWidget->yAxis->getRangeLower(), swr);
                m_swrLine2->point2->setCoords(m_swrWidget->yAxis->getRangeUpper(), swr);
            }else if(m_currentTab == "tab_2")
            {
                if(!m_phaseLine)
                {
                    m_phaseLine = new QCPItemStraightLine(m_phaseWidget);
                    m_phaseLine->setAntialiased(false);
                    m_phaseWidget->addItem(m_phaseLine);
                }
                if(!m_phaseLine2)
                {
                    m_phaseLine2 = new QCPItemStraightLine(m_phaseWidget);
                    m_phaseLine2->setAntialiased(false);
                    m_phaseWidget->addItem(m_phaseLine2);
                }
                m_phaseLine->point1->setCoords(frequency, -2000);
                m_phaseLine->point2->setCoords(frequency, 2000);
                m_phaseLine2->point1->setCoords(m_phaseWidget->yAxis->getRangeLower(), phase);
                m_phaseLine2->point2->setCoords(m_phaseWidget->yAxis->getRangeUpper(), phase);
            }else if(m_currentTab == "tab_3")
            {
                if(!m_rsLine)
                {
                    m_rsLine = new QCPItemStraightLine(m_rsWidget);
                    m_rsLine->setAntialiased(false);
                    m_rsWidget->addItem(m_rsLine);
                }
                m_rsLine->point1->setCoords(frequency, -2000);
                m_rsLine->point2->setCoords(frequency, 2000);
            }else if(m_currentTab == "tab_4")
            {
                if(!m_rpLine)
                {
                    m_rpLine = new QCPItemStraightLine(m_rpWidget);
                    m_rpLine->setAntialiased(false);
                    m_rpWidget->addItem(m_rpLine);
                }
                m_rpLine->point1->setCoords(frequency, -2000);
                m_rpLine->point2->setCoords(frequency, 2000);
            }else if(m_currentTab == "tab_5")
            {
                if(!m_rlLine)
                {
                    m_rlLine = new QCPItemStraightLine(m_rlWidget);
                    m_rlLine->setAntialiased(false);
                    m_rlWidget->addItem(m_rlLine);
                }
                if(!m_rlLine2)
                {
                    m_rlLine2 = new QCPItemStraightLine(m_rlWidget);
                    m_rlLine2->setAntialiased(false);
                    m_rlWidget->addItem(m_rlLine2);
                }
                m_rlLine->point1->setCoords(frequency, -2000);
                m_rlLine->point2->setCoords(frequency, 2000);
                m_rlLine2->point1->setCoords(m_rlWidget->yAxis->getRangeLower(), rl);
                m_rlLine2->point2->setCoords(m_rlWidget->yAxis->getRangeUpper(), rl);
            }

            QString str = QString::number(frequency);
            int index = str.indexOf(".");
            if(index < 0)
            {
                if(str.length() > 6)
                {
                    str.insert(str.length()-6," ");
                }
                if(str.length() > 3)
                {
                    str.insert(str.length()-3," ");
                }
            }else
            {
                int len = str.length() - index;
                if((str.length()-len) > 6)
                {
                    str.insert(str.length()-len-6," ");
                }
                if((str.length()-len) > 3)
                {
                    str.insert(str.length()-len-3," ");
                }
            }

            QString text;
            if(x > 0)
            {
                text = QString(tr("Frequency = %1 kHz\n"
                                  "SWR = %2\n"
                                  "RL = %3 dB\n"
                                  "Z = %4 Ohm\n"
                                  "|Z| = %5 Ohm\n"
                                  "|rho| = %6, phase = %7 °\n"
                                  "L = %8 nH\n"
                                  "Zpar = %9 Ohm\n"
                                  "Lpar = %10 nH\n"))
                    .arg(str)
                    .arg(QString::number(swr,'f', 2))
                    .arg(QString::number(rl,'f', 2))
                    .arg(zString)
                    .arg(QString::number(z,'f', 2))
                    .arg(QString::number(rho,'f', 2))
                    .arg(QString::number(phase,'f', 2))
                    .arg(QString::number(l,'f', 2))
                    .arg(zparString)
                    .arg(QString::number(lpar,'f', 2));
            }else
            {
                text = QString(tr("Frequency = %1 kHz\n"
                                  "SWR = %2\n"
                                  "RL = %3 dB\n"
                                  "Z = %4 Ohm\n"
                                  "|Z| = %5 Ohm\n"
                                  "|rho| = %6, phase = %7 °\n"
                                  "C = %8 pF\n"
                                  "Zpar = %9 Ohm\n"
                                  "Cpar = %10 pF\n"))
                        .arg(str)
                        .arg(QString::number(swr,'f', 2))
                        .arg(QString::number(rl,'f', 2))
                        .arg(zString)
                        .arg(QString::number(z,'f', 2))
                        .arg(QString::number(rho,'f', 2))
                        .arg(QString::number(phase,'f', 2))
                        .arg(QString::number(c,'f', 2))
                        .arg(zparString)
                        .arg(QString::number(cpar,'f', 2));
            }

            if(!m_farEndMeasurement)
            {
                QString cableString;
                QString lenUnits;
                if(m_measureSystemMetric)
                {
                    lenUnits = tr("m");
                }else
                {
                    lenUnits = tr("ft");
                }

                double len14 = SPEEDOFLIGHT/4/frequency/1000*m_cableVelFactor;
                double len12 = SPEEDOFLIGHT/2/frequency/1000*m_cableVelFactor;

                if(!m_measureSystemMetric)
                {
                    len14 *= FEETINMETER;
                    len12 *= FEETINMETER;
                }

                cableString = tr("Cable: length(1/4) = %1 %2, length(1/2) = %3 %4")
                        .arg(QString::number(len14,'f',2))
                        .arg(lenUnits)
                        .arg(QString::number(len12,'f',2))
                        .arg(lenUnits);
                text += cableString;
            }
            m_graphHint->setPopupText(text);
        }
    }
    replot();
}

void Measurements::on_mainWindowPos(int x, int y)
{
    if(m_graphHint)
    {
        m_graphHint->MainWindowPos(x, y);
    }
}

void Measurements::setCalibrationMode(bool enabled)
{
    m_calibrationMode = enabled;
}

bool Measurements::getGraphHintEnabled(void)
{
    return m_graphHintEnabled;
}

void Measurements::on_calibrationEnabled(bool enabled)
{
    if(m_swrWidget->graphCount() == 1)
    {
        return;
    }

    int graphsCount = m_swrWidget->graphCount();
    int measurementsCount = m_measurements.length()-1;
    for(int i = 1; i < graphsCount; ++i)
    {
        QCPDataMap swrmap;
        QCPDataMap rszmap;
        QCPDataMap rsxmap;
        QCPDataMap rsrmap;
        QCPDataMap rpzmap;
        QCPDataMap rpxmap;
        QCPDataMap rprmap;
        QCPDataMap rlmap;
        QCPDataMap phasemap;
        if(enabled)
        {
            swrmap = m_measurements.at(measurementsCount-(i-1)).swrGraphCalib;
            phasemap = m_measurements.at(measurementsCount-(i-1)).phaseGraphCalib;

            rszmap = m_measurements.at(measurementsCount-(i-1)).rszGraphCalib;
            rsxmap = m_measurements.at(measurementsCount-(i-1)).rsxGraphCalib;
            rsrmap = m_measurements.at(measurementsCount-(i-1)).rsrGraphCalib;

            rpzmap = m_measurements.at(measurementsCount-(i-1)).rpzGraphCalib;
            rpxmap = m_measurements.at(measurementsCount-(i-1)).rpxGraphCalib;
            rprmap = m_measurements.at(measurementsCount-(i-1)).rprGraphCalib;

            rlmap = m_measurements.at(measurementsCount-(i-1)).rlGraphCalib;
        }else
        {
            swrmap = m_measurements.at(measurementsCount-(i-1)).swrGraph;
            phasemap = m_measurements.at(measurementsCount-(i-1)).phaseGraph;

            rszmap = m_measurements.at(measurementsCount-(i-1)).rszGraph;
            rsxmap = m_measurements.at(measurementsCount-(i-1)).rsxGraph;
            rsrmap = m_measurements.at(measurementsCount-(i-1)).rsrGraph;

            rpzmap = m_measurements.at(measurementsCount-(i-1)).rpzGraph;
            rpxmap = m_measurements.at(measurementsCount-(i-1)).rpxGraph;
            rprmap = m_measurements.at(measurementsCount-(i-1)).rprGraph;

            rlmap = m_measurements.at(measurementsCount-(i-1)).rlGraph;
        }

        m_swrWidget->graph(i)->setData(&swrmap, true);
        m_phaseWidget->graph(i)->setData(&phasemap, true);

        m_rsWidget->graph((i*3))->setData(&rszmap, true);
        m_rsWidget->graph((i*3)-1)->setData(&rsxmap, true);
        m_rsWidget->graph((i*3)-2)->setData(&rsrmap, true);        

        m_rpWidget->graph((i*3))->setData(&rpzmap, true);
        m_rpWidget->graph((i*3)-1)->setData(&rpxmap, true);
        m_rpWidget->graph((i*3)-2)->setData(&rprmap, true);

        m_rlWidget->graph(i)->setData(&rlmap, true);
    }
    replot();
    emit calibrationChanged();
}

void Measurements::saveData(quint32 number, QString path)
{
    number = m_measurements.length()-1 - number;

    if(path.indexOf(".asd") >= 0 )
    {
        QFile saveFile(path);

        if (!saveFile.open(QIODevice::WriteOnly))
        {
            qWarning("Couldn't open save file.");
            return;
        }

        QVector <rawData> data;
        if(m_calibration != NULL)
        {
            if(m_calibration->getCalibrationEnabled())
            {
                data = m_measurements.at(number).dataRXCalib;
            }else
            {
                data = m_measurements.at(number).dataRX;
            }
        }else
        {
            data = m_measurements.at(number).dataRX;
        }

        //Dots
        QJsonObject mainObj;
        mainObj["DotsNumber"] = data.length();

        //Measurements
        QJsonArray measurementsArray;
        for(int i = 0; i < data.length(); ++i)
        {
            QJsonObject obj;
            obj["fq"] = data.at(i).fq;
            obj["r"] = data.at(i).r;
            obj["x"] = data.at(i).x;
            measurementsArray.append(obj);
        }
        mainObj["Measurements"] = measurementsArray;

        QJsonDocument saveDoc(mainObj);

        saveFile.write(saveDoc.toJson());
    }
}

void Measurements::loadData(QString path)
{
    if(path.indexOf(".asd") >= 0 )
    {
        QStringList list;
        list = path.split("/");
        if(list.length() == 1)
        {
            list.clear();
            list = path.split("\\");
        }

        QFile loadFile(path);

        if (!loadFile.open(QIODevice::ReadOnly)) {
            QMessageBox::information(NULL, tr("Error"), tr("Couldn't open save file."));
            qWarning("Couldn't open save file.");
            return;
        }
        on_newMeasurement(list.last());

        QByteArray saveData = loadFile.readAll();

        QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
        QJsonObject mainObj = loadDoc.object();

        QJsonArray measureArray = mainObj["Measurements"].toArray();

        for(int i = 0; i < measureArray.size(); ++i)
        {
            QJsonObject dataObject = measureArray[i].toObject();
            rawData data;
            data.read(dataObject);
            on_newData(data);
        }
    }else
    {
        importData(path);
    }
    on_redrawGraphs();
}

void Measurements::exportData(QString _name, int _type, int _number)
{
    if(_name.indexOf(".s1p") >= 0 )
    {
        QFile file(_name);

        if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))//if (!file.open(QFile::ReadWrite))
        {
            return;
        }

        QTextStream out(&file);

        out << "! Touchstone file generated by AntScope2";
        out << "\n";

        double Rswr = m_calibration->getZ0();

        if (_type == 0) // Z, RI
        {
            out << "# MHz Z RI R " << Rswr << "\n";
            out << "! Format: Frequency Z-real Z-imaginary (normalized to " << Rswr << " Ohm)\n";
        }else if (_type == 1) // S, RI
        {
            out << "# MHz S RI R " << Rswr << "\n";
            out << "! Format: Frequency S-real S-imaginary (normalized to " << Rswr << " Ohm)\n";
        }
        else if (_type == 2) // S, MA
        {
            out << "# MHz S MA R " << Rswr << "\n";
            out << "! Format: Frequency S-magnitude S-angle (normalized to " << Rswr << " Ohm, angle in degrees)\n";
        }

        // Data
        int len = 0;
        QVector<rawData> vector;
        if((m_calibration != NULL) && (m_calibration->getCalibrationEnabled()))
        {
            vector = m_measurements.at(_number).dataRXCalib;
        }else
        {
            vector = m_measurements.at(_number).dataRX;
        }
        len = vector.length();

        for (int i = 0; i < len; ++i)
        {
            QString s;

            s = QString("%1").arg(vector.at(i).fq);		// Fq
            out << s << " ";

            double R = vector.at(i).r;
            double X = vector.at(i).x;

            if (_type == 0) // Z, RI
            {
                if (!_isnan(R))
                    s = QString::number(R/Rswr,'g',4);           // R
                else
                    s = "0";
                out << s << " ";
                if (!_isnan(X))
                    s = QString::number(X/Rswr,'g',4);           // X
                else
                    s = "0";
                out << s << "\n";
            }
            else
            if (_type == 1) // S, RI
            {
                double Gre = (R*R-Rswr*Rswr+X*X)/((R+Rswr)*(R+Rswr)+X*X);
                double Gim = (2*Rswr*X)/((R+Rswr)*(R+Rswr)+X*X);

                if (!_isnan(Gre))
                    s = QString::number(Gre,'g',4);              // Real
                else
                    s = "0";
                out << s << " ";

                if (!_isnan(Gim))
                    s = QString::number(Gim,'g',4);              // Imaginary
                else
                    s = "0";
                out << s << "\n";

            }
            else
            if (_type == 2) // S, MA
            {
                double Gre = (R*R-Rswr*Rswr+X*X)/((R+Rswr)*(R+Rswr)+X*X);
                double Gim = (2*Rswr*X)/((R+Rswr)*(R+Rswr)+X*X);

                if (!_isnan(Gre))
                    s = QString::number(sqrt(Gre*Gre+Gim*Gim),'g',4);		// Magnitude
                else
                    s = "0";
                out << s << " ";

                if (!_isnan(Gim))
                    s = QString::number(atan2(Gim,Gre)/3.1415926*180.0,'g',4);		// Angle
                else
                    s = "0";
                out << s << "\n";
            }
        }
        out.flush();
    }else if(_name.indexOf(".csv") >= 0 )
    {
        QString str;
        QFile file(_name);
        bool result = file.open(QFile::ReadWrite);
        if(result)
        {
            str = "Frequency;R;X";
            file.write( str.toLocal8Bit(), str.length());
            file.write("\r\n", 2);

            int len = 0;
            QVector<rawData> vector;
            if((m_calibration != NULL) && (m_calibration->getCalibrationEnabled()))
            {
                vector = m_measurements.at(_number).dataRXCalib;
            }else
            {
                vector = m_measurements.at(_number).dataRX;
            }
            len = vector.length();

            for (int i = 0; i < len; ++i)
            {
                str = QString::number(vector.at(i).fq) +
                "," +//";" +
                QString::number(vector.at(i).r,'f',2) +
                "," +//";" +
                QString::number(vector.at(i).x,'f',2);

                file.write( str.toLocal8Bit(), str.length());
                file.write("\r\n", 2);
            }
            file.close();
        }
    }else if(_name.indexOf(".nwl") >= 0 )
    {
        QString str;
        QFile file(_name);
        bool result = file.open(QFile::ReadWrite);
        if(result)
        {
            str = "/\"Freq(kHz)\" \"Rs\" \"Xs\"/";
            file.write( str.toLocal8Bit(), str.length());
            file.write("\r\n", 2);

            int len = 0;
            QVector<rawData> vector;
            if((m_calibration != NULL) && (m_calibration->getCalibrationEnabled()))
            {
                vector = m_measurements.at(_number).dataRXCalib;
            }else
            {
                vector = m_measurements.at(_number).dataRX;
            }
            len = vector.length();

            for (int i = 0; i < len; ++i)
            {
                str = QString::number(vector.at(i).fq) +
                " " +//";" +
                QString::number(vector.at(i).r,'f',2) +
                " " +//";" +
                QString::number(vector.at(i).x,'f',2);

                file.write( str.toLocal8Bit(), str.length());
                file.write("\r\n", 2);
            }
            file.close();
        }
    }
}

void Measurements::importData(QString _name)
{
    if(_name.indexOf(".s1p") >= 0 )
    {
        QStringList list;
        list = _name.split("/");
        if(list.length() == 1)
        {
            list.clear();
            list = _name.split("\\");
        }
        on_newMeasurement(list.last());

        QString sPathName = _name;

        if (sPathName.isEmpty())
        {
            return;
        }

        QFile ifs(sPathName);

        if (!ifs.open(QFile::ReadWrite))
        {
            return;
        }
        QTextStream in(&ifs);
        bool bGood = true;

        int iLines=0, iPoints=0;

        double  fqmul = 1000.0; // Default is GHz
        int iUnit = 1; // Default is S
        int iFormat = 1; // Default is MA

        QString line;//char str[1000]; // Whole string
        char strn[5][100]; // Substrings

        double f, param1, param2; // for reading S11 data lines

        double Z0 = 50;

        do//while (ifs.isOpen() && (!ifs.eof()))
        {
            line = in.readLine();
            iLines++;

            if ( (line.length() > 2) && (line[0] == '#')) // Option line
            {
                line.remove(0,1);
                bool bErr = false;

                int ns = sscanf(line.toLocal8Bit(), "%s %s %s %s %s", strn[0], strn[1], strn[2], strn[3], strn[4]);

                for (int i=0; i<ns; i++)
                {
                    // Frequency unit

                    if (!strcmp(strn[i], "GHz"))
                        fqmul = 1000.0;
                    else
                    if (!strcmp(strn[i], "MHz"))
                        fqmul = 1.0;
                    else
                    if (!strcmp(strn[i], "KHz"))
                        fqmul = 0.001;
                    else
                    if (!strcmp(strn[i], "Hz"))
                        fqmul = 0.000001;
                    else

                    // Parameter

                    if (!strcmp(strn[i], "S"))
                        iUnit = 1;
                    else
                    if (!strcmp(strn[i], "Z"))
                        iUnit = 2;
                    else

                    // Format

                    if (!strcmp(strn[i], "MA"))
                        iFormat = 1;
                    else
                    if (!strcmp(strn[i], "RI"))
                        iFormat = 2;
                    else

                    // R n

                    if (!strcmp(strn[i], "R"))
                    {
                        if ( i < (ns-1) )
                        {
                            i++;

//                            setlocale(LC_NUMERIC,"C");
                            Z0 = atof(strn[i]);
//                            setlocale(LC_NUMERIC,"");

                            if ( (Z0<=0) || (Z0>10000) )
                            {
                                bErr = true;
                                break;
                            }
                        }
                        else
                        {
                            bErr = true;
                            break;
                        }
                    }
                    else
                    {
                        bErr = true;
                        break;
                    }
                }

                // Check possible combinations

                if ( (iUnit == 1) && (iFormat == 1) ) // S, MA
                {
                }
                else
                if ( (iUnit == 1) && (iFormat == 2) ) // S, RI
                {
                }
                else
                if ( (iUnit == 2) && (iFormat == 2) ) // Z, RI
                {
                }
                else
                {
                    bErr = true;
                }

                if (bErr == true)
                {
                    return;
                }

                continue;
            }

            if ( (strstr(line.toLocal8Bit(), "!") != NULL) || (strstr(line.toLocal8Bit(), ".") == NULL) ) // Comment or void line
                continue;

            // Scan data lines

            if ( sscanf(line.toLocal8Bit(), "%lf %lf %lf", &f, &param1, &param2) != 3)
            {
                return;
            }

            double r = 0,x = 0;

            if ( (iUnit == 1) && (iFormat == 1) ) // S, MA
            {
                    double Gr = param1 * cos(param2/180.0*M_PI);
                    double Gi = param1 * sin(param2/180.0*M_PI);

                    r = (1-Gr*Gr-Gi*Gi)/((1-Gr)*(1-Gr)+Gi*Gi);
                    x = (2*Gi)/((1-Gr)*(1-Gr)+Gi*Gi);
            }
            else
            if ( (iUnit == 1) && (iFormat == 2) ) // S, RI
            {
                    r = (1-param1*param1-param2*param2)/((1-param1)*(1-param1)+param2*param2);
                    x = (2*param2)/((1-param1)*(1-param1)+param2*param2);
            }
            else
            if ( (iUnit == 2) && (iFormat == 2) ) // Z, RI
            {
                    r = param1;
                    x = param2;
            }
            else
            {
                // Bug
            }

            if ( _isnan(r) || (r<0) )
            {
                r = 0;
            }
            if ( _isnan(x) )
            {
                x = 0;
            }

            rawData data;
            data.fq = f*fqmul;
            data.r =r*(Z0);
            data.x =x*(Z0);
            on_newData(data);
            iPoints++;
        }while (!line.isNull());

        if (bGood && (iPoints>1) )
        {
            return;
        }
        else
        {
            return;
        }
    }else if(_name.indexOf(".csv") >= 0 )
    {
        QStringList list;
        list = _name.split("/");
        if(list.length() == 1)
        {
            list.clear();
            list = _name.split("\\");
        }
        on_newMeasurement(list.last());

        QFile file(_name);
        bool result = file.open(QFile::ReadOnly);
        if(result)
        {
            QString str = file.readAll();

            QStringList nList = str.split('\n');
            for(int i = 1; i < nList.length(); ++i)
            {
                QStringList dList = nList.at(i).split(',');
                if(dList.length() ==3)
                {
                    rawData data;
                    data.fq = dList.at(0).toDouble();
                    data.r = dList.at(1).toDouble();
                    data.x = dList.at(2).toDouble();
                    on_newData(data);
                }
            }
        }
    }else if(_name.indexOf(".nwl") >= 0 )
    {
        QStringList list;
        list = _name.split("/");
        if(list.length() == 1)
        {
            list.clear();
            list = _name.split("\\");
        }
        on_newMeasurement(list.last());

        QFile file(_name);
        bool result = file.open(QFile::ReadOnly);
        if(result)
        {
            QString str = file.readAll();

            QStringList nList = str.split('\n');
            for(int i = 1; i < nList.length(); ++i)
            {
                QStringList dList = nList.at(i).split(' ');
                if(dList.length() ==3)
                {
                    rawData data;
                    data.fq = dList.at(0).toDouble();
                    data.r = dList.at(1).toDouble();
                    data.x = dList.at(2).toDouble();
                    on_newData(data);
                }
            }
        }
    }
}

int Measurements::CalcTdr(QVector <rawData> *data)
{
    int asize = data->length();

    if (asize < 200)
    {
        return 0;
    }

    double minfq = data->at(0).fq;
    if ( minfq > 0.1 )
    {
        return 0; // Wrong fq
    }

    double maxfq = data->at(asize-1).fq;

    int m_iTdrFftSize = 0;

    int i;
    for (i=0; ; i++)
    {
        m_iTdrFftSize = (1<<i);
        if ( (m_iTdrFftSize/2) >= (asize-1) )
            break;

        if (i==14)
            return 0; // bug
    }

    m_iTdrFftSize *= 8;

    if (m_iTdrFftSize > TDR_MAXARRAY)
            return 0; // bug

    m_tdrResolution = 1.0/(maxfq-minfq)/4*299792458*m_cableVelFactor / (m_iTdrFftSize/2) * (asize-1);

    m_tdrRange = m_tdrResolution*m_iTdrFftSize/1000000;

    float *TdrReal = new float[TDR_MAXARRAY];
    float *TdrImag = new float[TDR_MAXARRAY];

#define Rdevice 50.0

    for (i=0; i<=m_iTdrFftSize/2; i++)
    {
        if (i < asize)
        {
            double R = data->at(i).r;
            double X = data->at(i).x;

            double Gre = (R*R-Rdevice*Rdevice+X*X)/((R+Rdevice)*(R+Rdevice)+X*X);
            double Gim = (2*Rdevice*X)/((R+Rdevice)*(R+Rdevice)+X*X);

            if ( i==0)
            {
                double m_dFarEndImpedance = 50;
                Gre = (m_dFarEndImpedance-Rdevice)/(m_dFarEndImpedance+Rdevice);
                Gim = 0;
            }

#define KP (1.0/0.53836)
                double k = 0.53836-0.46146*cos(M_PI+M_PI*i/(asize-1));

                TdrReal[i] = Gre*m_iTdrFftSize/asize/2.0*k*KP;
                TdrImag[i] = Gim*m_iTdrFftSize/asize/2.0*k*KP;
        }
        else
        {
            TdrReal[i] = 0;
            TdrImag[i] = 0;
        }
    }

// Interpolate zero frequency

#define BDR 1
    for (i=0; i<BDR; i++)
    {
        double newreal = sqrt(TdrReal[BDR]*TdrReal[BDR]+TdrImag[BDR]*TdrImag[BDR]);

        if (TdrReal[BDR] < 0)
            TdrReal[i] = -newreal;
        else
            TdrReal[i] = newreal;

        TdrImag[i] = 0;

    }

// Mirror
    for (i=1; i<m_iTdrFftSize/2; i++)
    {
        TdrReal[m_iTdrFftSize-i] = TdrReal[i];
        TdrImag[m_iTdrFftSize-i] = -TdrImag[i];
    }
    TdrReal[m_iTdrFftSize/2] = 0;
    TdrImag[m_iTdrFftSize/2] = 0;

    FFT(TdrReal, TdrImag, m_iTdrFftSize, 1/*Inverse*/);	// Inverse FFT

    double ig = 0;
    for (i=0; i<m_iTdrFftSize; i++)
    {
        double Amp = TdrReal[i];
        if((Amp > 0.015) || (Amp < -0.015))
        {
            m_pdTdrImp[i] = Amp;
            ig += Amp/2/(((double)m_iTdrFftSize)/asize/2);
        }else
        {
            m_pdTdrImp[i] = 0;
        }

        m_pdTdrStep[i] = ig;
    }

    delete TdrReal;
    delete TdrImag;
    return m_iTdrFftSize;
}

void Measurements::FFT(float real[], float imag[], int length, int Inverse)
{
    double wreal, wpreal, wimag, wpimag, theta;
    double tempreal, tempimag, tempwreal, direction;

    int Addr, Position, Mask, BitRevAddr, PairAddr;
    int m, k;

    direction = -1.0;		// direction of rotating phasor for FFT

    if(Inverse)
        direction = 1.0;	// direction of rotating phasor for IFFT

    //  bit-reverse the addresses of both the real and imaginary arrays
    //  real[0..length-1] and imag[0..length-1] are the paired complex numbers

    for (Addr=0; Addr<length; Addr++)
    {
        // Derive Bit-Reversed Address
        BitRevAddr = 0;
        Position = length >> 1;
        Mask = Addr;
        while (Mask)
        {
            if(Mask & 1)
                BitRevAddr += Position;
            Mask >>= 1;
            Position >>= 1;
        }

        if (BitRevAddr > Addr)				// Swap
        {
            double s;
            s = real[BitRevAddr];			// real part
            real[BitRevAddr] = real[Addr];
            real[Addr] = s;
            s = imag[BitRevAddr];			// imaginary part
            imag[BitRevAddr] = imag[Addr];
            imag[Addr] = s;
        }
    }

    // FFT, IFFT Kernel

    for (k=1; k < length; k <<= 1)
    {
        theta = direction * M_PI / (double)k;
        wpimag = sin(theta);
        wpreal = cos(theta);
        wreal = 1.0;
        wimag = 0.0;

        for (m=0; m < k; m++)
        {
            for (Addr = m; Addr < length; Addr += (k*2))
            {
                PairAddr = Addr + k;

                tempreal = wreal * (double)real[PairAddr] - wimag * (double)imag[PairAddr];
                tempimag = wreal * (double)imag[PairAddr] + wimag * (double)real[PairAddr];


                real[PairAddr] = (double)real[Addr] - tempreal;
                imag[PairAddr] = (double)imag[Addr] - tempimag;
                real[Addr] += tempreal;
                imag[Addr] += tempimag;
            }
            tempwreal = wreal;
            wreal = wreal * wpreal - wimag * wpimag;
            wimag = wimag * wpreal + tempwreal * wpimag;
        }
    }

    if(Inverse)							// Normalize the IFFT coefficients
        for(int i=0; i<length; i++)
        {
            real[i] /= (double)length;
            imag[i] /= (double)length;
        }
}

void Measurements::on_dotsNumberChanged(int number)
{
    m_dotsNumber = number;
}

void Measurements::on_changeMeasureSystemMetric (bool state)
{
    m_measureSystemMetric = state;
    int count = m_tdrWidget->graphCount();
    qDebug() << "Measurements::on_changeMeasureSystemMetric " << count;
    if(m_tdrWidget->graphCount()>2)
    {
        if(m_measureSystemMetric)
        {
            m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_measurements.last().tdrImpGraph, true);
            m_tdrWidget->graph()->setData(&m_measurements.last().tdrStepGraph, true);
        }else
        {
            m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_measurements.last().tdrImpGraphFeet, true);
            m_tdrWidget->graph()->setData(&m_measurements.last().tdrStepGraphFeet, true);
        }
    }
    if(m_measureSystemMetric)
    {
        m_tdrWidget->xAxis->setLabel(tr("Length, m"));
    }
    else
    {
        m_tdrWidget->xAxis->setLabel(tr("Length, feet"));
    }
}

void Measurements::on_redrawGraphs()
{
    if(m_calibration == NULL)
    {
        return;
    }
    if(m_measurements.length() == 0)
    {
        replot();
        return;
    }

    if(m_calibration->getCalibrationEnabled())
    {
        if( m_currentTab == "tab_1")//SWR
        {
            if(m_farEndMeasurement)
            {
                calcFarEnd();
            }
            for(int i = 0; i < m_measurements.length(); ++i)
            {
                m_viewMeasurements[i].swrGraphCalib.clear();

                QCPDataMap map = m_measurements[i].swrGraphCalib;
                QList <double> list = map.keys();
                QCPData data;
                QCPData viewData;
                double maxSwr = m_swrWidget->yAxis->range().upper;
                for(int n = 0; n < list.length(); ++n)
                {
                    data.key = list.at(n);
                    viewData = map.value(list.at(n));
                    if( viewData.value > maxSwr || viewData.value < 1)
                    {
                        data.value = maxSwr;
                    }else
                    {
                        data.value = viewData.value;
                    }
                    m_viewMeasurements[i].swrGraphCalib.insert(data.key,data);
                }
                m_swrWidget->graph(i+1)->setData(&m_viewMeasurements[i].swrGraphCalib, true);
            }
        }else if(m_currentTab == "tab_2")//Phase
        {
            if(m_farEndMeasurement == 1)
            {
                calcFarEnd();
                for(int i = 0; i < m_measurements.length(); ++i)
                {
                    m_phaseWidget->graph(i+1)->setData(&m_farEndMeasurementsSub[i].phaseGraph, true);
                }
            }else if(m_farEndMeasurement == 1)
            {
                calcFarEnd();
                for(int i = 0; i < m_measurements.length(); ++i)
                {
                    m_phaseWidget->graph(i+1)->setData(&m_farEndMeasurementsAdd[i].phaseGraph, true);
                }
            }else
            {
                for(int i = 0; i < m_measurements.length(); ++i)
                {
                    m_phaseWidget->graph(i+1)->setData(&m_measurements[i].phaseGraphCalib, true);
                }
            }
        }else if(m_currentTab == "tab_3")//RX
        {
            calcFarEnd();
            for(int i = 0; i < m_measurements.length(); ++i)
            {
                QCPDataMap mapr;
                QCPDataMap mapx;
                QCPDataMap mapz;
                m_viewMeasurements[i].rsrGraph.clear();
                m_viewMeasurements[i].rsxGraph.clear();
                m_viewMeasurements[i].rszGraph.clear();
                QCPData data;
                QCPData value;
                double maxVal = m_rsWidget->yAxis->range().upper;
                double minVal = m_rsWidget->yAxis->range().lower;

                QList <double> listr;
                QList <double> listx;
                QList <double> listz;
                if(m_farEndMeasurement)
                {
                    if(m_farEndMeasurement == 1)
                    {
                        mapr = m_farEndMeasurementsSub.at(i).rsrGraph;
                        mapx = m_farEndMeasurementsSub.at(i).rsxGraph;
                        mapz = m_farEndMeasurementsSub.at(i).rszGraph;
                    }else if (m_farEndMeasurement == 2)
                    {
                        mapr = m_farEndMeasurementsAdd.at(i).rsrGraph;
                        mapx = m_farEndMeasurementsAdd.at(i).rsxGraph;
                        mapz = m_farEndMeasurementsAdd.at(i).rszGraph;
                    }
                }else
                {
                    mapr = m_measurements.at(i).rsrGraphCalib;
                    mapx = m_measurements.at(i).rsxGraphCalib;
                    mapz = m_measurements.at(i).rszGraphCalib;
                }
                listr = mapr.keys();
                listx = mapx.keys();
                listz = mapz.keys();
                for(int n = 0; n < listr.length(); ++n)
                {
                    data.key = listr.at(n);
                    value = mapr.value(listr.at(n));
                    if( value.value > maxVal)
                    {
                        data.value = maxVal;
                    }else if(value.value < minVal)
                    {
                        data.value = minVal;
                    }else
                    {
                        data.value = value.value;
                    }
                    m_viewMeasurements[i].rsrGraph.insertMulti(data.key,data);

                    data.key = listx.at(n);
                    value = mapx.value(listx.at(n));
                    if( value.value > maxVal)
                    {
                        data.value = maxVal;
                    }else if(value.value < minVal)
                    {
                        data.value = minVal;
                    }else
                    {
                        data.value = value.value;
                    }
                    m_viewMeasurements[i].rsxGraph.insertMulti(data.key,data);

                    data.key = listz.at(n);
                    value = mapz.value(listz.at(n));
                    if( value.value > maxVal)
                    {
                        data.value = maxVal;
                    }else if(value.value < minVal)
                    {
                        data.value = minVal;
                    }else
                    {
                        data.value = value.value;
                    }
                    m_viewMeasurements[i].rszGraph.insertMulti(data.key,data);
                }
                m_rsWidget->graph(i*3+3)->setData(&m_viewMeasurements[i].rszGraph, true);
                m_rsWidget->graph(i*3+2)->setData(&m_viewMeasurements[i].rsxGraph, true);
                m_rsWidget->graph(i*3+1)->setData(&m_viewMeasurements[i].rsrGraph, true);
            }
        }else if(m_currentTab == "tab_4")//RXpar
        {
            if(m_farEndMeasurement)
            {
                calcFarEnd();
            }
            for(int i = 0; i < m_measurements.length(); ++i)
            {
                QCPDataMap mapr;
                QCPDataMap mapx;
                QCPDataMap mapz;
                m_viewMeasurements[i].rprGraph.clear();
                m_viewMeasurements[i].rpxGraph.clear();
                m_viewMeasurements[i].rpzGraph.clear();
                QCPData data;
                QCPData value;
                double maxVal = m_rpWidget->yAxis->range().upper;
                double minVal = m_rpWidget->yAxis->range().lower;

                QList <double> listr;
                QList <double> listx;
                QList <double> listz;
                if(m_farEndMeasurement)
                {
                    if(m_farEndMeasurement == 1)
                    {
                        mapr = m_farEndMeasurementsSub.at(i).rprGraph;
                        mapx = m_farEndMeasurementsSub.at(i).rpxGraph;
                        mapz = m_farEndMeasurementsSub.at(i).rpzGraph;
                    }else if (m_farEndMeasurement == 2)
                    {
                        mapr = m_farEndMeasurementsAdd.at(i).rprGraph;
                        mapx = m_farEndMeasurementsAdd.at(i).rpxGraph;
                        mapz = m_farEndMeasurementsAdd.at(i).rpzGraph;
                    }
                }else
                {
                    mapr = m_measurements.at(i).rprGraphCalib;
                    mapx = m_measurements.at(i).rpxGraphCalib;
                    mapz = m_measurements.at(i).rpzGraphCalib;
                }
                listr = mapr.keys();
                listx = mapx.keys();
                listz = mapz.keys();
                for(int n = 0; n < listr.length(); ++n)
                {
                    data.key = listr.at(n);
                    value = mapr.value(listr.at(n));
                    if( value.value > maxVal)
                    {
                        data.value = maxVal;
                    }else if(value.value < minVal)
                    {
                        data.value = minVal;
                    }else
                    {
                        data.value = value.value;
                    }
                    m_viewMeasurements[i].rprGraph.insert(data.key,data);

                    data.key = listx.at(n);
                    value = mapx.value(listx.at(n));
                    if( value.value > maxVal)
                    {
                        data.value = maxVal;
                    }else if(value.value < minVal)
                    {
                        data.value = minVal;
                    }else
                    {
                        data.value = value.value;
                    }
                    m_viewMeasurements[i].rpxGraph.insert(data.key,data);

                    data.key = listz.at(n);
                    value = mapz.value(listz.at(n));
                    if( value.value > maxVal)
                    {
                        data.value = maxVal;
                    }else if(value.value < minVal)
                    {
                        data.value = minVal;
                    }else
                    {
                        data.value = value.value;
                    }
                    m_viewMeasurements[i].rpzGraph.insert(data.key,data);
                }
                m_rpWidget->graph(i*3+3)->setData(&m_viewMeasurements[i].rpzGraph, true);
                m_rpWidget->graph(i*3+2)->setData(&m_viewMeasurements[i].rpxGraph, true);
                m_rpWidget->graph(i*3+1)->setData(&m_viewMeasurements[i].rprGraph, true);
            }
        }else if(m_currentTab == "tab_5")//RL
        {
            if(m_farEndMeasurement)
            {
                calcFarEnd();
            }
            for(int i = 0; i < m_measurements.length(); ++i)
            {
                m_rlWidget->graph(i+1)->setData(&m_measurements[i].rlGraphCalib, true);
            }
        }else if(m_currentTab == "tab_6")//TDR
        {
            int len;
            if(m_farEndMeasurement)
            {
                calcFarEnd();
                if(m_farEndMeasurement == 1)
                {
                    len = CalcTdr(&m_farEndMeasurementsSub.last().dataRXCalib);
                    m_tdrWidget->xAxis->setRangeUpper(m_tdrRange);
                    m_tdrWidget->xAxis->setRangeMax(m_tdrRange);
                    double step = m_tdrRange/len;
                    m_farEndMeasurementsSub.last().tdrImpGraph.clear();
                    m_farEndMeasurementsSub.last().tdrStepGraph.clear();
                    m_farEndMeasurementsSub.last().tdrImpGraphFeet.clear();
                    m_farEndMeasurementsSub.last().tdrStepGraphFeet.clear();
                    for(int i = 0; i < len; ++i)
                    {
                        int x = i;
                        int y = m_pdTdrImp[i];
                        QCPData data;
                        data.key = x*step;
                        data.value = y;
                        m_farEndMeasurementsSub.last().tdrImpGraph.insert(data.key,data);
                        data.value = m_pdTdrStep[i];
                        m_farEndMeasurementsSub.last().tdrStepGraph.insert(data.key,data);

                        QCPData dataFeet;
                        //dataFeet.key = x*step/FEETINMETER;
                        dataFeet.key = x*step*FEETINMETER;
                        dataFeet.value = y;
                        m_farEndMeasurementsSub.last().tdrImpGraphFeet.insert(dataFeet.key,dataFeet);
                        dataFeet.value = m_pdTdrStep[i];
                        m_farEndMeasurementsSub.last().tdrStepGraphFeet.insert(dataFeet.key,dataFeet);
                    }
                    if(m_measureSystemMetric)
                    {
                        m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_farEndMeasurementsSub.last().tdrImpGraph, true);
                        m_tdrWidget->graph()->setData(&m_farEndMeasurementsSub.last().tdrStepGraph, true);
                    }else
                    {
                        m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_farEndMeasurementsSub.last().tdrImpGraphFeet, true);
                        m_tdrWidget->graph()->setData(&m_farEndMeasurementsSub.last().tdrStepGraphFeet, true);
                    }
                }else if (m_farEndMeasurement == 2)
                {
                    len = CalcTdr(&m_farEndMeasurementsAdd.last().dataRXCalib);
                    m_tdrWidget->xAxis->setRangeUpper(m_tdrRange);
                    m_tdrWidget->xAxis->setRangeMax(m_tdrRange);
                    double step = m_tdrRange/len;
                    m_farEndMeasurementsAdd.last().tdrImpGraph.clear();
                    m_farEndMeasurementsAdd.last().tdrStepGraph.clear();
                    m_farEndMeasurementsAdd.last().tdrImpGraphFeet.clear();
                    m_farEndMeasurementsAdd.last().tdrStepGraphFeet.clear();
                    for(int i = 0; i < len; ++i)
                    {
                        int x = i;
                        int y = m_pdTdrImp[i];
                        QCPData data;
                        data.key = x*step;
                        data.value = y;
                        m_farEndMeasurementsAdd.last().tdrImpGraph.insert(data.key,data);
                        data.value = m_pdTdrStep[i];
                        m_farEndMeasurementsAdd.last().tdrStepGraph.insert(data.key,data);

                        QCPData dataFeet;
                        //dataFeet.key = x*step/FEETINMETER;
                        dataFeet.key = x*step*FEETINMETER;
                        dataFeet.value = y;
                        m_farEndMeasurementsAdd.last().tdrImpGraphFeet.insert(dataFeet.key,dataFeet);
                        dataFeet.value = m_pdTdrStep[i];
                        m_farEndMeasurementsAdd.last().tdrStepGraphFeet.insert(dataFeet.key,dataFeet);
                    }
                    if(m_measureSystemMetric)
                    {
                        m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_farEndMeasurementsAdd.last().tdrImpGraph, true);
                        m_tdrWidget->graph()->setData(&m_farEndMeasurementsAdd.last().tdrStepGraph, true);
                    }else
                    {
                        m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_farEndMeasurementsAdd.last().tdrImpGraphFeet, true);
                        m_tdrWidget->graph()->setData(&m_farEndMeasurementsAdd.last().tdrStepGraphFeet, true);
                    }
                }
            }else
            {
                len = CalcTdr(&m_measurements.last().dataRXCalib);
                m_tdrWidget->xAxis->setRangeUpper(m_tdrRange);
                m_tdrWidget->xAxis->setRangeMax(m_tdrRange);
                double step = m_tdrRange/len;
                m_measurements.last().tdrImpGraph.clear();
                m_measurements.last().tdrStepGraph.clear();
                m_measurements.last().tdrImpGraphFeet.clear();
                m_measurements.last().tdrStepGraphFeet.clear();
                for(int i = 0; i < len; ++i)
                {
                    int x = i;
                    int y = m_pdTdrImp[i];
                    QCPData data;
                    data.key = x*step;
                    data.value = y;
                    m_measurements.last().tdrImpGraph.insert(data.key,data);
                    data.value = m_pdTdrStep[i];
                    m_measurements.last().tdrStepGraph.insert(data.key,data);

                    QCPData dataFeet;
                    //dataFeet.key = x*step/FEETINMETER;
                    dataFeet.key = x*step*FEETINMETER;
                    dataFeet.value = y;
                    m_measurements.last().tdrImpGraphFeet.insert(dataFeet.key,dataFeet);
                    dataFeet.value = m_pdTdrStep[i];
                    m_measurements.last().tdrStepGraphFeet.insert(dataFeet.key,dataFeet);
                }
                if(m_measureSystemMetric)
                {
                    m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_measurements.last().tdrImpGraph, true);
                    m_tdrWidget->graph()->setData(&m_measurements.last().tdrStepGraph, true);
                }else
                {
                    m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_measurements.last().tdrImpGraphFeet, true);
                    m_tdrWidget->graph()->setData(&m_measurements.last().tdrStepGraphFeet, true);
                }
            }
        }else if(m_currentTab == "tab_7")//Smith
        {
            if(m_farEndMeasurement)
            {
                calcFarEnd();
                if(m_farEndMeasurement == 1)
                {
                    for(int i = 0; i < m_measurements.length(); ++i)
                    {
                        m_measurements[i].smithCurve->setData(&m_farEndMeasurementsSub[i].smithGraph,true);
                    }
                }else if(m_farEndMeasurement == 2)
                {
                    for(int i = 0; i < m_measurements.length(); ++i)
                    {
                        m_measurements[i].smithCurve->setData(&m_farEndMeasurementsAdd[i].smithGraph,true);
                    }
                }
            }else
            {
                for(int i = 0; i < m_measurements.length(); ++i)
                {
                    m_measurements[i].smithCurve->setData(&m_measurements[i].smithGraphCalib,true);
                }
            }
        }
    }else
    {
        if( m_currentTab == "tab_1")//SWR
        {
            if(m_farEndMeasurement)
            {
                calcFarEnd();
            }
            for(int i = 0; i < m_measurements.length(); ++i)
            {
                m_viewMeasurements[i].swrGraph.clear();

                QCPDataMap map = m_measurements[i].swrGraph;
                QList <double> list = map.keys();
                QCPData data;
                QCPData viewData;
                double maxSwr = m_swrWidget->yAxis->range().upper;
                for(int n = 0; n < list.length(); ++n)
                {
                    data.key = list.at(n);
                    viewData = map.value(list.at(n));
                    if( viewData.value > maxSwr || viewData.value < 1)
                    {
                        data.value = maxSwr;
                    }else
                    {
                        data.value = viewData.value;
                    }
                    m_viewMeasurements[i].swrGraph.insert(data.key,data);
                }
                m_swrWidget->graph(i+1)->setData(&m_viewMeasurements[i].swrGraph, true);
            }
        }else if(m_currentTab == "tab_2")//Phase
        {
            if(m_farEndMeasurement == 1)
            {
                calcFarEnd();
                for(int i = 0; i < m_measurements.length(); ++i)
                {
                    m_phaseWidget->graph(i+1)->setData(&m_farEndMeasurementsSub[i].phaseGraph, true);
                }
            }else if(m_farEndMeasurement == 1)
            {
                calcFarEnd();
                for(int i = 0; i < m_measurements.length(); ++i)
                {
                    m_phaseWidget->graph(i+1)->setData(&m_farEndMeasurementsAdd[i].phaseGraph, true);
                }
            }else
            {
                for(int i = 0; i < m_measurements.length(); ++i)
                {
                    m_phaseWidget->graph(i+1)->setData(&m_measurements[i].phaseGraph, true);
                }
            }
        }else if(m_currentTab == "tab_3")//RX
        {
            calcFarEnd();
            for(int i = 0; i < m_measurements.length(); ++i)
            {
                QCPDataMap mapr;
                QCPDataMap mapx;
                QCPDataMap mapz;
                m_viewMeasurements[i].rsrGraph.clear();
                m_viewMeasurements[i].rsxGraph.clear();
                m_viewMeasurements[i].rszGraph.clear();
                QCPData data;
                QCPData value;
                double maxVal = m_rsWidget->yAxis->range().upper;
                double minVal = m_rsWidget->yAxis->range().lower;

                QList <double> listr;
                QList <double> listx;
                QList <double> listz;
                if(m_farEndMeasurement)
                {
                    if(m_farEndMeasurement == 1)
                    {
                        mapr = m_farEndMeasurementsSub.at(i).rsrGraph;
                        mapx = m_farEndMeasurementsSub.at(i).rsxGraph;
                        mapz = m_farEndMeasurementsSub.at(i).rszGraph;
                    }else if (m_farEndMeasurement == 2)
                    {
                        mapr = m_farEndMeasurementsAdd.at(i).rsrGraph;
                        mapx = m_farEndMeasurementsAdd.at(i).rsxGraph;
                        mapz = m_farEndMeasurementsAdd.at(i).rszGraph;
                    }
                }else
                {
                    mapr = m_measurements.at(i).rsrGraph;
                    mapx = m_measurements.at(i).rsxGraph;
                    mapz = m_measurements.at(i).rszGraph;
                }

                listr = mapr.keys();
                listx = mapx.keys();
                listz = mapz.keys();
                for(int n = 0; n < listr.length(); ++n)
                {
                    data.key = listr.at(n);
                    value = mapr.value(listr.at(n));
                    if( value.value > maxVal)
                    {
                        data.value = maxVal;
                    }else if(value.value < minVal)
                    {
                        data.value = minVal;
                    }else
                    {
                        data.value = value.value;
                    }
                    m_viewMeasurements[i].rsrGraph.insert(data.key,data);

                    data.key = listx.at(n);
                    value = mapx.value(listx.at(n));
                    if( value.value > maxVal)
                    {
                        data.value = maxVal;
                    }else if(value.value < minVal)
                    {
                        data.value = minVal;
                    }else
                    {
                        data.value = value.value;
                    }
                    m_viewMeasurements[i].rsxGraph.insertMulti(data.key,data);

                    data.key = listz.at(n);
                    value = mapz.value(listz.at(n));
                    if( value.value > maxVal)
                    {
                        data.value = maxVal;
                    }else if(value.value < minVal)
                    {
                        data.value = minVal;
                    }else
                    {
                        data.value = value.value;
                    }
                    m_viewMeasurements[i].rszGraph.insertMulti(data.key,data);
                }
                m_rsWidget->graph(i*3+3)->setData(&m_viewMeasurements[i].rszGraph, true);
                m_rsWidget->graph(i*3+2)->setData(&m_viewMeasurements[i].rsxGraph, true);
                m_rsWidget->graph(i*3+1)->setData(&m_viewMeasurements[i].rsrGraph, true);
            }
        }else if(m_currentTab == "tab_4")//RXpar
        {
            if(m_farEndMeasurement)
            {
                calcFarEnd();
            }
            for(int i = 0; i < m_measurements.length(); ++i)
            {
                QCPDataMap rMap;
                QCPDataMap xMap;
                QCPDataMap zMap;
                m_viewMeasurements[i].rprGraph.clear();
                m_viewMeasurements[i].rpxGraph.clear();
                m_viewMeasurements[i].rpzGraph.clear();
                QCPData data;
                QCPData value;
                double maxVal = m_rpWidget->yAxis->range().upper;
                double minVal = m_rpWidget->yAxis->range().lower;

                QList <double> rKeys;
                QList <double> xKeys;
                QList <double> zKeys;
                if(m_farEndMeasurement)
                {
                    if(m_farEndMeasurement == 1)
                    {
                        rMap = m_farEndMeasurementsSub.at(i).rprGraph;
                        xMap = m_farEndMeasurementsSub.at(i).rpxGraph;
                        zMap = m_farEndMeasurementsSub.at(i).rpzGraph;
                    }else if (m_farEndMeasurement == 2)
                    {
                        rMap = m_farEndMeasurementsAdd.at(i).rprGraph;
                        xMap = m_farEndMeasurementsAdd.at(i).rpxGraph;
                        zMap = m_farEndMeasurementsAdd.at(i).rpzGraph;
                    }
                }else
                {
                    rMap = m_measurements.at(i).rprGraph;
                    xMap = m_measurements.at(i).rpxGraph;
                    zMap = m_measurements.at(i).rpzGraph;
                }
                rKeys = rMap.keys();
                xKeys = xMap.keys();
                zKeys = zMap.keys();
                for(int n = 0; n < rKeys.length(); ++n)
                {
                    data.key = rKeys.at(n);
                    value = rMap.value(rKeys.at(n));
                    if( value.value > maxVal)
                    {
                        data.value = maxVal;
                    }else if(value.value < minVal)
                    {
                        data.value = minVal;
                    }else
                    {
                        data.value = value.value;
                    }
                    m_viewMeasurements[i].rprGraph.insert(data.key,data);

                    data.key = xKeys.at(n);
                    value = xMap.value(xKeys.at(n));
                    if( value.value > maxVal)
                    {
                        data.value = maxVal;
                    }else if(value.value < minVal)
                    {
                        data.value = minVal;
                    }else
                    {
                        data.value = value.value;
                    }
                    m_viewMeasurements[i].rpxGraph.insert(data.key,data);

                    data.key = zKeys.at(n);
                    value = zMap.value(zKeys.at(n));
                    if( value.value > maxVal)
                    {
                        data.value = maxVal;
                    }else if(value.value < minVal)
                    {
                        data.value = minVal;
                    }else
                    {
                        data.value = value.value;
                    }
                    m_viewMeasurements[i].rpzGraph.insert(data.key,data);
                }
                m_rpWidget->graph(i*3+3)->setData(&m_viewMeasurements[i].rpzGraph, true);
                m_rpWidget->graph(i*3+2)->setData(&m_viewMeasurements[i].rpxGraph, true);
                m_rpWidget->graph(i*3+1)->setData(&m_viewMeasurements[i].rprGraph, true);
            }
        }else if(m_currentTab == "tab_5")//RL
        {
            if(m_farEndMeasurement)
            {
                calcFarEnd();
            }
            for(int i = 0; i < m_measurements.length(); ++i)
            {
                m_rlWidget->graph(i+1)->setData(&m_measurements[i].rlGraph, true);
            }
        }else if(m_currentTab == "tab_6")//TDR
        {
            int len;
            if(m_farEndMeasurement)
            {
                calcFarEnd();
                if(m_farEndMeasurement == 1)
                {
                    len = CalcTdr(&m_farEndMeasurementsSub.last().dataRX);
                    m_tdrWidget->xAxis->setRangeUpper(m_tdrRange);
                    m_tdrWidget->xAxis->setRangeMax(m_tdrRange);
                    double step = m_tdrRange/len;
                    m_farEndMeasurementsSub.last().tdrImpGraph.clear();
                    m_farEndMeasurementsSub.last().tdrStepGraph.clear();
                    m_farEndMeasurementsSub.last().tdrImpGraphFeet.clear();
                    m_farEndMeasurementsSub.last().tdrStepGraphFeet.clear();
                    for(int i = 0; i < len; ++i)
                    {
                        int x = i;
                        int y = m_pdTdrImp[i];
                        QCPData data;
                        data.key = x*step;
                        data.value = y;
                        m_farEndMeasurementsSub.last().tdrImpGraph.insert(data.key,data);
                        data.value = m_pdTdrStep[i];
                        m_farEndMeasurementsSub.last().tdrStepGraph.insert(data.key,data);

                        QCPData dataFeet;
                        //dataFeet.key = x*step/FEETINMETER;
                        dataFeet.key = x*step*FEETINMETER;
                        dataFeet.value = y;
                        m_farEndMeasurementsSub.last().tdrImpGraphFeet.insert(dataFeet.key,dataFeet);
                        dataFeet.value = m_pdTdrStep[i];
                        m_farEndMeasurementsSub.last().tdrStepGraphFeet.insert(dataFeet.key,dataFeet);
                    }
                    if(m_measureSystemMetric)
                    {
                        m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_farEndMeasurementsSub.last().tdrImpGraph, true);
                        m_tdrWidget->graph()->setData(&m_farEndMeasurementsSub.last().tdrStepGraph, true);
                    }else
                    {
                        m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_farEndMeasurementsSub.last().tdrImpGraphFeet, true);
                        m_tdrWidget->graph()->setData(&m_farEndMeasurementsSub.last().tdrStepGraphFeet, true);
                    }
                }else if (m_farEndMeasurement == 2)
                {
                    len = CalcTdr(&m_farEndMeasurementsAdd.last().dataRX);
                    m_tdrWidget->xAxis->setRangeUpper(m_tdrRange);
                    m_tdrWidget->xAxis->setRangeMax(m_tdrRange);
                    double step = m_tdrRange/len;
                    m_farEndMeasurementsAdd.last().tdrImpGraph.clear();
                    m_farEndMeasurementsAdd.last().tdrStepGraph.clear();
                    m_farEndMeasurementsAdd.last().tdrImpGraphFeet.clear();
                    m_farEndMeasurementsAdd.last().tdrStepGraphFeet.clear();
                    for(int i = 0; i < len; ++i)
                    {
                        int x = i;
                        int y = m_pdTdrImp[i];
                        QCPData data;
                        data.key = x*step;
                        data.value = y;
                        m_farEndMeasurementsAdd.last().tdrImpGraph.insert(data.key,data);
                        data.value = m_pdTdrStep[i];
                        m_farEndMeasurementsAdd.last().tdrStepGraph.insert(data.key,data);

                        QCPData dataFeet;
                        //dataFeet.key = x*step/FEETINMETER;
                        dataFeet.key = x*step*FEETINMETER;
                        dataFeet.value = y;
                        m_farEndMeasurementsAdd.last().tdrImpGraphFeet.insert(dataFeet.key,dataFeet);
                        dataFeet.value = m_pdTdrStep[i];
                        m_farEndMeasurementsAdd.last().tdrStepGraphFeet.insert(dataFeet.key,dataFeet);
                    }
                    if(m_measureSystemMetric)
                    {
                        m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_farEndMeasurementsAdd.last().tdrImpGraph, true);
                        m_tdrWidget->graph()->setData(&m_farEndMeasurementsAdd.last().tdrStepGraph, true);
                    }else
                    {
                        m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_farEndMeasurementsAdd.last().tdrImpGraphFeet, true);
                        m_tdrWidget->graph()->setData(&m_farEndMeasurementsAdd.last().tdrStepGraphFeet, true);
                    }
                }
            }else
            {
                len = CalcTdr(&m_measurements.last().dataRX);
                m_tdrWidget->xAxis->setRangeMax(m_tdrRange);
                m_tdrWidget->xAxis->setRangeUpper(m_tdrRange);
                double step = m_tdrRange/len;
                m_measurements.last().tdrImpGraph.clear();
                m_measurements.last().tdrStepGraph.clear();
                m_measurements.last().tdrImpGraphFeet.clear();
                m_measurements.last().tdrStepGraphFeet.clear();
                for(int i = 0; i < len; ++i)
                {
                    int x = i;
                    QCPData data;
                    data.key = x*step;
                    data.value = m_pdTdrImp[i];
                    m_measurements.last().tdrImpGraph.insert(data.key,data);
                    data.value = m_pdTdrStep[i];
                    m_measurements.last().tdrStepGraph.insert(data.key,data);

                    QCPData dataFeet;
                    //dataFeet.key = x*step/FEETINMETER;
                    dataFeet.key = x*step*FEETINMETER;
                    dataFeet.value = m_pdTdrImp[i];
                    m_measurements.last().tdrImpGraphFeet.insert(dataFeet.key,dataFeet);
                    dataFeet.value = m_pdTdrStep[i];
                    m_measurements.last().tdrStepGraphFeet.insert(dataFeet.key,dataFeet);
                }
                if(m_measureSystemMetric)
                {
                    m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_measurements.last().tdrImpGraph, true);
                    m_tdrWidget->graph()->setData(&m_measurements.last().tdrStepGraph, true);
                }else
                {
                    m_tdrWidget->graph(m_tdrWidget->graphCount()-2)->setData(&m_measurements.last().tdrImpGraphFeet, true);
                    m_tdrWidget->graph()->setData(&m_measurements.last().tdrStepGraphFeet, true);
                }
            }

        }else if(m_currentTab == "tab_7")//Smith
        {
            if(m_farEndMeasurement)
            {
                calcFarEnd();
                if(m_farEndMeasurement == 1)
                {
                    for(int i = 0; i < m_measurements.length(); ++i)
                    {
                        m_measurements[i].smithCurve->setData(&m_farEndMeasurementsSub[i].smithGraph,true);
                    }
                }else if(m_farEndMeasurement == 2)
                {
                    for(int i = 0; i < m_measurements.length(); ++i)
                    {
                        m_measurements[i].smithCurve->setData(&m_farEndMeasurementsAdd[i].smithGraph,true);
                    }
                }
            }else
            {
                for(int i = 0; i < m_measurements.length(); ++i)
                {
                    m_measurements[i].smithCurve->setData(&m_measurements[i].smithGraphView,true);
                }
            }
        }
    }
    replot();
}

void Measurements::replot()
{
    if(m_currentTab == "tab_1")
    {
        m_swrWidget->replot();
    }else if(m_currentTab == "tab_2")
    {
        m_phaseWidget->replot();
    }else if(m_currentTab == "tab_3")
    {
        m_rsWidget->replot();
    }else if(m_currentTab == "tab_4")
    {
        m_rpWidget->replot();
    }else if(m_currentTab == "tab_5")
    {
        m_rlWidget->replot();
    }else if(m_currentTab == "tab_6")
    {
        m_tdrWidget->replot();
    }else if(m_currentTab == "tab_7")
    {
        m_smithWidget->replot();
    }
}

void Measurements::NormRXtoSmithPoint(double Rnorm, double Xnorm, double &x, double &y)
{
    double Denom = (Rnorm+1)*(Rnorm+1)+Xnorm*Xnorm;
    double RhoReal = ((Rnorm-1)*(Rnorm+1)+Xnorm*Xnorm)/Denom;
    double RhoImag = 2*Xnorm/Denom;

    x = RhoReal*6;// 6 - radius
    y = RhoImag*6;// 6 - radius
}

void Measurements::drawSmithImage (void)
{
    QPen pen;
    pen.setColor(Qt::black);
#define ROUND_DOTS_NUM 360
    QCPCurve *round1 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurve *round7 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurve *round2 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurve *round3 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurve *round4 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurve *round5 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurve *round6 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);

    QCPCurveDataMap map1;
    QCPCurveDataMap map2;
    QCPCurveDataMap map3;
    QCPCurveDataMap map4;
    QCPCurveDataMap map5;
    QCPCurveDataMap map6;
    QCPCurveDataMap map7;
    for(double i = 0; i < ROUND_DOTS_NUM; ++i)
    {
        map1.insert(i, QCPCurveData(i, (6 * qCos(i/57.02)), (6 * qSin(i/57.02))));
        map2.insert(i, QCPCurveData(i, (1 + 5 * qCos(i/57.02)), (5 * qSin(i/57.02))));
        map3.insert(i, QCPCurveData(i, (2 + 4 * qCos(i/57.02)), (4 * qSin(i/57.02))));
        map4.insert(i, QCPCurveData(i, (3 + 3 * qCos(i/57.02)), (3 * qSin(i/57.02))));
        map5.insert(i, QCPCurveData(i, (4 + 2 * qCos(i/57.02)), (2 * qSin(i/57.02))));
        map6.insert(i, QCPCurveData(i, (5 + 1 * qCos(i/57.02)), (1 * qSin(i/57.02))));
        map7.insert(i, QCPCurveData(i, (2 * qCos(i/57.02)), (2 * qSin(i/57.02))));
    }
    round1->setData( &map1,true);
    round1->setBrush(QBrush(QColor(0, 0, 255, 20)));
    round7->setData( &map7,true);
    round7->setBrush(QBrush(QColor(255, 255, 255, 255)));
    round2->setData( &map2,true);
    round3->setData( &map3,true);
    round4->setData( &map4,true);
    round5->setData( &map5,true);
    round6->setData( &map6,true);


    QCPCurve *round8 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurve *round9 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurveDataMap map8;
    QCPCurveDataMap map9;
    for(double i = 0; i < 90; ++i)//1 line
    {
        map8.insert(i, QCPCurveData(i, (6 + 6 * qCos((i+179.15)/57.02)), (6 + 6 * qSin((i+179.15)/57.02))));
        map9.insert(i, QCPCurveData(i, (6 + 6 * qCos((i+179.15)/57.02)), (-1)*(6 + 6 * qSin((i+179.15)/57.02))));
    }
    round8->setData( &map8,true);
    round9->setData( &map9,true);

    QCPCurve *round10 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurve *round11 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurveDataMap map10;
    QCPCurveDataMap map11;
    for(double i = 0; i < 53; ++i)//0.5 line
    {
        map10.insert(i, QCPCurveData(i, (6 + 12 * qCos((i+215.85)/57.02)), (12 + 12 * qSin((i+215.85)/57.02))));
        map11.insert(i, QCPCurveData(i, (6 + 12 * qCos((i+215.85)/57.02)), (-1)*(12 + 12 * qSin((i+215.85)/57.02))));
    }
    round10->setData( &map10,true);
    round11->setData( &map11,true);

    QCPCurve *round12 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurve *round13 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurveDataMap map12;
    QCPCurveDataMap map13;
    for(double i = 0; i < 127; ++i)//2 line
    {
        map12.insert(i, QCPCurveData(i, (6 + 3 * qCos((i+142.45)/57.02)), (3 + 3 * qSin((i+142.45)/57.02))));
        map13.insert(i, QCPCurveData(i, (6 + 3 * qCos((i+142.45)/57.02)), (-1)*(3 + 3 * qSin((i+142.45)/57.02))));
    }
    round12->setData( &map12,true);
    round13->setData( &map13,true);

    QCPCurve *round14 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurve *round15 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurveDataMap map14;
    QCPCurveDataMap map15;
    for(double i = 0; i < 151; ++i)// 5 line
    {
        map14.insert(i, QCPCurveData(i, (6 + 1.2 * qCos((i+112)/57.02)), (1.2 + 1.2 * qSin((i+112)/57.02))));//117.5
        map15.insert(i, QCPCurveData(i, (6 + 1.2 * qCos((i+112)/57.02)), (-1)*(1.2 + 1.2 * qSin((i+112)/57.02))));
    }
    round14->setData( &map14,true);
    round15->setData( &map15,true);

    QCPCurve *round16 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurve *round17 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurveDataMap map16;
    QCPCurveDataMap map17;
    for(double i = 0; i < 23; ++i)//0.2 line
    {
        map16.insert(i, QCPCurveData(i, (6 + 30 * qCos((i+246.19)/57.02)), (30 + 30 * qSin((i+246.19)/57.02))));
        map17.insert(i, QCPCurveData(i, (6 + 30 * qCos((i+246.19)/57.02)), (-1)*(30 + 30 * qSin((i+246.19)/57.02))));
    }
    round16->setData( &map16,true);
    round17->setData( &map17,true);


    // 0 line
    QCPCurve *round18 = new QCPCurve(m_smithWidget->xAxis, m_smithWidget->yAxis);
    QCPCurveDataMap map18;
    map18.insert(0, QCPCurveData(0, -6, 0));
    map18.insert(1, QCPCurveData(1, 6, 0));
    round18->setData( &map18,true);


    round1->setPen(pen);
    round2->setPen(pen);
    round3->setPen(pen);
    round4->setPen(pen);
    round5->setPen(pen);
    round6->setPen(pen);
    round7->setPen(pen);
    round8->setPen(pen);
    round9->setPen(pen);
    round10->setPen(pen);
    round11->setPen(pen);
    round12->setPen(pen);
    round13->setPen(pen);
    round14->setPen(pen);
    round15->setPen(pen);
    round16->setPen(pen);
    round17->setPen(pen);
    round18->setPen(pen);

    QFont serifFont("Times", 12, QFont::Bold);
    QCPItemText *center5 = new QCPItemText(m_smithWidget);
    QCPItemText *center2 = new QCPItemText(m_smithWidget);
    QCPItemText *center1 = new QCPItemText(m_smithWidget);
    QCPItemText *center05 = new QCPItemText(m_smithWidget);
    QCPItemText *center02 = new QCPItemText(m_smithWidget);
    QCPItemText *center0 = new QCPItemText(m_smithWidget);

    QCPItemText *up5 = new QCPItemText(m_smithWidget);
    QCPItemText *up2 = new QCPItemText(m_smithWidget);
    QCPItemText *up1 = new QCPItemText(m_smithWidget);
    QCPItemText *up05 = new QCPItemText(m_smithWidget);
    QCPItemText *up02 = new QCPItemText(m_smithWidget);

    QCPItemText *down5 = new QCPItemText(m_smithWidget);
    QCPItemText *down2 = new QCPItemText(m_smithWidget);
    QCPItemText *down1 = new QCPItemText(m_smithWidget);
    QCPItemText *down05 = new QCPItemText(m_smithWidget);
    QCPItemText *down02 = new QCPItemText(m_smithWidget);

    center5->position->setCoords(4.2, -0.3);
    center5->setText("5");
    center5->setFont(serifFont);
    center5->setColor(QColor(0, 0, 0, 150));

    center2->position->setCoords(2.2, -0.3);
    center2->setText("2");
    center2->setFont(serifFont);
    center2->setColor(QColor(0, 0, 0, 150));

    center1->position->setCoords(0.2, -0.3);
    center1->setText("1");
    center1->setFont(serifFont);
    center1->setColor(QColor(0, 0, 0, 150));

    center05->position->setCoords(-2.3, -0.3);
    center05->setText("0.5");
    center05->setFont(serifFont);
    center05->setColor(QColor(0, 0, 0, 150));

    center02->position->setCoords(-4.3, -0.3);
    center02->setText("0.2");
    center02->setFont(serifFont);
    center02->setColor(QColor(0, 0, 0, 150));

    center0->position->setCoords(-6.5, 0);
    center0->setText("0");
    center0->setFont(serifFont);
    center0->setColor(QColor(0, 0, 0, 150));

    up5->position->setCoords(6, 2.5);
    up5->setText("5");
    up5->setFont(serifFont);
    up5->setColor(QColor(0, 0, 0, 150));

    up2->position->setCoords(3.8, 5.4);
    up2->setText("2");
    up2->setFont(serifFont);
    up2->setColor(QColor(0, 0, 0, 150));

    up1->position->setCoords(0, 6.5);
    up1->setText("1");
    up1->setFont(serifFont);
    up1->setColor(QColor(0, 0, 0, 150));

    up05->position->setCoords(-4, 5.4);
    up05->setText("0.5");
    up05->setFont(serifFont);
    up05->setColor(QColor(0, 0, 0, 150));

    up02->position->setCoords(-6.5, 2.5);
    up02->setText("0.2");
    up02->setFont(serifFont);
    up02->setColor(QColor(0, 0, 0, 150));

    down5->position->setCoords(6, -2.5);
    down5->setText("-5");
    down5->setFont(serifFont);
    down5->setColor(QColor(0, 0, 0, 150));

    down2->position->setCoords(3.8, -5.4);
    down2->setText("-2");
    down2->setFont(serifFont);
    down2->setColor(QColor(0, 0, 0, 150));

    down1->position->setCoords(0, -6.5);
    down1->setText("-1");
    down1->setFont(serifFont);
    down1->setColor(QColor(0, 0, 0, 150));

    down05->position->setCoords(-4, -5.4);
    down05->setText("-0.5");
    down05->setFont(serifFont);
    down05->setColor(QColor(0, 0, 0, 150));

    down02->position->setCoords(-6.5, -2.5);
    down02->setText("-0.2");
    down02->setFont(serifFont);
    down02->setColor(QColor(0, 0, 0, 150));

}
//Cable-------------------------------------------------------------------------
void Measurements::setCableVelFactor(double value)
{
    m_cableVelFactor = value;
}
//------------------------------------------------------------------------------
void Measurements::setCableResistance(double value)
{
    m_cableResistance = value;
}
//------------------------------------------------------------------------------
void Measurements::setCableLossConductive(double value)
{
    m_cableLossConductive = value;
}
//------------------------------------------------------------------------------
void Measurements::setCableLossDielectric(double value)
{
    m_cableLossDielectric = value;
}
//------------------------------------------------------------------------------
void Measurements::setCableLossFqMHz(double value)
{
    m_cableLossFqMHz = value;
}
//------------------------------------------------------------------------------
void Measurements::setCableLossUnits(int value)
{
    m_cableLossUnits = value;
}
//------------------------------------------------------------------------------
void Measurements::setCableLossAtAnyFq(bool value)
{
    m_cableLossAtAnyFq = value;
}
//------------------------------------------------------------------------------
void Measurements::setCableLength(double value)
{
    m_cableLength = value;
}
//------------------------------------------------------------------------------
void Measurements::setCableFarEndMeasurement(int value)
{
    m_farEndMeasurement = value;
}

void Measurements::calcFarEnd(void)
{
    if(m_calibration != NULL)
    {
        int count = m_measurements.length();
        int dataCount;
        QVector <rawData> data;

        for(int i = 0; i < count; ++i)
        {
            if(m_calibration->getCalibrationEnabled())
            {
                dataCount = m_measurements.at(i).dataRXCalib.length();
                data = m_measurements.at(i).dataRXCalib;
            }else
            {
                dataCount = m_measurements.at(i).dataRX.length();
                data = m_measurements.at(i).dataRX;
            }

            if(m_farEndMeasurement==1) // subtract cable
            {
                m_farEndMeasurementsSub[i].dataRX.clear();
                m_farEndMeasurementsSub[i].rsrGraph.clear();
                m_farEndMeasurementsSub[i].rsxGraph.clear();
                m_farEndMeasurementsSub[i].rszGraph.clear();
                m_farEndMeasurementsSub[i].rprGraph.clear();
                m_farEndMeasurementsSub[i].rpxGraph.clear();
                m_farEndMeasurementsSub[i].rpzGraph.clear();
                m_farEndMeasurementsSub[i].phaseGraph.clear();
                m_farEndMeasurementsSub[i].rhoGraph.clear();
                m_farEndMeasurementsSub[i].smithGraph.clear();
            }else if(m_farEndMeasurement==2) // add cable
            {
                m_farEndMeasurementsAdd[i].dataRX.clear();
                m_farEndMeasurementsAdd[i].rsrGraph.clear();
                m_farEndMeasurementsAdd[i].rsxGraph.clear();
                m_farEndMeasurementsAdd[i].rszGraph.clear();
                m_farEndMeasurementsAdd[i].rprGraph.clear();
                m_farEndMeasurementsAdd[i].rpxGraph.clear();
                m_farEndMeasurementsAdd[i].rpzGraph.clear();
                m_farEndMeasurementsAdd[i].phaseGraph.clear();
                m_farEndMeasurementsAdd[i].rhoGraph.clear();
                m_farEndMeasurementsAdd[i].smithGraph.clear();
            }

            double fq;
            double R;
            double X;
            double Rpar;
            double Xpar;
            QCPData qdata;
            for(int n = 0; n < dataCount; ++n)
            {
                fq = data.at(n).fq;
                qdata.key = fq*1000;
                R = data.at(n).r;
                X = data.at(n).x;
                //if (m_farEndMeasurement != 0)
                {
                    double Klen = 1;
                    switch (m_cableLossUnits)
                    {
                        case 0: Klen = 1; break;
                        case 1: Klen = 1*100.0; break;
                        case 2: Klen = 1/FEETINMETER; break;
                        case 3: Klen = 1/FEETINMETER*100.0; break;
                    }

                    Complex Zload = Complex( R, X);

                    double dMatchedLossDb;  // Note that K1/K2 are in dB/100 ft
                    if(!m_cableLossAtAnyFq)
                        dMatchedLossDb = m_cableLossConductive*Klen*sqrt(fq/1000.0) + m_cableLossDielectric*Klen*fq/1000.0;
                    else
                        dMatchedLossDb = m_cableLossConductive*Klen + m_cableLossDielectric*Klen;


            #define NEPER 8.68588963806504        // = 20 / Ln(10)

                    double Alpha = dMatchedLossDb / 100.0 / NEPER; // Nepers (attenuation) per foot
                    double Beta = (2*M_PI * fq/1000.0) / (SPEEDOFLIGHT*FEETINMETER/1000000.0 * m_cableVelFactor); // Radians (phase constant) per foot

                    double Alphal = Alpha * m_cableLength;
                    double Betal = Beta * m_cableLength;

                    if(m_farEndMeasurement==1) // subtract cable
                    {
                        Alphal = -Alphal;
                        Betal = -Betal;

                        if (m_cableLossUnits==0)
                        {
                            Alphal *= FEETINMETER;
                            Betal *= FEETINMETER;
                        }

                        Complex Sinh_gl = Complex( cos(Betal) * sinh(Alphal), sin(Betal) * cosh(Alphal) );
                        Complex Cosh_gl = Complex( cos(Betal) * cosh(Alphal), sin(Betal) * sinh(Alphal) );

                        Complex Zo = Complex(m_cableResistance, -m_cableResistance * (Alpha / Beta));

                        Complex ZIZL = Zo * ( (Zload*Cosh_gl + Zo*Sinh_gl) /  (Zo*Cosh_gl + Zload*Sinh_gl) );

                        R = ZIZL.real();
                        if(R<0.0001)
                            R = 0.0001;
                        X = ZIZL.imag();

                        Rpar = R*(1+X*X/R/R);
                        Xpar = X*(1+R*R/X/X);

                        if (_isnan(R) || (R<0.001) ) {R = 0.01;}
                        if (_isnan(X)) {X = 0;}

                        double Rnorm = R/m_Z0;
                        double Xnorm = X/m_Z0;

                        double Denom = (Rnorm+1)*(Rnorm+1)+Xnorm*Xnorm;
                        double RhoReal = ((Rnorm-1)*(Rnorm+1)+Xnorm*Xnorm)/Denom;
                        double RhoImag = 2*Xnorm/Denom;

                        double RhoPhase = atan2(RhoImag, RhoReal) / M_PI * 180.0;
                        double RhoMod = sqrt(RhoReal*RhoReal+RhoImag*RhoImag);

                        rawData da = data.at(n);
                        da.r = R;
                        da.x = X;
                        m_farEndMeasurementsSub[i].dataRX.append(da);

                        qdata.value = R;
                        m_farEndMeasurementsSub[i].rsrGraph.insert(qdata.key,qdata);
                        qdata.value = X;
                        m_farEndMeasurementsSub[i].rsxGraph.insert(qdata.key,qdata);
                        qdata.value = computeZ(R, X);
                        m_farEndMeasurementsSub[i].rszGraph.insert(qdata.key,qdata);

                        qdata.value = Rpar;
                        m_farEndMeasurementsSub[i].rprGraph.insert(qdata.key,qdata);
                        qdata.value = Xpar;
                        m_farEndMeasurementsSub[i].rpxGraph.insert(qdata.key,qdata);
                        qdata.value = computeZ(R, X);
                        m_farEndMeasurementsSub[i].rpzGraph.insert(qdata.key,qdata);

                        qdata.value = RhoPhase;
                        m_farEndMeasurementsSub[i].phaseGraph.insert(qdata.key,qdata);
                        qdata.value = RhoMod;
                        m_farEndMeasurementsSub[i].rhoGraph.insert(qdata.key,qdata);

                        double pointX,pointY;
                        NormRXtoSmithPoint(R/m_Z0, X/m_Z0, pointX, pointY);
                        int len = m_farEndMeasurementsSub[i].dataRX.length();
                        m_farEndMeasurementsSub[i].smithGraph.insert(len, QCPCurveData(len, pointX, pointY));
                    }else if (m_farEndMeasurement==2) // add cable
                    {
                        if (m_cableLossUnits==0)
                        {
                            Alphal *= FEETINMETER;
                            Betal *= FEETINMETER;
                        }

                        Complex Sinh_gl = Complex( cos(Betal) * sinh(Alphal), sin(Betal) * cosh(Alphal) );
                        Complex Cosh_gl = Complex( cos(Betal) * cosh(Alphal), sin(Betal) * sinh(Alphal) );

                        Complex Zo = Complex(m_cableResistance, -m_cableResistance * (Alpha / Beta));

                        Complex ZIZL = Zo * ( (Zload*Cosh_gl + Zo*Sinh_gl) /  (Zo*Cosh_gl + Zload*Sinh_gl) );

                        R = ZIZL.real();
                        if(R<0.0001)
                            R = 0.0001;
                        X = ZIZL.imag();

                        Rpar = R*(1+X*X/R/R);
                        Xpar = X*(1+R*R/X/X);

                        if (_isnan(R) || (R<0.001) ) {R = 0.01;}
                        if (_isnan(X)) {X = 0;}

                        double Rnorm = R/m_Z0;
                        double Xnorm = X/m_Z0;

                        double Denom = (Rnorm+1)*(Rnorm+1)+Xnorm*Xnorm;
                        double RhoReal = ((Rnorm-1)*(Rnorm+1)+Xnorm*Xnorm)/Denom;
                        double RhoImag = 2*Xnorm/Denom;

                        double RhoPhase = atan2(RhoImag, RhoReal) / M_PI * 180.0;
                        double RhoMod = sqrt(RhoReal*RhoReal+RhoImag*RhoImag);

                        rawData da = data.at(n);
                        da.r = R;
                        da.x = X;
                        m_farEndMeasurementsAdd[i].dataRX.append(da);

                        qdata.value = R;
                        m_farEndMeasurementsAdd[i].rsrGraph.insert(qdata.key,qdata);
                        qdata.value = X;
                        m_farEndMeasurementsAdd[i].rsxGraph.insert(qdata.key,qdata);
                        qdata.value = computeZ(R, X);
                        m_farEndMeasurementsAdd[i].rszGraph.insert(qdata.key,qdata);

                        qdata.value = Rpar;
                        m_farEndMeasurementsAdd[i].rprGraph.insert(qdata.key,qdata);
                        qdata.value = Xpar;
                        m_farEndMeasurementsAdd[i].rpxGraph.insert(qdata.key,qdata);
                        qdata.value = computeZ(R, X);
                        m_farEndMeasurementsAdd[i].rpzGraph.insert(qdata.key,qdata);

                        qdata.value = RhoPhase;
                        m_farEndMeasurementsAdd[i].phaseGraph.insert(qdata.key,qdata);
                        qdata.value = RhoMod;
                        m_farEndMeasurementsAdd[i].rhoGraph.insert(qdata.key,qdata);

                        double pointX,pointY;
                        NormRXtoSmithPoint(R/m_Z0, X/m_Z0, pointX, pointY);
                        int len = m_farEndMeasurementsAdd[i].dataRX.length();
                        m_farEndMeasurementsAdd[i].smithGraph.insert(len, QCPCurveData(len, pointX, pointY));
                    }
                }
            }
        }
    }
}

int Measurements::CalcTdr2(QVector <rawData> *data)
{
    #define   MIN_VECTOR_SIZE    500
    #define   MAX_VECTOR_SIZE    16384    						// value must be by Radix2 !!!
    #define   PI                 (double)3.14159265358979323846
    #define   LIGHT_SPEED        (double)299792458.0

    //------  ZERO_STYLEs
    #define   ARTIFICAL_12        0  // phase of zero is calculated from 1 & 2 bin
    #define   FROM_LOWEST         1  // phase of zero is calculated from lowest frequency posible, this point must be located at zero index of R,X vector

    #define  FT_DIRECT        			-1    // Direct transform.
    #define  FT_INVERSE       			 1    // Inverse transform.

    //---------STEP flags
    #define STEPUNITY   		0
    #define STEPIMPEDANCE		1
    //-------------------

    double GK_Re[MAX_VECTOR_SIZE*2];           // vector size, doubling for mirroring
    double GK_Im[MAX_VECTOR_SIZE*2];           // vector size, doubling for mirroring

    int vector_length;
    int radix2length;
    int  log2N;

    int cnt;
    double R;
    double X;
    double t_Re;
    double t_Im;
    double hamm;
    if(data->length() < MIN_VECTOR_SIZE+1)
    {
        return 0;
    }
    if(data->length() > MAX_VECTOR_SIZE)
    {
        return 0;
    }
    //-----------cache vars, try to find radix2 length
    vector_length = data->length();

    radix2length = DTF_FindRadix2Length(vector_length, &log2N);

    if(radix2length == 0)
    {
        return 0;
    }
    //--------------

    //-------generating GK, calculation, windowing, padding and mirroring
    cnt=1;  //starting from first bin, zero will be inserted later
    do{
        if(cnt < vector_length)
        {
            //----GE calc
            X = data->at(cnt).x;
            R = data->at(cnt).r;
            t_Re = (R*R - m_Z0 * m_Z0 +X*X)/((R+ m_Z0)*(R+m_Z0)+X*X);
            t_Im = (2*m_Z0*X)/((R+m_Z0)*(R+m_Z0)+X*X);
            //----window calc,hamming, one half, positive frequency part
            hamm = 0.53836 - 0.46146 * cos(PI * (1.0 + ((double)cnt / (double)vector_length)));
            // zero case is drop out and it equals 1.0 and applies to zero bin

            t_Re *= hamm;
            t_Im *= hamm;
            //----positive frequencies
            GK_Re[cnt] = t_Re;
            GK_Im[cnt] = t_Im;
            //---negative frequencies, mirror
            GK_Re[radix2length*2-cnt]=t_Re;
            GK_Im[radix2length*2-cnt]=-t_Im;
        }else
        {
            //---padding to radix2length by zeroes
            GK_Re[cnt] = 0.0;
            GK_Im[cnt] = 0.0;
            GK_Re[radix2length*2-cnt]=0.0;
            GK_Im[radix2length*2-cnt]=0.0;
        }
    }while(++cnt<radix2length);

    //-----put N/2 value
    GK_Re[radix2length] = 0.0;
    GK_Im[radix2length] = 0.0;


    //-----deliver zero bin
    double p1;
    double p2;
    double mod;

    p1 = sqrt(GK_Re[1]*GK_Re[1]+GK_Im[1]*GK_Im[1]);
    p2 = sqrt(GK_Re[2]*GK_Re[2]+GK_Im[2]*GK_Im[2]);
    mod = 2*p1-p2;    // module is linear extrapolated from p1,p2

    p1 = atan2(GK_Re[1], GK_Im[1]);
    p2 = atan2(GK_Re[2], GK_Im[2]);

    double phase = p2-p1;

    while(phase > PI) {phase=phase-2*PI;}
    while(phase <-PI) {phase=phase+2*PI;}

    p2 = phase;

    phase = p1-p2;

    while(phase > PI) {phase=phase-2*PI;}
    while(phase <-PI) {phase=phase+2*PI;}

    p2 = phase;

    if((p2 < -PI/2)||(p2 > PI/2))
    {
        GK_Re[0] = mod * (-1.0);
    }else
    {
        GK_Re[0] = mod * (1.0);
    }

    GK_Im[0]=0.0;
    //------------------------------------------

    FFT2(GK_Im, GK_Re, radix2length*2, (log2N+1), FT_DIRECT);

    for(int i = 0; i < radix2length*2; ++i)
    {
        m_pdTdrImp[i] = GK_Re[i];
    }

    //dtf_do_step
    qint16 end,cnt1;
    double acc;
    //-------
    acc = 0.0;
    end = radix2length*2;
    cnt1 = 0;
    do{
        acc = acc + GK_Re[cnt1];
        m_pdTdrStep[cnt1] = acc;
    }while(++cnt1<end);

    //DTF_CalcResolution
    double tmp;
//    vector_length = UTIL_limit_u16(MIN_VECTOR_SIZE, MAX_VECTOR_SIZE, vector_length);
    tmp = (data->last().fq/1000 - data->first().fq/1000)/(vector_length-1);   // resolution by original set
    tmp = tmp * radix2length;                // new Fmax by radix2length
    tmp = (LIGHT_SPEED * m_cableVelFactor)/(( tmp - data->first().fq/1000)*2*1000000.0);

    m_tdrResolution = tmp*1000;

    m_tdrRange = m_tdrResolution * radix2length/1000000;


    return end;
}

qint16 Measurements::DTF_FindRadix2Length(qint16 length, int *log2N)
{
    qint8 log;
    qint16 result =0x0010;   //start from 16
    log = 4;
    while(result <= MAX_VECTOR_SIZE)
    {
        if (result >= length)
        {
            (*log2N)=log;
            return result;
        }
        result<<=1;
        log++;
    }
    return 0;
}

void  Measurements::FFT2(double *Rdat, double *Idat, int N, int LogN, int Ft_Flag)
{
  register int  i, j, n, k, io, ie, in, nn;
  double        ru, iu, rtp, itp, rtq, itq, rw, iw, sr;

  static const double Rcoef[14] =
  {  -1.0000000000000000,  0.0000000000000000,  0.7071067811865475,
      0.9238795325112867,  0.9807852804032304,  0.9951847266721969,
      0.9987954562051724,  0.9996988186962042,  0.9999247018391445,
      0.9999811752826011,  0.9999952938095761,  0.9999988234517018,
      0.9999997058628822,  0.9999999264657178
  };
  static const double Icoef[14] =
  {   0.0000000000000000, -1.0000000000000000, -0.7071067811865474,
     -0.3826834323650897, -0.1950903220161282, -0.0980171403295606,
     -0.0490676743274180, -0.0245412285229122, -0.0122715382857199,
     -0.0061358846491544, -0.0030679567629659, -0.0015339801862847,
     -0.0007669903187427, -0.0003834951875714
  };

  nn = N >> 1;
  ie = N;
  for(n=1; n<=LogN; n++)
  {
    rw = Rcoef[LogN - n];
    iw = Icoef[LogN - n];
    in = ie >> 1;
    ru = 1.0;
    iu = 0.0;
    for(j=0; j<in; j++)
    {
      for(i=j; i<N; i+=ie)
      {
        io       = i + in;
        rtp      = Rdat[i]  + Rdat[io];
        itp      = Idat[i]  + Idat[io];
        rtq      = Rdat[i]  - Rdat[io];
        itq      = Idat[i]  - Idat[io];
        Rdat[io] = rtq * ru - itq * iu;
        Idat[io] = itq * ru + rtq * iu;
        Rdat[i]  = rtp;
        Idat[i]  = itp;
      }

      if(Ft_Flag == FT_INVERSE) iw = -iw;
      sr = ru;
      ru = ru * rw - iu * iw;
      iu = iu * rw + sr * iw;
    }
    ie >>= 1;
  }

  for(j=i=1; i<N; i++)
  {
    if(i < j)
    {
      io       = i - 1;
      in       = j - 1;
      rtp      = Rdat[in];
      itp      = Idat[in];
      Rdat[in] = Rdat[io];
      Idat[in] = Idat[io];
      Rdat[io] = rtp;
      Idat[io] = itp;
    }

    k = nn;

    while(k < j)
    {
      j   = j - k;
      k >>= 1;
    }

    j = j + k;
  }

  if(Ft_Flag == FT_INVERSE) return;

  rw = 1.0 / N;

  for(i=0; i<N; i++)
  {
    Rdat[i] *= rw;
    Idat[i] *= rw;
  }

  return;
}


void Measurements::on_translate()
{
    if (m_graphHint != nullptr)
    {
        m_graphHint->setPopupText(tr("Frequency = \n"
                                     "SWR = \n"
                                     "RL = \n"
                                     "Z = \n"
                                     "|Z| = \n"
                                     "|rho| = \n"
                                     "C = \n"
                                     "Zpar = \n"
                                     "Cpar = \n"
                                     "Cable: "));
    }
    if (m_graphBriefHint != nullptr)
    {
        m_graphBriefHint->setName(tr("BriefHint"));
    }

    if (m_tdrWidget->xAxis != nullptr)
    {
        m_tdrWidget->xAxis->setLabel(m_measureSystemMetric ? tr("Length, m") : tr("Length, feet"));
    }
}


