#include "telemetrypanel.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {
QLabel* makeKey(QWidget* parent) {
    auto* label = new QLabel(parent);
    label->setObjectName(QStringLiteral("metricKey"));
    label->setAlignment(Qt::AlignLeading | Qt::AlignVCenter);
    return label;
}

QLabel* makeValue(QWidget* parent, const QString& accent = QString()) {
    auto* label = new QLabel(QStringLiteral("-"), parent);
    label->setObjectName(accent.isEmpty() ? QStringLiteral("metricValue") : accent);
    label->setAlignment(Qt::AlignTrailing | Qt::AlignVCenter);
    return label;
}
}

TelemetryPanel::TelemetryPanel(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(320);
    setObjectName(QStringLiteral("telemetryPanel"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto* panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("telemetryCard"));
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(18, 18, 18, 18);
    panelLayout->setSpacing(14);
    root->addWidget(panel);

    m_title = new QLabel(panel);
    m_title->setObjectName(QStringLiteral("telemetryTitle"));
    panelLayout->addWidget(m_title);

    m_subtitle = new QLabel(panel);
    m_subtitle->setObjectName(QStringLiteral("telemetrySubtitle"));
    m_subtitle->setWordWrap(true);
    panelLayout->addWidget(m_subtitle);

    auto* metricsFrame = new QFrame(panel);
    metricsFrame->setObjectName(QStringLiteral("metricsFrame"));
    auto* grid = new QGridLayout(metricsFrame);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);
    grid->setContentsMargins(14, 14, 14, 14);
    panelLayout->addWidget(metricsFrame);

    int row = 0;
    const auto addRow = [&](QLabel*& keyLabel, QLabel*& valueLabel, const QString& accent = QString()) {
        keyLabel = makeKey(metricsFrame);
        valueLabel = makeValue(metricsFrame, accent);
        grid->addWidget(keyLabel, row, 0);
        grid->addWidget(valueLabel, row, 1);
        ++row;
    };

    addRow(m_modeKey, m_mode, QStringLiteral("modeValue"));
    addRow(m_hVoltKey, m_hVolt);
    addRow(m_vVoltKey, m_vVolt);
    addRow(m_rCurKey, m_rCur, QStringLiteral("metricValueRed"));
    addRow(m_gCurKey, m_gCur, QStringLiteral("metricValueGreen"));
    addRow(m_bCurKey, m_bCur, QStringLiteral("metricValueBlue"));
    addRow(m_cmdsKey, m_cmds, QStringLiteral("metricValueCyan"));

    setStyleSheet(R"(
        QWidget#telemetryPanel {
            background: transparent;
        }
        QFrame#telemetryCard {
            background: #e8ebf0;
            border-top: 1px solid #ffffff;
            border-left: 1px solid #ffffff;
            border-right: 1px solid #98a3af;
            border-bottom: 1px solid #98a3af;
        }
        QLabel#telemetryTitle {
            color: #213b58;
            font: bold 16px "Verdana";
        }
        QLabel#telemetrySubtitle {
            color: #586979;
            font: 11px "Tahoma";
        }
        QFrame#metricsFrame {
            background: #f4f6f8;
            border: 1px solid #aab2bb;
        }
        QLabel#metricKey {
            color: #4f6274;
            font: 11px "Tahoma";
        }
        QLabel#metricValue {
            color: #1e3042;
            font: bold 12px "Courier New";
        }
        QLabel#modeValue {
            color: #234c77;
            background: #dfe8f1;
            border: 1px solid #95a7b8;
            padding: 4px 8px;
            font: bold 12px "Tahoma";
        }
        QLabel#metricValueRed {
            color: #9d3636;
            font: bold 12px "Courier New";
        }
        QLabel#metricValueGreen {
            color: #2f7a44;
            font: bold 12px "Courier New";
        }
        QLabel#metricValueBlue {
            color: #315d9f;
            font: bold 12px "Courier New";
        }
        QLabel#metricValueCyan {
            color: #1f6275;
            font: bold 12px "Courier New";
        }
    )");

    retranslateUi();
    refresh({}, false, 0);
}

void TelemetryPanel::setLanguage(AppLanguage language) {
    m_language = language;
    retranslateUi();
    refresh(m_lastState, m_lastVectorMode, m_lastCmdCount);
}

void TelemetryPanel::refresh(const BeamState& state, bool vectorMode, int cmdCount) {
    m_lastState = state;
    m_lastVectorMode = vectorMode;
    m_lastCmdCount = cmdCount;

    const UiText t = uiText(m_language);
    const QLocale locale = localeForLanguage(m_language);

    m_mode->setText(vectorMode ? t.vectorModeTelemetry : t.rasterModeTelemetry);
    m_hVolt->setText(locale.toString(static_cast<double>(state.hVolt * 12.5f), 'f', 2) + QStringLiteral(" V"));
    m_vVolt->setText(locale.toString(static_cast<double>(state.vVolt * 12.5f), 'f', 2) + QStringLiteral(" V"));
    m_rCur->setText(locale.toString(static_cast<double>(state.rAmp * 1.5f), 'f', 3) + QStringLiteral(" mA"));
    m_gCur->setText(locale.toString(static_cast<double>(state.gAmp * 1.5f), 'f', 3) + QStringLiteral(" mA"));
    m_bCur->setText(locale.toString(static_cast<double>(state.bAmp * 1.5f), 'f', 3) + QStringLiteral(" mA"));
    m_cmds->setText(locale.toString(cmdCount));
}

void TelemetryPanel::retranslateUi() {
    const UiText t = uiText(m_language);

    m_title->setText(t.telemetryTitle);
    m_subtitle->setText(t.telemetrySubtitle);
    m_modeKey->setText(t.telemetryModeKey);
    m_hVoltKey->setText(t.telemetryHKey);
    m_vVoltKey->setText(t.telemetryVKey);
    m_rCurKey->setText(t.telemetryRKey);
    m_gCurKey->setText(t.telemetryGKey);
    m_bCurKey->setText(t.telemetryBKey);
    m_cmdsKey->setText(t.telemetryCmdsKey);
}
