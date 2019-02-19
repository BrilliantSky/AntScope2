#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QFileDialog>
#include <QTimer>
#include <analyzer/analyzer.h>
#include <analyzer/analyzerparameters.h>
#include <crc32.h>
#include <QSettings>
#include <calibration.h>

//#include <shlobj.h>

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
    Q_OBJECT

public:
    explicit Settings(QWidget *parent = 0);
    ~Settings();

    void setAnalyzer(Analyzer * analyzer);
    void setCalibration(Calibration * calibration);
    void setGraphHintChecked(bool checked);
    void setGraphBriefHintChecked(bool checked);
    void setMarkersHintChecked(bool checked);
    void setMeasureSystemMetric(bool state);
    void setZ0(double _Z0);

    void setCableVelFactor(double value);
    double getCableVelFactor(void)const;
    void setCableResistance(double value);
    double getCableResistance(void)const;
    void setCableLossConductive(double value);
    double getCableLossConductive(void)const;
    void setCableLossDielectric(double value);
    double getCableLossDielectric(void)const;
    void setCableLossFqMHz(double value);
    double getCableLossFqMHz(void)const;
    void setCableLossUnits(int value);
    int getCableLossUnits(void)const;
    void setCableLossAtAnyFq(bool value);
    bool getCableLossAtAnyFq(void)const;
    void setCableLength(double value);
    double getCableLength(void)const;
    void setCableFarEndMeasurement(int value);
    int getCableFarEndMeasurement(void)const;
    void setCableIndex(int value);
    int getCableIndex(void)const;
    void setBands(QList<QString> list);

    static QString programDataPath(QString _fileName);
    static QString localDataPath(QString _fileName);
    static QString setIniFile();

    void setFirmwareAutoUpdate(bool checked);
    void setAntScopeAutoUpdate(bool checked);
    void setAntScopeVersion(QString version);
    void setAutoDetectMode(bool state, QString portName);

    void setLanguages(QStringList list, int number);
    void on_translate();
private:
    Ui::Settings *ui;
    Analyzer * m_analyzer;
    Calibration * m_calibration;

    bool m_isComplete;
    QString m_pathToFw;
    QString m_path;

    QTimer * m_generalTimer;

    QSettings * m_settings;

    bool m_markersHintEnabled;
    bool m_graphHintEnabled;
    bool m_graphBriefHintEnabled;

    bool m_onlyOneCalib;

    bool m_metricChecked;
    bool m_americanChecked;

    bool m_manualDetectChecked;
    bool m_autoDetectChecked;
    QString m_manualDetectComString;

    int m_farEndMeasurement;
    static QString iniFilePath;

    QList <QString> m_cablesList;

    void enableButtons(bool enabled);
    void cableActionEnableButtons(bool enabled);
    void openCablesFile(QString path);
    void initCustomizeTab();

signals:
    void paramsChanged();
    void checkUpdatesBtn();
    void autoUpdatesCheckBox(bool);
    void updateBtn(QString);

    void graphHintChecked(bool);
    void graphBriefHintChecked(bool);
    void markersHintChecked(bool);

    void startCalibration();
    void startCalibrationOpen();
    void startCalibrationShort();
    void startCalibrationLoad();

    void openOpenFile(QString);
    void shortOpenFile(QString);
    void loadOpenFile(QString);

    void calibrationEnabled(bool);
    void changeMeasureSystemMetric(bool);

    void Z0Changed(double);

    void cableActionChanged(int);
    void firmwareAutoUpdateStateChanged(bool);
    void antScopeAutoUpdateStateChanged(bool);

    void changedAutoDetectMode(bool);
    void changedSerialPort(QString);

    void languageChanged(int);
    void bandChanged(QString);

private slots:
    void on_browseBtn_clicked();
    void on_checkUpdatesBtn_clicked();
    void on_autoUpdatesCheckBox_clicked(bool checked);
    void on_updateBtn_clicked();
    void on_percentChanged(qint32 percent);
    void findBootloader (void);
    void on_generalTimerTick();
    void on_graphHintCheckBox_clicked(bool checked);
    void on_markersHintCheckBox_clicked(bool checked);
    void on_calibWizard_clicked();
    void on_percentCalibrationChanged(qint32 state, qint32 percent);
    void on_openCalibBtn_clicked();
    void on_shortCalibBtn_clicked();
    void on_loadCalibBtn_clicked();
    void on_turnOnOffBtn_clicked();
    void on_openOpenFileBtn_clicked();
    void on_shortOpenFileBtn_clicked();
    void on_loadOpenFileBtn_clicked();
    void on_measureSystemMetric_clicked(bool checked);
    void on_measureSystemAmerican_clicked(bool checked);
    void on_doNothingBtn_clicked(bool checked);
    void on_subtractCableBtn_clicked(bool checked);
    void on_addCableBtn_clicked(bool checked);
    void on_cableComboBox_currentIndexChanged(int index);
    void on_updateGraphsBtn_clicked();
    void on_aa30bootFound();
    void on_aa30updateComplete();
    void on_graphBriefHintCheckBox_clicked(bool checked);

    void on_autoUpdatesCheckBox(bool checked);
    void on_checkBox_AntScopeAutoUpdate_clicked(bool checked);
    void on_autoDetect_clicked(bool checked);
    void on_manualDetect_clicked(bool checked);
    void on_serialPortComboBox_activated(const QString &arg1);
    void on_languageComboBox_currentIndexChanged(int index);    
    void on_closeButton_clicked();
    void on_bandsComboBox_currentIndexChanged(int index);
    void on_enableCustomizeControls(bool enable);
    void on_addButton();
    void on_removeButton();
    void onApplyButton();
    void on_comboBoxPrototype_currentIndexChanged(int index);
    void on_comboBoxName_currentIndexChanged(int index);
    void on_fqMinFinished();
    void on_fqMaxFinished();
};

#endif // SETTINGS_H
