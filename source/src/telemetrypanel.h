#pragma once
#include <QWidget>
#include "physics.h"
#include "localization.h"

class QLabel;

class TelemetryPanel : public QWidget {
    Q_OBJECT
public:
    explicit TelemetryPanel(QWidget* parent = nullptr);
    void setLanguage(AppLanguage language);
    void refresh(const BeamState& s, bool vectorMode, int cmdCount);

private:
    void retranslateUi();

    QLabel* m_title = nullptr;
    QLabel* m_subtitle = nullptr;
    QLabel* m_modeKey = nullptr;
    QLabel* m_hVoltKey = nullptr;
    QLabel* m_vVoltKey = nullptr;
    QLabel* m_rCurKey = nullptr;
    QLabel* m_gCurKey = nullptr;
    QLabel* m_bCurKey = nullptr;
    QLabel* m_cmdsKey = nullptr;
    QLabel* m_mode = nullptr;
    QLabel* m_hVolt = nullptr;
    QLabel* m_vVolt = nullptr;
    QLabel* m_rCur = nullptr;
    QLabel* m_gCur = nullptr;
    QLabel* m_bCur = nullptr;
    QLabel* m_cmds = nullptr;
    AppLanguage m_language = AppLanguage::English;
    BeamState m_lastState;
    bool m_lastVectorMode = false;
    int m_lastCmdCount = 0;
};
