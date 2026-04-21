#include "mainwindow.h"

#include "crtrenderer.h"
#include "telemetrypanel.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

namespace {
QString appBrandTitle() {
    return QStringLiteral("CRT PHYSICS EMULATOR");
}
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_language(defaultLanguageFromSystem())
{
    setWindowIcon(QIcon(QStringLiteral(":/branding/logo.png")));

    buildUi();
    buildMenus();
    applyRetroStyle();
    setSourceContext(SourceContext::DefaultPattern);
    setLanguage(m_language);

    resize(1480, 900);
    statusBar()->showMessage(uiText(m_language).readyStatus, 4000);
}

void MainWindow::loadImage() {
    const UiText t = uiText(m_language);
    const QString file = QFileDialog::getOpenFileName(
        this,
        t.selectImageTitle,
        QString(),
        t.imageFileFilter
    );
    if (file.isEmpty()) {
        return;
    }

    if (!m_renderer->physics()->loadImage(file)) {
        QMessageBox::warning(this, t.imageLoadFailedTitle, t.imageLoadFailedBody);
        return;
    }

    m_renderer->resetPhosphor();
    m_lastState = {};
    m_lastVectorMode = false;
    m_lastCommandCount = 0;
    setSourceContext(SourceContext::RasterFile, QFileInfo(file).fileName());
    updateTelemetry();
    statusBar()->showMessage(t.imageLoadedStatus, 5000);
}

void MainWindow::loadCommands() {
    const UiText t = uiText(m_language);
    const QString file = QFileDialog::getOpenFileName(
        this,
        t.selectVectorTitle,
        QString(),
        t.jsonFileFilter
    );
    if (file.isEmpty()) {
        return;
    }

    if (!m_renderer->physics()->loadCommands(file)) {
        QMessageBox::warning(this, t.vectorLoadFailedTitle, t.vectorLoadFailedBody);
        return;
    }

    m_renderer->resetPhosphor();
    m_lastState = {};
    m_lastVectorMode = m_renderer->physics()->isVectorMode();
    m_lastCommandCount = m_renderer->physics()->commandCount();
    setSourceContext(SourceContext::VectorFile, QFileInfo(file).fileName());
    updateTelemetry();

    QMessageBox::information(
        this,
        t.vectorLoadedTitle,
        t.vectorLoadedBody.arg(localeForLanguage(m_language).toString(m_lastCommandCount))
    );
}

void MainWindow::exportCommands() {
    const UiText t = uiText(m_language);
    QString file = QFileDialog::getSaveFileName(
        this,
        t.saveVectorTitle,
        t.saveVectorDefaultName,
        t.jsonFileFilter
    );
    if (file.isEmpty()) {
        return;
    }
    if (!file.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
        file += QStringLiteral(".json");
    }

    ExportStats stats;
    if (!m_renderer->physics()->exportCommands(file, &stats)) {
        QMessageBox::warning(this, t.saveVectorFailedTitle, t.saveVectorFailedBody);
        return;
    }

    m_lastCommandCount = stats.emitted;
    updateTelemetry();

    const QLocale locale = localeForLanguage(m_language);
    QMessageBox::information(
        this,
        t.exportVectorTitle,
        t.exportVectorBody
            .arg(locale.toString(stats.emitted))
            .arg(locale.toString(stats.skipped))
            .arg(locale.toString(stats.compressionRatio(), 'f', 1))
    );

    statusBar()->showMessage(t.exportVectorStatus, 5000);
}

void MainWindow::restorePattern() {
    m_renderer->physics()->generateDefaultPattern();
    m_renderer->resetPhosphor();
    m_lastState = {};
    m_lastVectorMode = false;
    m_lastCommandCount = 0;
    setSourceContext(SourceContext::DefaultPattern);
    updateTelemetry();
    statusBar()->showMessage(uiText(m_language).restorePatternStatus, 5000);
}

void MainWindow::showAbout() {
    const UiText t = uiText(m_language);

    QDialog dialog(this);
    dialog.setModal(true);
    dialog.setWindowTitle(t.aboutWindowTitle);
    dialog.setWindowIcon(QIcon(QStringLiteral(":/branding/logo.png")));
    dialog.setFixedSize(520, 340);
    dialog.setStyleSheet(R"(
        QDialog {
            background: #d9dde3;
            color: #1f2833;
        }
        QFrame#aboutPanel {
            background: #eef1f5;
            border: 1px solid #8a95a1;
        }
        QFrame#logoPanel {
            background: #ffffff;
            border: 1px solid #a2adb9;
        }
        QLabel#aboutTitle {
            color: #233750;
            font: bold 16px "Verdana";
        }
        QLabel#aboutBody {
            color: #334456;
            font: 12px "Tahoma";
        }
        QLabel#aboutRights {
            color: #5b6b7c;
            font: 11px "Tahoma";
        }
        QPushButton {
            background: #e7ebf0;
            color: #1f2a36;
            border-top: 1px solid #ffffff;
            border-left: 1px solid #ffffff;
            border-right: 1px solid #7f8a96;
            border-bottom: 1px solid #7f8a96;
            padding: 6px 18px;
            min-height: 22px;
            font: 12px "Tahoma";
        }
        QPushButton:hover {
            background: #f2f4f7;
        }
        QPushButton:pressed {
            background: #d4dae2;
        }
    )");

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(16, 16, 16, 16);

    auto* panel = new QFrame(&dialog);
    panel->setObjectName(QStringLiteral("aboutPanel"));
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(20, 20, 20, 20);
    panelLayout->setSpacing(14);
    root->addWidget(panel);

    auto* logoPanel = new QFrame(panel);
    logoPanel->setObjectName(QStringLiteral("logoPanel"));
    auto* logoLayout = new QVBoxLayout(logoPanel);
    logoLayout->setContentsMargins(12, 12, 12, 12);

    auto* logoLabel = new QLabel(logoPanel);
    logoLabel->setAlignment(Qt::AlignCenter);
    QPixmap logo(QStringLiteral(":/branding/logo.png"));
    logoLabel->setPixmap(logo.scaledToWidth(172, Qt::SmoothTransformation));
    logoLayout->addWidget(logoLabel);
    panelLayout->addWidget(logoPanel);

    auto* title = new QLabel(appBrandTitle(), panel);
    title->setObjectName(QStringLiteral("aboutTitle"));
    title->setAlignment(Qt::AlignCenter);
    title->setWordWrap(true);
    panelLayout->addWidget(title);

    auto* body = new QLabel(t.aboutBody, panel);
    body->setObjectName(QStringLiteral("aboutBody"));
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);
    panelLayout->addWidget(body);

    auto* rights = new QLabel(t.aboutRights, panel);
    rights->setObjectName(QStringLiteral("aboutRights"));
    rights->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(rights);

    auto* closeButton = new QPushButton(t.closeButton, panel);
    closeButton->setFixedWidth(132);
    panelLayout->addWidget(closeButton, 0, Qt::AlignHCenter);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

void MainWindow::onRenderStateChanged(BeamState state, bool vectorMode, int cmdCount) {
    m_lastState = state;
    m_lastVectorMode = vectorMode;
    if (vectorMode) {
        m_lastCommandCount = cmdCount;
    }
    updateTelemetry();
}

void MainWindow::setLanguage(AppLanguage language) {
    m_language = language;

    QLocale::setDefault(localeForLanguage(language));
    qApp->setLayoutDirection(layoutDirectionForLanguage(language));
    setLayoutDirection(layoutDirectionForLanguage(language));
    if (m_centralWidget) {
        m_centralWidget->setLayoutDirection(layoutDirectionForLanguage(language));
    }

    if (m_renderer) {
        m_renderer->setLanguage(language);
    }
    if (m_telemetry) {
        m_telemetry->setLanguage(language);
    }

    if (m_arabicAction) {
        m_arabicAction->setChecked(language == AppLanguage::Arabic);
    }
    if (m_englishAction) {
        m_englishAction->setChecked(language == AppLanguage::English);
    }
    if (m_frenchAction) {
        m_frenchAction->setChecked(language == AppLanguage::French);
    }
    if (m_chineseAction) {
        m_chineseAction->setChecked(language == AppLanguage::Chinese);
    }

    retranslateUi();
    updateSourceLabel();
    updateTelemetry();
}

void MainWindow::buildUi() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    auto* root = new QVBoxLayout(m_centralWidget);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    auto* headerRow = new QHBoxLayout;
    headerRow->setSpacing(10);
    headerRow->setDirection(QBoxLayout::LeftToRight);
    root->addLayout(headerRow);

    auto* badgeFrame = new QFrame(m_centralWidget);
    badgeFrame->setObjectName(QStringLiteral("panel"));
    auto* badgeLayout = new QVBoxLayout(badgeFrame);
    badgeLayout->setContentsMargins(16, 14, 16, 14);
    badgeLayout->setSpacing(2);

    m_badgeTitle = new QLabel(badgeFrame);
    m_badgeTitle->setObjectName(QStringLiteral("badgeTitle"));
    m_badgeTitle->setWordWrap(true);
    badgeLayout->addWidget(m_badgeTitle);
    headerRow->addWidget(badgeFrame, 1);

    auto* modeFrame = new QFrame(m_centralWidget);
    modeFrame->setObjectName(QStringLiteral("panel"));
    auto* modeLayout = new QVBoxLayout(modeFrame);
    modeLayout->setContentsMargins(16, 14, 16, 14);
    modeLayout->setSpacing(6);

    m_modeTitle = new QLabel(modeFrame);
    m_modeTitle->setObjectName(QStringLiteral("microTitle"));
    m_modeLamp = new QLabel(modeFrame);
    m_modeLamp->setObjectName(QStringLiteral("modeLamp"));
    m_sourceStamp = new QLabel(modeFrame);
    m_sourceStamp->setObjectName(QStringLiteral("sourceStamp"));
    m_sourceStamp->setWordWrap(true);

    modeLayout->addWidget(m_modeTitle);
    modeLayout->addWidget(m_modeLamp);
    modeLayout->addWidget(m_sourceStamp);
    headerRow->addWidget(modeFrame);

    auto* middleRow = new QHBoxLayout;
    middleRow->setSpacing(10);
    middleRow->setDirection(QBoxLayout::LeftToRight);
    root->addLayout(middleRow, 1);

    auto* viewportFrame = new QFrame(m_centralWidget);
    viewportFrame->setObjectName(QStringLiteral("panel"));
    auto* viewportLayout = new QVBoxLayout(viewportFrame);
    viewportLayout->setContentsMargins(10, 10, 10, 10);
    viewportLayout->setSpacing(8);

    m_viewCaption = new QLabel(viewportFrame);
    m_viewCaption->setObjectName(QStringLiteral("sectionTitle"));
    viewportLayout->addWidget(m_viewCaption);

    m_renderer = new CRTRenderer(viewportFrame);
    m_renderer->setLayoutDirection(Qt::LeftToRight);
    viewportLayout->addWidget(m_renderer, 1);
    middleRow->addWidget(viewportFrame, 1);

    m_telemetry = new TelemetryPanel(m_centralWidget);
    middleRow->addWidget(m_telemetry);

    auto* controlFrame = new QFrame(m_centralWidget);
    controlFrame->setObjectName(QStringLiteral("panel"));
    auto* controlLayout = new QHBoxLayout(controlFrame);
    controlLayout->setContentsMargins(12, 10, 12, 10);
    controlLayout->setSpacing(10);
    controlLayout->setDirection(QBoxLayout::LeftToRight);
    root->addWidget(controlFrame);

    m_sourceGroup = new QGroupBox(controlFrame);
    auto* sourceLayout = new QVBoxLayout(m_sourceGroup);
    m_loadImageButton = new QPushButton(m_sourceGroup);
    m_restorePatternButton = new QPushButton(m_sourceGroup);
    sourceLayout->addWidget(m_loadImageButton);
    sourceLayout->addWidget(m_restorePatternButton);
    controlLayout->addWidget(m_sourceGroup);

    m_deflectionGroup = new QGroupBox(controlFrame);
    auto* deflectionLayout = new QVBoxLayout(m_deflectionGroup);
    m_hCheck = new QCheckBox(m_deflectionGroup);
    m_hCheck->setChecked(true);
    m_vCheck = new QCheckBox(m_deflectionGroup);
    m_vCheck->setChecked(true);
    deflectionLayout->addWidget(m_hCheck);
    deflectionLayout->addWidget(m_vCheck);
    controlLayout->addWidget(m_deflectionGroup);

    m_timingGroup = new QGroupBox(controlFrame);
    auto* timingLayout = new QVBoxLayout(m_timingGroup);
    auto* timingRow = new QHBoxLayout;
    timingRow->setDirection(QBoxLayout::LeftToRight);
    m_speedCaption = new QLabel(m_timingGroup);
    m_speedValue = new QLabel(QStringLiteral("25"), m_timingGroup);
    m_speedValue->setObjectName(QStringLiteral("speedValue"));
    timingRow->addWidget(m_speedCaption);
    timingRow->addStretch(1);
    timingRow->addWidget(m_speedValue);
    timingLayout->addLayout(timingRow);
    m_speedSlider = new QSlider(Qt::Horizontal, m_timingGroup);
    m_speedSlider->setRange(1, 50);
    m_speedSlider->setValue(25);
    timingLayout->addWidget(m_speedSlider);
    controlLayout->addWidget(m_timingGroup, 1);

    m_ioGroup = new QGroupBox(controlFrame);
    auto* ioLayout = new QVBoxLayout(m_ioGroup);
    m_exportVectorButton = new QPushButton(m_ioGroup);
    m_loadVectorButton = new QPushButton(m_ioGroup);
    ioLayout->addWidget(m_exportVectorButton);
    ioLayout->addWidget(m_loadVectorButton);
    controlLayout->addWidget(m_ioGroup);

    connect(m_loadImageButton, &QPushButton::clicked, this, &MainWindow::loadImage);
    connect(m_restorePatternButton, &QPushButton::clicked, this, &MainWindow::restorePattern);
    connect(m_exportVectorButton, &QPushButton::clicked, this, &MainWindow::exportCommands);
    connect(m_loadVectorButton, &QPushButton::clicked, this, &MainWindow::loadCommands);

    connect(m_hCheck, &QCheckBox::toggled, m_renderer, &CRTRenderer::setHEnabled);
    connect(m_vCheck, &QCheckBox::toggled, m_renderer, &CRTRenderer::setVEnabled);
    connect(m_speedSlider, &QSlider::valueChanged, this, [this](int value) {
        m_renderer->setSpeed(value);
        m_speedValue->setText(localeForLanguage(m_language).toString(value));
        if (!m_lastVectorMode) {
            statusBar()->showMessage(uiText(m_language).pulseRateStatusFormat.arg(localeForLanguage(m_language).toString(value)), 1500);
        }
    });
    connect(m_renderer, &CRTRenderer::stateUpdated, this, &MainWindow::onRenderStateChanged);
}

void MainWindow::buildMenus() {
    m_loadImageAction = new QAction(this);
    m_loadVectorAction = new QAction(this);
    m_exportVectorAction = new QAction(this);
    m_restorePatternAction = new QAction(this);
    m_aboutAction = new QAction(this);
    m_exitAction = new QAction(this);

    connect(m_loadImageAction, &QAction::triggered, this, &MainWindow::loadImage);
    connect(m_loadVectorAction, &QAction::triggered, this, &MainWindow::loadCommands);
    connect(m_exportVectorAction, &QAction::triggered, this, &MainWindow::exportCommands);
    connect(m_restorePatternAction, &QAction::triggered, this, &MainWindow::restorePattern);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

    m_fileMenu = menuBar()->addMenu(QString());
    m_languageMenu = menuBar()->addMenu(QString());
    m_helpMenu = menuBar()->addMenu(QString());

    m_fileMenu->addAction(m_loadImageAction);
    m_fileMenu->addAction(m_loadVectorAction);
    m_fileMenu->addAction(m_exportVectorAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_restorePatternAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);

    m_languageActions = new QActionGroup(this);
    m_languageActions->setExclusive(true);

    const auto addLanguageAction = [this](AppLanguage language, QAction*& action) {
        action = new QAction(this);
        action->setCheckable(true);
        m_languageActions->addAction(action);
        m_languageMenu->addAction(action);
        connect(action, &QAction::triggered, this, [this, language]() { setLanguage(language); });
    };

    addLanguageAction(AppLanguage::Arabic, m_arabicAction);
    addLanguageAction(AppLanguage::English, m_englishAction);
    addLanguageAction(AppLanguage::French, m_frenchAction);
    addLanguageAction(AppLanguage::Chinese, m_chineseAction);

    m_helpMenu->addAction(m_aboutAction);

    m_toolbar = addToolBar(QString());
    m_toolbar->setMovable(false);
    m_toolbar->addAction(m_loadImageAction);
    m_toolbar->addAction(m_loadVectorAction);
    m_toolbar->addAction(m_exportVectorAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_restorePatternAction);
}

void MainWindow::applyRetroStyle() {
    setStyleSheet(R"(
        QMainWindow {
            background: #cfd4db;
        }
        QMenuBar {
            background: #d7dce3;
            color: #1f2b36;
            border-top: 1px solid #ffffff;
            border-left: 1px solid #ffffff;
            border-right: 1px solid #939eaa;
            border-bottom: 1px solid #939eaa;
        }
        QMenuBar::item {
            padding: 5px 10px;
            background: transparent;
            font: 12px "Tahoma";
        }
        QMenuBar::item:selected {
            background: #bcc6d2;
        }
        QMenu {
            background: #edf0f4;
            color: #1f2b36;
            border: 1px solid #8d97a3;
        }
        QMenu::item {
            padding: 6px 18px;
            font: 12px "Tahoma";
        }
        QMenu::item:selected {
            background: #c4ced9;
        }
        QToolBar {
            background: #d8dde4;
            border-top: 1px solid #ffffff;
            border-left: 1px solid #ffffff;
            border-right: 1px solid #99a3af;
            border-bottom: 1px solid #99a3af;
            spacing: 4px;
            padding: 4px;
        }
        QToolButton, QPushButton {
            background: #e6eaef;
            color: #22303e;
            border-top: 1px solid #ffffff;
            border-left: 1px solid #ffffff;
            border-right: 1px solid #848e98;
            border-bottom: 1px solid #848e98;
            padding: 6px 12px;
            min-height: 22px;
            font: 12px "Tahoma";
        }
        QToolButton:hover, QPushButton:hover {
            background: #f1f4f7;
        }
        QToolButton:pressed, QPushButton:pressed {
            background: #d0d7df;
        }
        QFrame#panel {
            background: #e8ebf0;
            border-top: 1px solid #ffffff;
            border-left: 1px solid #ffffff;
            border-right: 1px solid #98a3af;
            border-bottom: 1px solid #98a3af;
        }
        QGroupBox {
            color: #213246;
            border: 1px solid #a4adb7;
            margin-top: 12px;
            padding-top: 14px;
            background: #f2f4f7;
            font: bold 12px "Tahoma";
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 10px;
            padding: 0 4px;
            color: #304966;
        }
        QLabel {
            color: #243342;
            font: 12px "Tahoma";
        }
        QLabel#badgeTitle {
            font: bold 18px "Verdana";
            color: #223b58;
        }
        QLabel#microTitle {
            color: #49647f;
            font: bold 11px "Tahoma";
        }
        QLabel#modeLamp {
            font: bold 12px "Tahoma";
            padding: 6px 10px;
            border: 1px solid #9da7b2;
            background: #eef2f6;
        }
        QLabel#sourceStamp {
            color: #5a6b7b;
            font: 11px "Tahoma";
        }
        QLabel#sectionTitle {
            font: bold 12px "Tahoma";
            color: #2e465f;
            padding: 2px 2px 4px 2px;
        }
        QLabel#speedValue {
            color: #1f3955;
            font: bold 12px "Tahoma";
            min-width: 36px;
        }
        QCheckBox {
            color: #243342;
            spacing: 8px;
            font: 12px "Tahoma";
        }
        QCheckBox::indicator {
            width: 15px;
            height: 15px;
            border: 1px solid #7f8a96;
            background: #ffffff;
        }
        QCheckBox::indicator:checked {
            background: #6787ab;
            border: 1px solid #4d6885;
        }
        QSlider::groove:horizontal {
            height: 6px;
            background: #c2c9d2;
            border: 1px solid #8a94a0;
            margin: 2px 0;
        }
        QSlider::handle:horizontal {
            width: 16px;
            margin: -6px 0;
            background: #edf1f4;
            border-top: 1px solid #ffffff;
            border-left: 1px solid #ffffff;
            border-right: 1px solid #7c8792;
            border-bottom: 1px solid #7c8792;
        }
        QStatusBar {
            background: #d8dde4;
            color: #243342;
            border-top: 1px solid #98a3af;
            font: 12px "Tahoma";
        }
    )");
}

void MainWindow::retranslateUi() {
    const UiText t = uiText(m_language);

    setWindowTitle(appBrandTitle());
    m_badgeTitle->setText(appBrandTitle());
    m_modeTitle->setText(t.engineStatusTitle);
    m_viewCaption->setText(t.viewportCaption);
    m_sourceGroup->setTitle(t.sourceGroupTitle);
    m_loadImageButton->setText(t.loadImageButton);
    m_restorePatternButton->setText(t.restorePatternButton);
    m_deflectionGroup->setTitle(t.deflectionGroupTitle);
    m_hCheck->setText(t.hDeflectionToggle);
    m_vCheck->setText(t.vDeflectionToggle);
    m_timingGroup->setTitle(t.timingGroupTitle);
    m_speedCaption->setText(t.speedCaption);
    m_speedValue->setText(localeForLanguage(m_language).toString(m_speedSlider->value()));
    m_ioGroup->setTitle(t.ioGroupTitle);
    m_exportVectorButton->setText(t.exportVectorButton);
    m_loadVectorButton->setText(t.loadVectorButton);

    m_loadImageAction->setText(t.loadImageAction);
    m_loadVectorAction->setText(t.loadVectorAction);
    m_exportVectorAction->setText(t.exportVectorAction);
    m_restorePatternAction->setText(t.restorePatternAction);
    m_aboutAction->setText(t.aboutAction);
    m_exitAction->setText(t.exitAction);

    m_fileMenu->setTitle(t.fileMenuTitle);
    m_languageMenu->setTitle(t.languageMenuTitle);
    m_helpMenu->setTitle(t.helpMenuTitle);
    m_toolbar->setWindowTitle(t.quickAccessTitle);

    m_arabicAction->setText(nativeLanguageName(AppLanguage::Arabic));
    m_englishAction->setText(nativeLanguageName(AppLanguage::English));
    m_frenchAction->setText(nativeLanguageName(AppLanguage::French));
    m_chineseAction->setText(nativeLanguageName(AppLanguage::Chinese));

    updateModeLamp();
}

void MainWindow::updateTelemetry() {
    m_telemetry->refresh(m_lastState, m_lastVectorMode, m_lastCommandCount);
    updateModeLamp();
}

void MainWindow::updateModeLamp() {
    const UiText t = uiText(m_language);

    if (m_lastVectorMode) {
        m_modeLamp->setText(t.vectorModeChip);
        m_modeLamp->setStyleSheet(QStringLiteral(
            "QLabel#modeLamp { font: bold 12px 'Tahoma'; padding: 6px 10px; "
            "background: #dde8f3; color: #234d7a; border: 1px solid #8b9fb4; }"));
    } else {
        m_modeLamp->setText(t.rasterModeChip);
        m_modeLamp->setStyleSheet(QStringLiteral(
            "QLabel#modeLamp { font: bold 12px 'Tahoma'; padding: 6px 10px; "
            "background: #ece7d8; color: #66551f; border: 1px solid #a39a82; }"));
    }
}

void MainWindow::updateSourceLabel() {
    const UiText t = uiText(m_language);

    if (m_sourceContext == SourceContext::VectorFile) {
        m_sourceStamp->setText(t.sourceVectorFormat.arg(m_sourceName));
        return;
    }

    const QString rasterName = m_sourceContext == SourceContext::DefaultPattern ? t.defaultPatternName : m_sourceName;
    m_sourceStamp->setText(t.sourceRasterFormat.arg(rasterName));
}

void MainWindow::setSourceContext(SourceContext context, const QString& name) {
    m_sourceContext = context;
    m_sourceName = name;
    updateSourceLabel();
}
