#pragma once

#include <QImage>
#include <QMatrix4x4>
#include <QPoint>
#include <QTimer>
#include <QVector>
#include <QVector3D>
#include <QWidget>

#include "physics.h"
#include "localization.h"

class QMouseEvent;
class QPainter;
class QPaintEvent;
class QPolygonF;
class QResizeEvent;
class QWheelEvent;

class CRTRenderer : public QWidget {
    Q_OBJECT
public:
    explicit CRTRenderer(QWidget* parent = nullptr);

    PhysicsEngine* physics() { return &m_phy; }
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void setHEnabled(bool value) { m_hEnabled = value; }
    void setVEnabled(bool value) { m_vEnabled = value; }
    void setSpeed(int value) { m_speed = value; }
    void setLanguage(AppLanguage language) { m_language = language; update(); }

signals:
    void stateUpdated(BeamState state, bool vectorMode, int cmdCount);

public slots:
    void resetPhosphor();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void tick();
    void updateProjection();
    void updateViewMatrices();
    void fadeScreen();
    void drawPhosphorHits(const QVector<DrawCall>& draws);
    void updateScreenImage();
    void updateSweepOverlay(const QVector<DrawCall>& draws);
    float shadowMaskWeight(int x, int channel) const;

    void drawBackground(QPainter& painter);
    void drawBackdropGrid(QPainter& painter);
    void drawScreen(QPainter& painter);
    void drawSweepOverlay(QPainter& painter) const;
    void drawTube(QPainter& painter);
    void drawCoils(QPainter& painter);
    void drawGuns(QPainter& painter);
    void drawBeam(QPainter& painter);
    void drawOverlayHints(QPainter& painter);

    float tubeRadiusAt(float z) const;
    QVector3D cameraPosition() const;
    QMatrix4x4 viewMatrix() const;
    QPointF projectPoint(const QVector3D& point, bool* ok = nullptr) const;
    float projectedPixelRadius(const QVector3D& center, float worldRadius) const;
    QPolygonF projectCircle(const QVector3D& center,
                            const QVector3D& axisA,
                            const QVector3D& axisB,
                            int segments = 40) const;

    PhysicsEngine m_phy;
    BeamState m_beam;
    QImage m_screenImg;
    QVector<float> m_phosphor;
    QVector<DrawCall> m_activeSweepDraws;
    bool m_hEnabled = true;
    bool m_vEnabled = true;
    int m_speed = 25;
    AppLanguage m_language = AppLanguage::English;

    float m_theta = -0.42f;
    float m_phi = 0.34f;
    float m_radius = 175.f;
    QVector3D m_target{0.f, 0.f, -25.f};
    QPoint m_lastMouse;
    bool m_leftBtn = false;

    QMatrix4x4 m_proj;
    QMatrix4x4 m_view;
    QMatrix4x4 m_viewProj;
    QTimer* m_timer = nullptr;

    static constexpr float SCREEN_W = 80.f;
    static constexpr float SCREEN_H = 60.f;
    static constexpr float CATHODE_Z = -100.f;
    static constexpr float COIL_Z = -45.f;
    static constexpr int TEX_W = 384;
    static constexpr int TEX_H = 288;
};
