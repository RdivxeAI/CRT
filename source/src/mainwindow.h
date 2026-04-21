#pragma once

#include <QMainWindow>
#include "physics.h"
#include "localization.h"

class QAction;
class QActionGroup;
class QCheckBox;
class QGroupBox;
class QLabel;
class QMenu;
class QPushButton;
class QSlider;
class QString;
class QToolBar;
class QWidget;

class CRTRenderer;
class TelemetryPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void loadImage();
    void loadCommands();
    void exportCommands();
    void restorePattern();
    void showAbout();
    void onRenderStateChanged(BeamState state, bool vectorMode, int cmdCount);
    void setLanguage(AppLanguage language);

private:
    enum class SourceContext {
        DefaultPattern,
        RasterFile,
        VectorFile
    };

    void buildUi();
    void buildMenus();
    void applyRetroStyle();
    void retranslateUi();
    void updateTelemetry();
    void updateModeLamp();
    void updateSourceLabel();
    void setSourceContext(SourceContext context, const QString& name = QString());

    CRTRenderer* m_renderer = nullptr;
    TelemetryPanel* m_telemetry = nullptr;
    QWidget* m_centralWidget = nullptr;
    QCheckBox* m_hCheck = nullptr;
    QCheckBox* m_vCheck = nullptr;
    QSlider* m_speedSlider = nullptr;
    QLabel* m_speedValue = nullptr;
    QLabel* m_modeLamp = nullptr;
    QLabel* m_sourceStamp = nullptr;
    QLabel* m_badgeTitle = nullptr;
    QLabel* m_modeTitle = nullptr;
    QLabel* m_viewCaption = nullptr;
    QLabel* m_speedCaption = nullptr;
    QPushButton* m_loadImageButton = nullptr;
    QPushButton* m_restorePatternButton = nullptr;
    QPushButton* m_exportVectorButton = nullptr;
    QPushButton* m_loadVectorButton = nullptr;
    QGroupBox* m_sourceGroup = nullptr;
    QGroupBox* m_deflectionGroup = nullptr;
    QGroupBox* m_timingGroup = nullptr;
    QGroupBox* m_ioGroup = nullptr;

    QAction* m_loadImageAction = nullptr;
    QAction* m_loadVectorAction = nullptr;
    QAction* m_exportVectorAction = nullptr;
    QAction* m_restorePatternAction = nullptr;
    QAction* m_aboutAction = nullptr;
    QAction* m_exitAction = nullptr;
    QAction* m_arabicAction = nullptr;
    QAction* m_englishAction = nullptr;
    QAction* m_frenchAction = nullptr;
    QAction* m_chineseAction = nullptr;
    QActionGroup* m_languageActions = nullptr;
    QMenu* m_fileMenu = nullptr;
    QMenu* m_helpMenu = nullptr;
    QMenu* m_languageMenu = nullptr;
    QToolBar* m_toolbar = nullptr;

    BeamState m_lastState;
    bool m_lastVectorMode = false;
    int m_lastCommandCount = 0;
    AppLanguage m_language = AppLanguage::English;
    SourceContext m_sourceContext = SourceContext::DefaultPattern;
    QString m_sourceName;
};
