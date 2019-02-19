#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStyleFactory>
#include <QCheckBox>
#include <QPushButton>
#include <QShortcut>
#include <analyzer/analyzer.h>
#include <qcustomplot.h>
#include <presets.h>
#include <measurements.h>
#include <analyzer/analyzerdata.h>
#include <screenshot.h>
#include <QTimer>
#include <settings.h>
#include <fqsettings.h>
#include <markers.h>
#include <QSettings>
//#include <QQuickItem>
#include <calibration.h>
#include <print.h>
#include <QJsonObject>
#include <export.h>
#include <ctime>
#include <QTranslator>
#include <updater.h>
#include <antscopeupdatedialog.h>

namespace Ui {
class MainWindow;
}
#define LANGUAGES_QUANTITY 4
static QString languages[LANGUAGES_QUANTITY]={
    "English",
    "Русский",
    "Українська",
    "日本語"
};

static QString languages_small[LANGUAGES_QUANTITY]={
    "en",
    "ru",
    "uk",
    "ja"
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

protected:
    void closeEvent(QCloseEvent *event);
    void changeEvent(QEvent* event);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void openFile(QString path);
    QString lastSavePath() { return m_lastSavePath; }
    Analyzer* analyzer() { return m_analyzer; }
    bool isMeasuring() { return analyzer()->isMeasuring(); }

private:

    Ui::MainWindow *ui;
    AnalyzerData *m_analyzerData;
    Analyzer *m_analyzer;
    Screenshot *m_screenshot;

    Presets * m_presets;

    QWidget *m_tab_1;
    QWidget *m_tab_2;
    QWidget *m_tab_3;
    QWidget *m_tab_4;
    QWidget *m_tab_5;
    QWidget *m_tab_6;
    QWidget *m_tab_7;
    QHBoxLayout *m_horizontalLayout_1;
    QHBoxLayout *m_horizontalLayout_2;
    QHBoxLayout *m_horizontalLayout_3;
    QHBoxLayout *m_horizontalLayout_4;
    QHBoxLayout *m_horizontalLayout_5;
    QHBoxLayout *m_horizontalLayout_6;
    QHBoxLayout *m_horizontalLayout_7;
    QCustomPlot *m_swrWidget;
    QCustomPlot *m_phaseWidget;
    QCustomPlot *m_rsWidget;
    QCustomPlot *m_rpWidget;
    QCustomPlot *m_rlWidget;
    QCustomPlot *m_tdrWidget;
    QCustomPlot *m_smithWidget;
    QMap<QString, QCustomPlot *> m_mapWidgets;

    Measurements *m_measurements;
    QVector <QCPItemRect*> m_itemRectList;
    Settings *m_settingsDialog;
    Export *m_exportDialog;
    FqSettings *m_fqSettings;
    Markers *m_markers;
    QSettings *m_settings;
    Calibration *m_calibration;

    Print *m_print;

    bool m_isContinuos;
    int m_dotsNumber;

    QString m_lastSavePath;
    QString m_lastOpenPath;

    bool m_measureSystemMetric;
    double m_Z0;

//    QTimer *m_redrawTimer;
    QTimer *m_1secTimer;

    double m_cableVelFactor;
    double m_cableResistance;
    double m_cableLossConductive;
    double m_cableLossDielectric;
    double m_cableLossFqMHz;
    qint32 m_cableLossUnits;
    qint32 m_cableLossAtAnyFq;
    double m_cableLength;
    qint32 m_farEndMeasurement;
    qint32 m_cableIndex;

    bool m_isRange;

    Updater * m_updater;

    AntScopeUpdateDialog * m_updateDialog;

    bool m_deferredUpdate;

    bool m_autoUpdateEnabled;
    bool m_autoFirmwareUpdateEnabled;

    int m_swrZoomState;
    int m_phaseZoomState;
    int m_rsZoomState;
    int m_rpZoomState;
    int m_rlZoomState;
    int m_tdrZoomState;
    int m_smithZoomState;

    bool m_autoDetectMode;
    QString m_serialPort;

    QTranslator *m_qtLanguageTranslator;

    int m_languageNumber;

    bool m_addingMarker;
    bool m_isMouseClick;
    bool m_bInterrupted;
    QMap<QString, QStringList*> m_BandsMap;

    void setWidgetsSettings();
    bool loadBands();
    void setBands(QCustomPlot * widget, QStringList* bands, double y1, double y2);
    void setBands(QCustomPlot * widget, double y1, double y2);
    void addBand (QCustomPlot * widget, double x1, double x2, double y1, double y2);
    void createTabs (QString sequence);
    void moveEvent(QMoveEvent *);
    void resizeEvent(QResizeEvent *e);
    bool event(QEvent *event);

    void setFqFrom(QString from);
    void setFqFrom(double from);
    void setFqTo(QString to);
    void setFqTo(double to);
    double getFqFrom(void);
    double getFqTo(void);
    bool loadLanguage(QString locale); // locale: en, ukr, ru, jp, etc.
    void saveFile(int row, QString path);
    QCustomPlot* getCurrentPlot();

signals:
    void measure(qint64,qint64,int);
    void measureContinuous(qint64,qint64,int);
    void currentTab(QString);
    void focus(bool);
    void newCursorFq(double x, int number, int mouseX, int mouseY);
    void newCursorSmithPos(double x, double y, int number);
    void mainWindowPos(int, int);
    void aa30bootFound();
    void updateProgress(int);
    void stopMeasure();
    void isRangeChanged(bool);
    void rescale();

public slots:
    void on_pressF1 ();
    void on_pressF2 ();
    void on_pressF3 ();
    void on_pressF4 ();
    void on_pressF5 ();
    void on_pressF6 ();
    void on_pressF7 ();
    void on_pressEsc();
    void on_pressF9 ();
    void on_pressF10();
    void on_pressDelete();
    void on_pressPlus();
    void on_pressCtrlPlus();
    void on_pressMinus();
    void on_pressCtrlMinus();
    void on_pressCtrlZero();
    void on_pressLeft();
    void on_pressRight();
    void on_pressCtrlC();
    void on_analyzerFound(QString name);
    void on_analyzerDisconnected();
    void on_mouseWheel_swr(QWheelEvent *e);
    void on_mouseMove_swr(QMouseEvent *);
    void on_mouseWheel_phase(QWheelEvent *e);
    void on_mouseMove_phase(QMouseEvent *);
    void on_mouseWheel_rs(QWheelEvent * e);
    void on_mouseMove_rs(QMouseEvent *);
    void on_mouseWheel_rp(QWheelEvent *e);
    void on_mouseMove_rp(QMouseEvent *);
    void on_mouseWheel_rl(QWheelEvent *e);
    void on_mouseMove_rl(QMouseEvent *);
    void on_mouseWheel_tdr(QWheelEvent *e);
    void on_mouseMove_tdr(QMouseEvent *e);
    void on_mouseMove_smith(QMouseEvent *e);
    void on_singleStart_clicked();
    void on_continuousStartBtn_clicked(bool checked);
    void on_fqSettingsBtn_clicked();
    void on_presetsAddBtn_clicked();
    void on_tableWidget_presets_cellDoubleClicked(int row, int column);
    void on_presetsDeleteBtn_clicked();
    void on_pressetsUpBtn_clicked();
    void on_exportBtn_clicked();
    void on_measurementComplete();
    void on_translate(int number);
private slots:
    void on_analyzerDataBtn_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_screenshotAA_clicked();
    void on_settingsBtn_clicked();
    void on_dotsNumberChanged(int number);
    void on_measurmentsDeleteBtn_clicked();
    void on_tableWidget_measurments_cellClicked(int row, int column);
    void on_screenshot_clicked();
    void on_printBtn_clicked();
    void on_measurmentsSaveBtn_clicked();
    void on_measurementsOpenBtn_clicked();
    void on_measurementsClearBtn_clicked(bool);
    void on_importBtn_clicked();
    void on_changeMeasureSystemMetric (bool state);
    void on_Z0Changed(double _Z0);
    void updateGraph ();
    void on_settingsParamsChanged();
    void on_limitsBtn_clicked(bool checked);
    void on_rangeBtn_clicked(bool checked);
    void on_lineEdit_fqFrom_editingFinished();
    void on_lineEdit_fqTo_editingFinished();
    void resizeWnd(void);
    void on_newVersionAvailable();
    void on_downloadAfterClosing();
    void on_firmwareAutoUpdateStateChanged( bool state);
    void on_antScopeAutoUpdateStateChanged( bool state);
    void on_1secTimerTick();
    void on_changedAutoDetectMode(bool state);
    void on_changedSerialPort(QString portName);
    void on_calibrationChanged();
    void on_SaveFile(QString path);
    void on_mouseDoubleClick(QMouseEvent* e);
    void onCustomContextMenuRequested(const QPoint&);
    void onCreateMarker(const QPoint& pos);
    void onCreateMarker(QAction*);
    void on_bandChanged(QString);
    void onSpinChanged(int value);
    void calibrationToggled(bool checked);
    void on_dataChanged(qint64 _center_khz, qint64 _range_khz, qint32 _dots);
    void on_importFinished(double _fqMin, double _fqMax);
    void onFullRange(bool);
};



#endif // MAINWINDOW_H
