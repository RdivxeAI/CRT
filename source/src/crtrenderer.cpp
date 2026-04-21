#include "crtrenderer.h"

#include <QFont>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygon>
#include <QResizeEvent>
#include <QTransform>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.1415926535f;

template <typename T>
T clampValue(T value, T minimum, T maximum) {
    return std::max(minimum, std::min(value, maximum));
}

float lerpValue(float a, float b, float t) {
    return a + (b - a) * t;
}

QColor beamColor(const BeamState& state, int alpha = 255) {
    return QColor::fromRgbF(clampValue(state.rAmp, 0.0f, 1.0f),
                            clampValue(state.gAmp, 0.0f, 1.0f),
                            clampValue(state.bAmp, 0.0f, 1.0f),
                            clampValue(alpha / 255.0, 0.0, 1.0));
}
}

CRTRenderer::CRTRenderer(QWidget* parent)
    : QWidget(parent),
      m_screenImg(TEX_W, TEX_H, QImage::Format_RGB32),
      m_phosphor(TEX_W * TEX_H * 3, 0.0f)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_screenImg.fill(Qt::black);
    updateProjection();

    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &CRTRenderer::tick);
    m_timer->start(16);
}

QSize CRTRenderer::minimumSizeHint() const {
    return {760, 480};
}

QSize CRTRenderer::sizeHint() const {
    return {1040, 640};
}

void CRTRenderer::resetPhosphor() {
    std::fill(m_phosphor.begin(), m_phosphor.end(), 0.0f);
    m_screenImg.fill(Qt::black);
    m_activeSweepDraws.clear();
    update();
}

void CRTRenderer::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    drawBackground(painter);
    drawScreen(painter);
    drawGuns(painter);
    drawCoils(painter);
    drawBeam(painter);
    drawTube(painter);
    drawOverlayHints(painter);
}

void CRTRenderer::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateProjection();
}

void CRTRenderer::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_leftBtn = true;
        m_lastMouse = event->pos();
    }
    QWidget::mousePressEvent(event);
}

void CRTRenderer::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_leftBtn = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void CRTRenderer::mouseMoveEvent(QMouseEvent* event) {
    if (m_leftBtn) {
        const QPoint delta = event->pos() - m_lastMouse;
        m_theta -= delta.x() * 0.008f;
        m_phi = clampValue(m_phi + delta.y() * 0.0065f, -1.1f, 1.1f);
        m_lastMouse = event->pos();
        updateViewMatrices();
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void CRTRenderer::wheelEvent(QWheelEvent* event) {
    const float steps = static_cast<float>(event->angleDelta().y()) / 120.0f;
    m_radius = clampValue(m_radius - steps * 9.0f, 105.0f, 320.0f);
    updateViewMatrices();
    update();
    event->accept();
}

void CRTRenderer::tick() {
    fadeScreen();

    QVector<DrawCall> draws;
    const int baseSpeed = clampValue(m_speed, 1, 50);
    const bool vectorPlayback = m_phy.usesVectorPlayback();
    const int iterations = vectorPlayback
                               ? clampValue(std::max(baseSpeed * 160, std::max(512, m_phy.commandCount() / 4)), 768, 12000)
                               : clampValue((baseSpeed * baseSpeed * 2) + 80, 96, 6000);

    m_beam = m_phy.step(iterations, m_hEnabled, m_vEnabled, draws);
    drawPhosphorHits(draws);
    updateSweepOverlay(draws);
    updateScreenImage();

    emit stateUpdated(m_beam, m_phy.isVectorMode(), m_phy.commandCount());
    update();
}

void CRTRenderer::updateProjection() {
    m_proj.setToIdentity();
    const float aspect = height() > 0 ? static_cast<float>(width()) / static_cast<float>(height()) : 1.0f;
    m_proj.perspective(45.0f, aspect, 1.0f, 1000.0f);
    updateViewMatrices();
}

void CRTRenderer::updateViewMatrices() {
    m_view.setToIdentity();
    m_view.lookAt(cameraPosition(), m_target, {0.0f, 1.0f, 0.0f});
    m_viewProj = m_proj * m_view;
}

void CRTRenderer::fadeScreen() {
    const bool vectorPlayback = m_phy.usesVectorPlayback();
    const float rDecay = vectorPlayback ? 0.985f : 0.972f;
    const float gDecay = vectorPlayback ? 0.989f : 0.978f;
    const float bDecay = vectorPlayback ? 0.982f : 0.968f;

    for (int i = 0; i < m_phosphor.size(); i += 3) {
        m_phosphor[i] *= rDecay;
        m_phosphor[i + 1] *= gDecay;
        m_phosphor[i + 2] *= bDecay;

        if (m_phosphor[i] < 0.00015f) {
            m_phosphor[i] = 0.0f;
        }
        if (m_phosphor[i + 1] < 0.00015f) {
            m_phosphor[i + 1] = 0.0f;
        }
        if (m_phosphor[i + 2] < 0.00015f) {
            m_phosphor[i + 2] = 0.0f;
        }
    }
}

void CRTRenderer::drawPhosphorHits(const QVector<DrawCall>& draws) {
    if (draws.isEmpty()) {
        return;
    }

    const bool vectorPlayback = m_phy.usesVectorPlayback();

    if (vectorPlayback) {
        constexpr float kCoreDen = 2.0f * 0.72f * 0.72f;
        constexpr float kHaloDen = 2.0f * 1.55f * 1.55f;

        const auto depositVectorSample = [&](float x, float y, float r, float g, float b, float energy) {
            const int cx = static_cast<int>(std::floor(x + 0.5f));
            const int cy = static_cast<int>(std::floor(y + 0.5f));
            const float whiteBoost = energy * 0.022f;

            for (int dy = -4; dy <= 4; ++dy) {
                const int py = cy + dy;
                if (py < 0 || py >= TEX_H) continue;
                const float fy = static_cast<float>(py) - y;
                const float fy2 = fy * fy;

                for (int dx = -4; dx <= 4; ++dx) {
                    const int px = cx + dx;
                    if (px < 0 || px >= TEX_W) continue;
                    const float fx = static_cast<float>(px) - x;
                    const float d2 = fx * fx + fy2;
                    const float core = std::exp(-d2 / kCoreDen);
                    const float halo = std::exp(-d2 / kHaloDen);
                    const float w = (core * 0.78f + halo * 0.18f) * energy;
                    if (w < 0.0005f) continue;

                    const int idx = (py * TEX_W + px) * 3;
                    m_phosphor[idx]     += r * shadowMaskWeight(px, 0) * w + core * whiteBoost;
                    m_phosphor[idx + 1] += g * shadowMaskWeight(px, 1) * w + core * whiteBoost;
                    m_phosphor[idx + 2] += b * shadowMaskWeight(px, 2) * w + core * whiteBoost;
                }
            }
        };

        bool hasPrevActive = false;
        DrawCall prevActive;

        for (const DrawCall& dc : draws) {
            const float intensity = clampValue((dc.r + dc.g + dc.b) / 3.0f, 0.0f, 1.0f);
            if (intensity < 0.01f) {
                hasPrevActive = false;
                continue;
            }

            const float x = clampValue(dc.normX * static_cast<float>(TEX_W - 1), 0.0f, static_cast<float>(TEX_W - 1));
            const float y = clampValue(dc.normY * static_cast<float>(TEX_H - 1), 0.0f, static_cast<float>(TEX_H - 1));
            const float energy = 0.038f + intensity * 0.022f;

            if (!hasPrevActive) {
                depositVectorSample(x, y, dc.r, dc.g, dc.b, energy);
                prevActive = dc;
                hasPrevActive = true;
                continue;
            }

            const float prevX = clampValue(prevActive.normX * static_cast<float>(TEX_W - 1), 0.0f, static_cast<float>(TEX_W - 1));
            const float prevY = clampValue(prevActive.normY * static_cast<float>(TEX_H - 1), 0.0f, static_cast<float>(TEX_H - 1));
            const int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::abs(x - prevX), std::abs(y - prevY)))));

            for (int step = 1; step <= steps; ++step) {
                const float t = static_cast<float>(step) / static_cast<float>(steps);
                depositVectorSample(lerpValue(prevX, x, t),
                                    lerpValue(prevY, y, t),
                                    lerpValue(prevActive.r, dc.r, t),
                                    lerpValue(prevActive.g, dc.g, t),
                                    lerpValue(prevActive.b, dc.b, t),
                                    energy * 0.84f);
            }

            prevActive = dc;
        }

        return;
    }

    const auto depositSample = [&](const DrawCall& dc) {
        if ((dc.r + dc.g + dc.b) < 0.02f) {
            return;
        }

        const float x = clampValue(dc.normX * static_cast<float>(TEX_W - 1), 0.0f, static_cast<float>(TEX_W - 1));
        const float y = clampValue(dc.normY * static_cast<float>(TEX_H - 1), 0.0f, static_cast<float>(TEX_H - 1));
        const float intensity = clampValue((dc.r + dc.g + dc.b) / 3.0f, 0.0f, 1.0f);

        const float coreSigmaX = 0.82f + intensity * 0.56f;
        const float coreSigmaY = 0.64f + intensity * 0.38f;
        const float coreX = coreSigmaX * 1.55f;
        const float coreY = coreSigmaY;
        const float haloX = coreX * 1.92f;
        const float haloY = coreY * 1.8f;
        const float coreXDen = std::max(0.01f, 2.0f * coreX * coreX);
        const float coreYDen = std::max(0.01f, 2.0f * coreY * coreY);
        const float haloXDen = std::max(0.01f, 2.0f * haloX * haloX);
        const float haloYDen = std::max(0.01f, 2.0f * haloY * haloY);

        const int radiusX = static_cast<int>(std::ceil(haloX * 2.25f));
        const int radiusY = static_cast<int>(std::ceil(haloY * 2.25f));
        const int minX = std::max(0, static_cast<int>(std::floor(x)) - radiusX);
        const int maxX = std::min(TEX_W - 1, static_cast<int>(std::floor(x)) + radiusX);
        const int minY = std::max(0, static_cast<int>(std::floor(y)) - radiusY);
        const int maxY = std::min(TEX_H - 1, static_cast<int>(std::floor(y)) + radiusY);

        for (int py = minY; py <= maxY; ++py) {
            const float dy = static_cast<float>(py) - y;
            const float dy2 = dy * dy;

            for (int px = minX; px <= maxX; ++px) {
                const float dx = static_cast<float>(px) - x;
                const float dx2 = dx * dx;
                const float core = std::exp(-((dx2 / coreXDen) + (dy2 / coreYDen)));
                const float halo = std::exp(-((dx2 / haloXDen) + (dy2 / haloYDen)));

                float energy = core * 0.82f + halo * 0.1f;
                energy *= 0.92f + 0.08f * std::cos((static_cast<float>(py) - y) * 1.4f);

                const float whiteCore = core * intensity * 0.04f;
                const int idx = (py * TEX_W + px) * 3;
                m_phosphor[idx]     += (dc.r * shadowMaskWeight(px, 0) * energy) + whiteCore;
                m_phosphor[idx + 1] += (dc.g * shadowMaskWeight(px, 1) * energy) + whiteCore;
                m_phosphor[idx + 2] += (dc.b * shadowMaskWeight(px, 2) * energy) + whiteCore;
            }
        }
    };

    bool hasPrev = false;
    DrawCall prev;
    for (const DrawCall& dc : draws) {
        if (hasPrev) {
            const float dxPixels = std::abs(dc.normX - prev.normX) * static_cast<float>(TEX_W);
            const float dyPixels = std::abs(dc.normY - prev.normY) * static_cast<float>(TEX_H);
            const float maxDeltaPixels = std::max(dxPixels, dyPixels);

            if (maxDeltaPixels <= 1.35f) {
                const int steps = std::clamp(static_cast<int>(std::ceil(maxDeltaPixels)), 1, 2);
                for (int step = 1; step <= steps; ++step) {
                    const float t = static_cast<float>(step) / static_cast<float>(steps);
                    depositSample({
                        lerpValue(prev.normX, dc.normX, t),
                        lerpValue(prev.normY, dc.normY, t),
                        lerpValue(prev.r, dc.r, t),
                        lerpValue(prev.g, dc.g, t),
                        lerpValue(prev.b, dc.b, t)
                    });
                }
                prev = dc;
                continue;
            }
        }

        depositSample(dc);
        prev = dc;
        hasPrev = true;
    }
}

void CRTRenderer::updateSweepOverlay(const QVector<DrawCall>& draws) {
    m_activeSweepDraws.clear();
    if (draws.isEmpty()) {
        return;
    }

    const int targetSamples = m_phy.usesVectorPlayback() ? 120 : 96;
    const int drawCount = static_cast<int>(draws.size());
    const int stride = std::max(1, drawCount / targetSamples);
    m_activeSweepDraws.reserve((drawCount / stride) + 1);

    for (int i = 0; i < drawCount; i += stride) {
        m_activeSweepDraws.push_back(draws[i]);
    }

    const DrawCall& last = draws.back();
    if (m_activeSweepDraws.isEmpty()
        || m_activeSweepDraws.back().normX != last.normX
        || m_activeSweepDraws.back().normY != last.normY
        || m_activeSweepDraws.back().r != last.r
        || m_activeSweepDraws.back().g != last.g
        || m_activeSweepDraws.back().b != last.b) {
        m_activeSweepDraws.push_back(last);
    }
}

void CRTRenderer::updateScreenImage() {
    auto* pixels = reinterpret_cast<QRgb*>(m_screenImg.bits());
    const int pixelCount = TEX_W * TEX_H;

    for (int i = 0; i < pixelCount; ++i) {
        const int phosphorIdx = i * 3;
        float r = 1.0f - std::exp(-m_phosphor[phosphorIdx] * 1.55f);
        float g = 1.0f - std::exp(-m_phosphor[phosphorIdx + 1] * 1.48f);
        float b = 1.0f - std::exp(-m_phosphor[phosphorIdx + 2] * 1.68f);

        const float luma = (r + g + b) / 3.0f;
        r = clampValue(std::pow(clampValue(r + (luma * 0.03f), 0.0f, 1.0f), 0.86f), 0.0f, 1.0f);
        g = clampValue(std::pow(clampValue(g + (luma * 0.04f), 0.0f, 1.0f), 0.84f), 0.0f, 1.0f);
        b = clampValue(std::pow(clampValue(b + (luma * 0.025f), 0.0f, 1.0f), 0.88f), 0.0f, 1.0f);

        pixels[i] = qRgb(static_cast<int>(r * 255.0f + 0.5f),
                         static_cast<int>(g * 255.0f + 0.5f),
                         static_cast<int>(b * 255.0f + 0.5f));
    }
}

float CRTRenderer::shadowMaskWeight(int x, int channel) const {
    static constexpr float kMask[3][3] = {
        {1.00f, 0.22f, 0.10f},
        {0.10f, 1.00f, 0.22f},
        {0.22f, 0.10f, 1.00f}
    };
    const int phase = ((x % 3) + 3) % 3;
    return kMask[phase][channel];
}

void CRTRenderer::drawBackground(QPainter& painter) {
    painter.fillRect(rect(), QColor(50, 57, 66));
    drawBackdropGrid(painter);
}

void CRTRenderer::drawBackdropGrid(QPainter& painter) {
    painter.save();

    painter.setPen(QPen(QColor(74, 84, 96), 1));
    for (int x = 0; x < width(); x += 40) {
        painter.drawLine(x, 0, x, height());
    }
    for (int y = 0; y < height(); y += 40) {
        painter.drawLine(0, y, width(), y);
    }

    painter.restore();
}

void CRTRenderer::drawScreen(QPainter& painter) {
    bool ok = true;
    const QPointF topLeft = projectPoint({-SCREEN_W * 0.5f, SCREEN_H * 0.5f, 0.0f}, &ok);
    if (!ok) {
        return;
    }
    const QPointF topRight = projectPoint({SCREEN_W * 0.5f, SCREEN_H * 0.5f, 0.0f}, &ok);
    if (!ok) {
        return;
    }
    const QPointF bottomRight = projectPoint({SCREEN_W * 0.5f, -SCREEN_H * 0.5f, 0.0f}, &ok);
    if (!ok) {
        return;
    }
    const QPointF bottomLeft = projectPoint({-SCREEN_W * 0.5f, -SCREEN_H * 0.5f, 0.0f}, &ok);
    if (!ok) {
        return;
    }

    const QPolygonF dstQuad{topLeft, topRight, bottomRight, bottomLeft};
    const QPolygonF srcQuad{
        QPointF(0.0, 0.0),
        QPointF(TEX_W, 0.0),
        QPointF(TEX_W, TEX_H),
        QPointF(0.0, TEX_H)
    };

    QTransform mapping;
    if (!QTransform::quadToQuad(srcQuad, dstQuad, mapping)) {
        return;
    }

    QPainterPath clip;
    clip.addPolygon(dstQuad);

    QPolygonF shadow = dstQuad;
    shadow.translate(5.0, 6.0);
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 70));
    painter.drawPolygon(shadow);
    painter.restore();

    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(40, 48, 56, 55));
    painter.drawPolygon(dstQuad);
    painter.restore();

    painter.save();
    painter.setClipPath(clip);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setWorldTransform(mapping, false);
    painter.drawImage(QPointF(0.0, 0.0), m_screenImg);
    painter.setPen(QPen(QColor(0, 0, 0, 38), 1.0));
    for (int y = 0; y < TEX_H; y += 5) {
        painter.drawLine(QPointF(0.0, y), QPointF(TEX_W, y));
    }
    drawSweepOverlay(painter);
    painter.restore();

    painter.save();
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(175, 188, 202), 2.0));
    painter.drawPolygon(dstQuad);
    painter.setPen(QPen(QColor(255, 255, 255, 55), 1.0));
    painter.drawPolyline(dstQuad);
    painter.restore();
}

void CRTRenderer::drawSweepOverlay(QPainter& painter) const {
    if (m_activeSweepDraws.isEmpty()) {
        return;
    }

    const bool vectorPlayback = m_phy.usesVectorPlayback();
    const float maxGap = vectorPlayback ? 20.0f : 2.1f;
    bool hasPrev = false;
    QPointF prevPoint;
    QColor prevColor;

    for (const DrawCall& dc : m_activeSweepDraws) {
        const float intensity = clampValue((dc.r + dc.g + dc.b) / 3.0f, 0.0f, 1.0f);
        if (intensity < 0.01f) {
            hasPrev = false;
            continue;
        }

        const QPointF point(dc.normX * static_cast<float>(TEX_W - 1),
                            dc.normY * static_cast<float>(TEX_H - 1));
        const QColor color = QColor::fromRgbF(clampValue(dc.r + 0.08f, 0.0f, 1.0f),
                                              clampValue(dc.g + 0.08f, 0.0f, 1.0f),
                                              clampValue(dc.b + 0.08f, 0.0f, 1.0f),
                                              1.0f);
        const float coreWidth = vectorPlayback ? (1.35f + intensity * 1.1f) : (0.85f + intensity * 0.5f);
        const float glowWidth = coreWidth + (vectorPlayback ? 2.4f : 1.25f);

        if (hasPrev && QLineF(prevPoint, point).length() <= maxGap) {
            QPen glow(QColor(color.red(), color.green(), color.blue(), vectorPlayback ? 58 : 34));
            glow.setWidthF(glowWidth);
            glow.setCapStyle(Qt::RoundCap);
            painter.setPen(glow);
            painter.drawLine(prevPoint, point);

            QPen core(prevColor);
            core.setWidthF(coreWidth);
            core.setCapStyle(Qt::RoundCap);
            painter.setPen(core);
            painter.drawLine(prevPoint, point);
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(color.red(), color.green(), color.blue(), vectorPlayback ? 90 : 60));
        painter.drawEllipse(point, coreWidth * 0.56f, coreWidth * 0.56f);

        prevPoint = point;
        prevColor = color;
        hasPrev = true;
    }
}

void CRTRenderer::drawTube(QPainter& painter) {
    constexpr int ringCount = 10;
    constexpr int segments = 24;

    QVector<QPolygonF> rings;
    rings.reserve(ringCount);

    for (int i = 0; i < ringCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(ringCount - 1);
        const float z = CATHODE_Z + t * (0.0f - CATHODE_Z);
        const float r = tubeRadiusAt(z);
        rings.push_back(projectCircle({0.0f, 0.0f, z}, {r, 0.0f, 0.0f}, {0.0f, r, 0.0f}, segments));
    }

    painter.save();
    for (int i = 0; i < rings.size(); ++i) {
        if (rings[i].size() < 3) {
            continue;
        }
        painter.setBrush(QColor(192, 201, 210, 8 + (i * 3)));
        painter.setPen(QPen(QColor(168, 178, 189, 28 + i * 4), 1.0));
        painter.drawPolygon(rings[i]);
    }

    painter.setPen(QPen(QColor(136, 147, 160, 58), 1.0));
    for (int i = 0; i + 1 < rings.size(); ++i) {
        if (rings[i].size() != segments || rings[i + 1].size() != segments) {
            continue;
        }
        for (int s = 0; s < segments; s += 3) {
            painter.drawLine(rings[i][s], rings[i + 1][s]);
        }
    }

    if (!rings.isEmpty() && rings.back().size() >= 3) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(208, 216, 224, 100), 1.8));
        painter.drawPolygon(rings.back());
    }
    painter.restore();
}

void CRTRenderer::drawCoils(QPainter& painter) {
    const auto drawDonut = [&](const QVector3D& center, const QVector3D& axisA, const QVector3D& axisB) {
        const QPolygonF outer = projectCircle(center, axisA, axisB, 28);
        const QPolygonF inner = projectCircle(center, axisA * 0.68f, axisB * 0.68f, 28);
        if (outer.size() < 3 || inner.size() < 3) {
            return;
        }

        QPainterPath path;
        path.setFillRule(Qt::OddEvenFill);
        path.addPolygon(outer);
        path.addPolygon(inner);

        painter.fillPath(path, QColor(158, 126, 68, 76));
        painter.setPen(QPen(QColor(184, 156, 101, 132), 1.3));
        painter.drawPath(path);
    };

    painter.save();
    drawDonut({0.0f, 8.5f, COIL_Z}, {8.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 8.0f});
    drawDonut({0.0f, -8.5f, COIL_Z}, {8.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 8.0f});
    drawDonut({-8.5f, 0.0f, COIL_Z}, {0.0f, 8.0f, 0.0f}, {0.0f, 0.0f, 8.0f});
    drawDonut({8.5f, 0.0f, COIL_Z}, {0.0f, 8.0f, 0.0f}, {0.0f, 0.0f, 8.0f});
    painter.restore();
}

void CRTRenderer::drawGuns(QPainter& painter) {
    const auto drawBarrel = [&](const QVector3D& start, const QVector3D& end, const QColor& color, float radius) {
        bool okA = false;
        bool okB = false;
        const QPointF a = projectPoint(start, &okA);
        const QPointF b = projectPoint(end, &okB);
        if (!okA || !okB) {
            return;
        }

        const float thickness = std::max(1.0f, projectedPixelRadius((start + end) * 0.5f, radius));
        QPen core(color);
        core.setWidthF(thickness);
        core.setCapStyle(Qt::RoundCap);
        painter.setPen(core);
        painter.drawLine(a, b);

        painter.setPen(Qt::NoPen);
        painter.setBrush(color.lighter(108));
        painter.drawEllipse(b, thickness * 0.52f, thickness * 0.52f);
    };

    painter.save();
    drawBarrel({0.0f, 0.0f, CATHODE_Z + 1.0f}, {0.0f, 0.0f, CATHODE_Z + 12.0f}, QColor(112, 118, 128), 4.2f);
    drawBarrel({-1.8f, 2.0f, CATHODE_Z + 12.0f}, {-1.8f, 2.0f, CATHODE_Z + 24.0f}, QColor(194, 92, 92), 0.9f);
    drawBarrel({1.8f, 2.0f, CATHODE_Z + 12.0f}, {1.8f, 2.0f, CATHODE_Z + 24.0f}, QColor(92, 176, 108), 0.9f);
    drawBarrel({0.0f, -2.0f, CATHODE_Z + 12.0f}, {0.0f, -2.0f, CATHODE_Z + 24.0f}, QColor(94, 128, 188), 0.9f);
    painter.restore();
}

void CRTRenderer::drawBeam(QPainter& painter) {
    const QVector3D cathode(0.0f, 0.0f, CATHODE_Z + 16.0f);
    const QVector3D coil(m_beam.hVolt * 10.0f, m_beam.vVolt * 10.0f, COIL_Z);
    const QVector3D screen(m_beam.hVolt * (SCREEN_W * 0.5f), m_beam.vVolt * (SCREEN_H * 0.5f), 0.0f);

    bool ok0 = false;
    bool ok1 = false;
    bool ok2 = false;
    const QPointF p0 = projectPoint(cathode, &ok0);
    const QPointF p1 = projectPoint(coil, &ok1);
    const QPointF p2 = projectPoint(screen, &ok2);
    if (!ok0 || !ok1 || !ok2) {
        return;
    }

    const float intensity = clampValue(m_beam.rAmp + m_beam.gAmp + m_beam.bAmp, 0.0f, 1.8f);
    const QColor coreColor = m_beam.active ? beamColor(m_beam) : QColor(230, 235, 255, 32);
    const float coreWidth = m_beam.active ? (1.9f + intensity * 1.6f) : 0.9f;

    painter.save();

    QPen glow(QColor(coreColor.red(), coreColor.green(), coreColor.blue(), m_beam.active ? 52 : 16));
    glow.setWidthF(coreWidth + 2.4f);
    glow.setCapStyle(Qt::RoundCap);
    painter.setPen(glow);
    painter.drawLine(p0, p1);
    painter.drawLine(p1, p2);

    QPen core(coreColor);
    core.setWidthF(coreWidth);
    core.setCapStyle(Qt::RoundCap);
    painter.setPen(core);
    painter.drawLine(p0, p1);
    painter.drawLine(p1, p2);

    if (m_beam.active) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(coreColor.red(), coreColor.green(), coreColor.blue(), 132));
        const float radius = projectedPixelRadius(screen, 0.75f) + intensity * 1.8f;
        painter.drawEllipse(p2, radius, radius);
    }

    painter.restore();
}

void CRTRenderer::drawOverlayHints(QPainter& painter) {
    const qreal boxWidth = std::min<qreal>(360.0, std::max<qreal>(220.0, width() - 32.0));
    const QRectF box(16.0, height() - 46.0, boxWidth, 30.0);
    painter.save();
    painter.setPen(QPen(QColor(133, 145, 160), 1.0));
    painter.setBrush(QColor(75, 83, 93, 170));
    painter.drawRect(box);
    painter.setPen(QColor(232, 238, 245, 210));
    painter.setFont(QFont(QStringLiteral("Tahoma"), 8));
    painter.drawText(box.adjusted(10.0, 0.0, -10.0, 0.0),
                     Qt::AlignVCenter | Qt::AlignLeading,
                     uiText(m_language).rendererHint);
    painter.restore();
}

float CRTRenderer::tubeRadiusAt(float z) const {
    if (z <= COIL_Z) {
        return 5.5f;
    }

    const float t = clampValue((z - COIL_Z) / (0.0f - COIL_Z), 0.0f, 1.0f);
    return 5.5f + std::pow(t, 2.45f) * 39.5f;
}

QVector3D CRTRenderer::cameraPosition() const {
    const float cosPhi = std::cos(m_phi);
    return {
        m_target.x() + m_radius * cosPhi * std::cos(m_theta),
        m_target.y() + m_radius * std::sin(m_phi),
        m_target.z() + m_radius * cosPhi * std::sin(m_theta)
    };
}

QMatrix4x4 CRTRenderer::viewMatrix() const {
    return m_view;
}

QPointF CRTRenderer::projectPoint(const QVector3D& point, bool* ok) const {
    const QVector4D clip = m_viewProj * QVector4D(point, 1.0f);
    if (clip.w() <= 0.001f) {
        if (ok) {
            *ok = false;
        }
        return {};
    }

    const QVector3D ndc = clip.toVector3DAffine();
    if (ok) {
        *ok = true;
    }

    return {
        (ndc.x() + 1.0f) * 0.5f * width(),
        (1.0f - ndc.y()) * 0.5f * height()
    };
}

float CRTRenderer::projectedPixelRadius(const QVector3D& center, float worldRadius) const {
    const QVector4D viewSpace = m_view * QVector4D(center, 1.0f);
    const float depth = std::max(1.0f, -viewSpace.z());
    const float fov = qDegreesToRadians(45.0f);
    const float pixelsPerUnit = height() / (2.0f * std::tan(fov * 0.5f) * depth);
    return std::max(0.75f, worldRadius * pixelsPerUnit);
}

QPolygonF CRTRenderer::projectCircle(const QVector3D& center,
                                     const QVector3D& axisA,
                                     const QVector3D& axisB,
                                     int segments) const {
    QPolygonF polygon;
    polygon.reserve(segments);

    for (int i = 0; i < segments; ++i) {
        const float angle = (static_cast<float>(i) / static_cast<float>(segments)) * (2.0f * kPi);
        const QVector3D point = center + axisA * std::cos(angle) + axisB * std::sin(angle);
        bool ok = false;
        const QPointF projected = projectPoint(point, &ok);
        if (!ok) {
            return {};
        }
        polygon << projected;
    }

    return polygon;
}
