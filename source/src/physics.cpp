#include "physics.h"

#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace {
constexpr float kPi = 3.1415926535f;

struct RawVectorCmd {
    double x = 0.0;
    double y = 0.0;
    double r = 1.0;
    double g = 1.0;
    double b = 1.0;
    bool hasPoint = false;
    bool hasColor = false;
    bool draw = true;
    bool directAxes = false;
};

struct CanvasHints {
    double width = -1.0;
    double height = -1.0;
};

struct AxisRange {
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();
    bool valid = false;

    void include(double value) {
        min = std::min(min, value);
        max = std::max(max, value);
        valid = true;
    }

    double span() const {
        return valid ? (max - min) : 0.0;
    }
};

float jsonToFloat(const QJsonValue& value) {
    return value.isString() ? value.toString().toFloat()
                            : static_cast<float>(value.toDouble());
}

bool jsonToDouble(const QJsonValue& value, double& output) {
    if (value.isDouble()) {
        output = value.toDouble();
        return true;
    }

    if (value.isString()) {
        bool ok = false;
        output = value.toString().toDouble(&ok);
        return ok;
    }

    return false;
}

bool jsonToBool(const QJsonValue& value, bool& output) {
    if (value.isBool()) {
        output = value.toBool();
        return true;
    }

    if (value.isDouble()) {
        output = !qFuzzyIsNull(value.toDouble());
        return true;
    }

    if (value.isString()) {
        const QString text = value.toString().trimmed().toLower();
        if (text == QStringLiteral("true") || text == QStringLiteral("yes") || text == QStringLiteral("on")) {
            output = true;
            return true;
        }
        if (text == QStringLiteral("false") || text == QStringLiteral("no") || text == QStringLiteral("off")) {
            output = false;
            return true;
        }
    }

    return false;
}

float clampFloat(float value, float minimum, float maximum) {
    return std::max(minimum, std::min(value, maximum));
}

double clampDouble(double value, double minimum, double maximum) {
    return std::max(minimum, std::min(value, maximum));
}

float lerpFloat(float a, float b, float t) {
    return a + (b - a) * t;
}

QImage buildDefaultPattern(const QSize& size) {
    QImage image(size, QImage::Format_RGB32);
    image.fill(Qt::black);

    constexpr int colorCount = 7;
    const QColor colors[] = {
        Qt::white, Qt::yellow, Qt::cyan, Qt::green,
        Qt::magenta, Qt::red, Qt::blue
    };
    const qreal barWidth = static_cast<qreal>(size.width()) / static_cast<qreal>(colorCount);

    QPainter painter(&image);
    for (int i = 0; i < colorCount; ++i) {
        painter.fillRect(QRectF(i * barWidth, 0.0, barWidth, size.height()), colors[i]);
    }

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::white);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(size.width() / 2.0, size.height() / 2.0),
                        size.width() * 0.125,
                        size.height() * 0.17);

    QPen framePen(QColor(255, 255, 255, 150));
    framePen.setWidthF(std::max(1.0, size.width() * 0.008));
    painter.setBrush(Qt::NoBrush);
    painter.setPen(framePen);
    painter.drawRoundedRect(QRectF(size.width() * 0.18,
                                   size.height() * 0.2,
                                   size.width() * 0.64,
                                   size.height() * 0.6),
                            size.width() * 0.04,
                            size.width() * 0.04);
    painter.end();

    return image;
}

void copyImageToBuffer(const QImage& image, QVector<quint8>& buffer) {
    buffer.resize(image.width() * image.height() * 4);
    std::memcpy(buffer.data(), image.constBits(), static_cast<size_t>(buffer.size()));
}

bool readNumber(const QJsonObject& object, std::initializer_list<const char*> keys, double& output) {
    for (const char* key : keys) {
        const auto it = object.find(QString::fromLatin1(key));
        if (it != object.end() && jsonToDouble(*it, output)) {
            return true;
        }
    }
    return false;
}

bool readBool(const QJsonObject& object, std::initializer_list<const char*> keys, bool& output) {
    for (const char* key : keys) {
        const auto it = object.find(QString::fromLatin1(key));
        if (it != object.end() && jsonToBool(*it, output)) {
            return true;
        }
    }
    return false;
}

bool parsePointArray(const QJsonArray& array, RawVectorCmd& cmd) {
    if (array.size() < 2) {
        return false;
    }

    if (!jsonToDouble(array[0], cmd.x) || !jsonToDouble(array[1], cmd.y)) {
        return false;
    }

    cmd.hasPoint = true;
    cmd.directAxes = false;

    double component = 0.0;
    if (array.size() >= 5
        && jsonToDouble(array[2], component)) {
        cmd.r = component;
        if (jsonToDouble(array[3], cmd.g) && jsonToDouble(array[4], cmd.b)) {
            cmd.hasColor = true;
        }
    }

    if (array.size() >= 6) {
        bool draw = true;
        if (jsonToBool(array[5], draw)) {
            cmd.draw = draw;
        }
    }

    return true;
}

bool parseObjectPoint(const QJsonObject& object, RawVectorCmd& cmd) {
    double x = 0.0;
    double y = 0.0;
    bool hasPoint = false;

    if (readNumber(object, {"H", "h", "horizontal", "Horizontal"}, x)
        && readNumber(object, {"V", "v", "vertical", "Vertical"}, y)) {
        cmd.directAxes = true;
        hasPoint = true;
    } else if (readNumber(object, {"X", "x", "left", "cx"}, x)
               && readNumber(object, {"Y", "y", "top", "cy"}, y)) {
        cmd.directAxes = false;
        hasPoint = true;
    } else if (object.contains(QStringLiteral("pos")) && object.value(QStringLiteral("pos")).isObject()) {
        const QJsonObject pos = object.value(QStringLiteral("pos")).toObject();
        if (readNumber(pos, {"x", "X", "h", "H"}, x)
            && readNumber(pos, {"y", "Y", "v", "V"}, y)) {
            cmd.directAxes = pos.contains(QStringLiteral("h")) || pos.contains(QStringLiteral("H"));
            hasPoint = true;
        }
    } else if (object.contains(QStringLiteral("point")) && object.value(QStringLiteral("point")).isArray()) {
        RawVectorCmd arrayPoint;
        if (parsePointArray(object.value(QStringLiteral("point")).toArray(), arrayPoint)) {
            cmd = arrayPoint;
            hasPoint = true;
        }
    }

    if (!hasPoint) {
        return false;
    }

    cmd.x = x;
    cmd.y = y;
    cmd.hasPoint = true;

    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    if (readNumber(object, {"R", "r", "red"}, r)
        && readNumber(object, {"G", "g", "green"}, g)
        && readNumber(object, {"B", "b", "blue"}, b)) {
        cmd.r = r;
        cmd.g = g;
        cmd.b = b;
        cmd.hasColor = true;
    } else if (object.contains(QStringLiteral("color")) && object.value(QStringLiteral("color")).isObject()) {
        const QJsonObject color = object.value(QStringLiteral("color")).toObject();
        if (readNumber(color, {"r", "R", "red"}, r)
            && readNumber(color, {"g", "G", "green"}, g)
            && readNumber(color, {"b", "B", "blue"}, b)) {
            cmd.r = r;
            cmd.g = g;
            cmd.b = b;
            cmd.hasColor = true;
        }
    } else if (object.contains(QStringLiteral("color")) && object.value(QStringLiteral("color")).isArray()) {
        const QJsonArray color = object.value(QStringLiteral("color")).toArray();
        if (color.size() >= 3
            && jsonToDouble(color[0], r)
            && jsonToDouble(color[1], g)
            && jsonToDouble(color[2], b)) {
            cmd.r = r;
            cmd.g = g;
            cmd.b = b;
            cmd.hasColor = true;
        }
    }

    bool draw = true;
    if (readBool(object, {"draw", "penDown", "beamOn", "visible"}, draw)) {
        cmd.draw = draw;
    }

    bool blank = false;
    if (readBool(object, {"blank", "penUp"}, blank) && blank) {
        cmd.draw = false;
    }

    return true;
}

void appendRawCommand(const RawVectorCmd& input, QVector<RawVectorCmd>& out, bool startNewStroke) {
    if (!input.hasPoint) {
        return;
    }

    RawVectorCmd cmd = input;
    if (!cmd.hasColor && cmd.draw) {
        cmd.r = 1.0;
        cmd.g = 1.0;
        cmd.b = 1.0;
    }
    if (!cmd.draw) {
        cmd.r = 0.0;
        cmd.g = 0.0;
        cmd.b = 0.0;
        cmd.hasColor = true;
    }

    if (startNewStroke && !out.isEmpty()) {
        RawVectorCmd blankMove = cmd;
        blankMove.draw = false;
        blankMove.r = 0.0;
        blankMove.g = 0.0;
        blankMove.b = 0.0;
        blankMove.hasColor = true;
        out.push_back(blankMove);
    }

    out.push_back(cmd);
}

bool isPointArray(const QJsonArray& array) {
    if (array.size() < 2) {
        return false;
    }

    double x = 0.0;
    double y = 0.0;
    return jsonToDouble(array[0], x) && jsonToDouble(array[1], y);
}

bool appendFlatCommands(const QJsonArray& array, QVector<RawVectorCmd>& out, bool startNewStroke = false) {
    bool appended = false;
    bool strokeBreakPending = startNewStroke;

    for (const QJsonValue& value : array) {
        RawVectorCmd cmd;
        bool valid = false;

        if (value.isObject()) {
            valid = parseObjectPoint(value.toObject(), cmd);
        } else if (value.isArray()) {
            valid = parsePointArray(value.toArray(), cmd);
        }

        if (!valid) {
            continue;
        }

        appendRawCommand(cmd, out, strokeBreakPending);
        strokeBreakPending = false;
        appended = true;
    }

    return appended;
}

bool appendStrokeCollection(const QJsonArray& strokes, QVector<RawVectorCmd>& out) {
    bool appended = false;
    bool firstStroke = true;

    for (const QJsonValue& strokeValue : strokes) {
        QJsonArray points;

        if (strokeValue.isArray()) {
            points = strokeValue.toArray();
        } else if (strokeValue.isObject()) {
            const QJsonObject strokeObject = strokeValue.toObject();
            if (strokeObject.contains(QStringLiteral("points")) && strokeObject.value(QStringLiteral("points")).isArray()) {
                points = strokeObject.value(QStringLiteral("points")).toArray();
            } else if (strokeObject.contains(QStringLiteral("signals")) && strokeObject.value(QStringLiteral("signals")).isArray()) {
                points = strokeObject.value(QStringLiteral("signals")).toArray();
            } else if (strokeObject.contains(QStringLiteral("commands")) && strokeObject.value(QStringLiteral("commands")).isArray()) {
                points = strokeObject.value(QStringLiteral("commands")).toArray();
            }
        }

        if (points.isEmpty()) {
            continue;
        }

        if (appendFlatCommands(points, out, !firstStroke)) {
            appended = true;
            firstStroke = false;
        }
    }

    return appended;
}

CanvasHints extractCanvasHints(const QJsonObject& root) {
    CanvasHints hints;
    double value = -1.0;

    if (readNumber(root, {"width", "canvasWidth", "viewportWidth"}, value)) {
        hints.width = value;
    }
    if (readNumber(root, {"height", "canvasHeight", "viewportHeight"}, value)) {
        hints.height = value;
    }

    if (root.contains(QStringLiteral("meta")) && root.value(QStringLiteral("meta")).isObject()) {
        const QJsonObject meta = root.value(QStringLiteral("meta")).toObject();
        if (hints.width <= 0.0 && readNumber(meta, {"width", "canvasWidth", "viewportWidth"}, value)) {
            hints.width = value;
        }
        if (hints.height <= 0.0 && readNumber(meta, {"height", "canvasHeight", "viewportHeight"}, value)) {
            hints.height = value;
        }
    }

    return hints;
}

float normalizeAxis(double value,
                    const AxisRange& range,
                    double extentHint,
                    bool directAxis,
                    bool verticalAxis) {
    if (range.valid && range.min >= -1.001 && range.max <= 1.001) {
        if (!directAxis && range.min >= 0.0) {
            const double unit = clampDouble(value, 0.0, 1.0);
            return verticalAxis ? static_cast<float>(1.0 - (unit * 2.0))
                                : static_cast<float>((unit * 2.0) - 1.0);
        }
        return clampFloat(static_cast<float>(value), -1.0f, 1.0f);
    }

    if (extentHint > 1.0) {
        const double denominator = std::max(1.0, extentHint - 1.0);
        const double unit = clampDouble(value / denominator, 0.0, 1.0);
        const double mapped = directAxis
                                  ? ((unit * 2.0) - 1.0)
                                  : (verticalAxis ? (1.0 - (unit * 2.0)) : ((unit * 2.0) - 1.0));
        return clampFloat(static_cast<float>(mapped), -1.0f, 1.0f);
    }

    if (range.valid && range.span() > 0.000001) {
        const double unit = clampDouble((value - range.min) / range.span(), 0.0, 1.0);
        const double mapped = directAxis
                                  ? ((unit * 2.0) - 1.0)
                                  : (verticalAxis ? (1.0 - (unit * 2.0)) : ((unit * 2.0) - 1.0));
        return clampFloat(static_cast<float>(mapped), -1.0f, 1.0f);
    }

    const float fallback = static_cast<float>(directAxis || !verticalAxis ? value : -value);
    return clampFloat(fallback, -1.0f, 1.0f);
}

QVector<VectorCmd> normalizeCommands(const QVector<RawVectorCmd>& raw, const CanvasHints& hints) {
    QVector<VectorCmd> output;
    output.reserve(raw.size());

    AxisRange xRange;
    AxisRange yRange;
    for (const RawVectorCmd& cmd : raw) {
        if (!cmd.hasPoint) {
            continue;
        }
        xRange.include(cmd.x);
        yRange.include(cmd.y);
    }

    for (const RawVectorCmd& cmd : raw) {
        if (!cmd.hasPoint) {
            continue;
        }

        output.push_back({
            normalizeAxis(cmd.x, xRange, hints.width, cmd.directAxes, false),
            normalizeAxis(cmd.y, yRange, hints.height, cmd.directAxes, true),
            clampFloat(static_cast<float>(cmd.r), 0.0f, 1.0f),
            clampFloat(static_cast<float>(cmd.g), 0.0f, 1.0f),
            clampFloat(static_cast<float>(cmd.b), 0.0f, 1.0f)
        });
    }

    return output;
}

bool sameVectorCmd(const VectorCmd& a, const VectorCmd& b) {
    return std::abs(a.H - b.H) < 0.0005f
        && std::abs(a.V - b.V) < 0.0005f
        && std::abs(a.R - b.R) < 0.0005f
        && std::abs(a.G - b.G) < 0.0005f
        && std::abs(a.B - b.B) < 0.0005f;
}

bool isLitVectorCmd(const VectorCmd& cmd) {
    return (cmd.R + cmd.G + cmd.B) > 0.01f;
}

QPoint commandPointInImage(const VectorCmd& cmd, const QSize& size) {
    const float unitX = clampFloat((cmd.H + 1.0f) * 0.5f, 0.0f, 1.0f);
    const float unitY = clampFloat((-cmd.V + 1.0f) * 0.5f, 0.0f, 1.0f);
    return {
        static_cast<int>(std::lround(unitX * static_cast<float>(size.width() - 1))),
        static_cast<int>(std::lround(unitY * static_cast<float>(size.height() - 1)))
    };
}

QColor commandColor(const VectorCmd& cmd) {
    return QColor::fromRgbF(clampFloat(cmd.R, 0.0f, 1.0f),
                            clampFloat(cmd.G, 0.0f, 1.0f),
                            clampFloat(cmd.B, 0.0f, 1.0f),
                            1.0f);
}

void brightenPixel(QImage& image, const QPoint& point, const QColor& color) {
    if (!QRect(QPoint(0, 0), image.size()).contains(point)) {
        return;
    }

    const QRgb current = image.pixel(point);
    image.setPixel(point,
                   qRgb(std::max(qRed(current), color.red()),
                        std::max(qGreen(current), color.green()),
                        std::max(qBlue(current), color.blue())));
}

QImage rasterizeCommandsToImage(const QVector<VectorCmd>& commands, const QSize& size) {
    QImage image(size, QImage::Format_RGB32);
    image.fill(Qt::black);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const float maxLinkDistance = std::max(1.5f,
                                           (static_cast<float>(size.width())
                                            / static_cast<float>(PhysicsEngine::ANA_W)) * 1.6f);

    bool hasPrev = false;
    QPoint prevPoint;

    for (const VectorCmd& cmd : commands) {
        if (!isLitVectorCmd(cmd)) {
            hasPrev = false;
            continue;
        }

        const QPoint point = commandPointInImage(cmd, size);
        const QColor color = commandColor(cmd);

        if (hasPrev) {
            const float dx = static_cast<float>(point.x() - prevPoint.x());
            const float dy = static_cast<float>(point.y() - prevPoint.y());
            if (std::max(std::abs(dx), std::abs(dy)) <= maxLinkDistance) {
                painter.setPen(color);
                painter.drawLine(prevPoint, point);
            }
        }

        brightenPixel(image, point, color);
        prevPoint = point;
        hasPrev = true;
    }

    painter.end();
    return image;
}

QVector<VectorCmd> optimizeCommands(const QVector<VectorCmd>& input) {
    QVector<VectorCmd> output;
    output.reserve(input.size());

    bool pendingBlank = false;
    VectorCmd lastBlank;

    for (const VectorCmd& cmd : input) {
        if (!isLitVectorCmd(cmd)) {
            if (!output.isEmpty()) {
                pendingBlank = true;
                lastBlank = cmd;
            }
            continue;
        }

        if (pendingBlank) {
            if (output.isEmpty() || !sameVectorCmd(output.back(), lastBlank)) {
                output.push_back(lastBlank);
            }
            pendingBlank = false;
        }

        if (!output.isEmpty()) {
            const VectorCmd& prev = output.back();
            if (sameVectorCmd(prev, cmd)) {
                continue;
            }

            if (isLitVectorCmd(prev)) {
                const float delta = std::max(std::abs(prev.H - cmd.H), std::abs(prev.V - cmd.V));
                const float colorDelta = std::max({std::abs(prev.R - cmd.R),
                                                   std::abs(prev.G - cmd.G),
                                                   std::abs(prev.B - cmd.B)});
                if (delta < 0.0009f && colorDelta < 0.0015f) {
                    continue;
                }
            }
        }

        output.push_back(cmd);
    }

    return output;
}
}

PhysicsEngine::PhysicsEngine() {
    m_pixels.fill(0, ANA_W * ANA_H * 4);
    generateDefaultPattern();
}

void PhysicsEngine::generateDefaultPattern() {
    const QImage analysisImage = buildDefaultPattern({ANA_W, ANA_H}).convertToFormat(QImage::Format_RGBA8888);
    m_rasterSource = buildDefaultPattern({RASTER_W, RASTER_H}).convertToFormat(QImage::Format_RGB32);

    copyImageToBuffer(analysisImage, m_pixels);

    m_cmds.clear();
    m_vectorMode = false;
    m_rasterizedCommandPlayback = false;
    m_cmdIdx = 0;
    resetRasterScan();
}

bool PhysicsEngine::loadImage(const QString& path) {
    QImage image(path);
    if (image.isNull()) {
        return false;
    }

    const QImage analysisImage = image.scaled(ANA_W, ANA_H, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                                     .convertToFormat(QImage::Format_RGBA8888);
    m_rasterSource = image.scaled(RASTER_W, RASTER_H, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                         .convertToFormat(QImage::Format_RGB32);

    copyImageToBuffer(analysisImage, m_pixels);

    m_cmds.clear();
    m_vectorMode = false;
    m_rasterizedCommandPlayback = false;
    m_cmdIdx = 0;
    resetRasterScan();
    return true;
}

bool PhysicsEngine::loadCommands(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        return false;
    }

    QVector<RawVectorCmd> rawCommands;
    CanvasHints hints;
    bool parsed = false;

    if (doc.isObject()) {
        const QJsonObject root = doc.object();
        hints = extractCanvasHints(root);

        if (root.contains(QStringLiteral("signals")) && root.value(QStringLiteral("signals")).isArray()) {
            parsed = appendFlatCommands(root.value(QStringLiteral("signals")).toArray(), rawCommands);
        } else if (root.contains(QStringLiteral("commands")) && root.value(QStringLiteral("commands")).isArray()) {
            parsed = appendFlatCommands(root.value(QStringLiteral("commands")).toArray(), rawCommands);
        } else if (root.contains(QStringLiteral("points")) && root.value(QStringLiteral("points")).isArray()) {
            parsed = appendFlatCommands(root.value(QStringLiteral("points")).toArray(), rawCommands);
        } else if (root.contains(QStringLiteral("strokes")) && root.value(QStringLiteral("strokes")).isArray()) {
            parsed = appendStrokeCollection(root.value(QStringLiteral("strokes")).toArray(), rawCommands);
        } else if (root.contains(QStringLiteral("paths")) && root.value(QStringLiteral("paths")).isArray()) {
            parsed = appendStrokeCollection(root.value(QStringLiteral("paths")).toArray(), rawCommands);
        }
    } else if (doc.isArray()) {
        const QJsonArray rootArray = doc.array();
        if (!rootArray.isEmpty() && rootArray.first().isArray() && !isPointArray(rootArray.first().toArray())) {
            parsed = appendStrokeCollection(rootArray, rawCommands);
        } else {
            parsed = appendFlatCommands(rootArray, rawCommands);
        }
    }

    QVector<VectorCmd> parsedCommands = optimizeCommands(normalizeCommands(rawCommands, hints));
    if (!parsed || parsedCommands.isEmpty()) {
        return false;
    }

    m_cmds = std::move(parsedCommands);
    updateRasterBuffersFromCommands(m_cmds);
    m_vectorMode = true;
    m_rasterizedCommandPlayback = true;
    m_cmdIdx = 0;
    resetRasterScan();
    return true;
}

void PhysicsEngine::updateRasterBuffersFromCommands(const QVector<VectorCmd>& commands) {
    const QImage analysisImage = rasterizeCommandsToImage(commands, {ANA_W, ANA_H})
                                     .convertToFormat(QImage::Format_RGBA8888);
    m_rasterSource = rasterizeCommandsToImage(commands, {RASTER_W, RASTER_H})
                         .convertToFormat(QImage::Format_RGB32);
    copyImageToBuffer(analysisImage, m_pixels);
}

bool PhysicsEngine::exportCommands(const QString& path, ExportStats* stats) const {
    QJsonObject meta;
    meta[QStringLiteral("type")] = QStringLiteral("Smart Vector Gun Commands");
    meta[QStringLiteral("resolution")] = QStringLiteral("Independent (-1.0 to 1.0 Voltage Scale)");
    meta[QStringLiteral("compression")] = QStringLiteral("Zero-Skipping (Black pixels omitted)");
    meta[QStringLiteral("note")] = QStringLiteral("The beam is emitted only for illuminated pixels while dark regions are skipped.");
    meta[QStringLiteral("canvasWidth")] = ANA_W;
    meta[QStringLiteral("canvasHeight")] = ANA_H;

    ExportStats localStats;
    localStats.total = ANA_W * ANA_H;

    QJsonArray signalArray;
    for (int y = 0; y < ANA_H; ++y) {
        for (int x = 0; x < ANA_W; ++x) {
            const int idx = (y * ANA_W + x) * 4;
            const float rAmp = m_pixels[idx] / 255.0f;
            const float gAmp = m_pixels[idx + 1] / 255.0f;
            const float bAmp = m_pixels[idx + 2] / 255.0f;

            if ((rAmp + gAmp + bAmp) < 0.05f) {
                ++localStats.skipped;
                continue;
            }

            const float hVolt = (static_cast<float>(x) / static_cast<float>(ANA_W)) * 2.0f - 1.0f;
            const float vVolt = -((static_cast<float>(y) / static_cast<float>(ANA_H)) * 2.0f - 1.0f);

            QJsonObject command;
            command[QStringLiteral("H")] = QString::number(hVolt, 'f', 3);
            command[QStringLiteral("V")] = QString::number(vVolt, 'f', 3);
            command[QStringLiteral("R")] = QString::number(rAmp, 'f', 2);
            command[QStringLiteral("G")] = QString::number(gAmp, 'f', 2);
            command[QStringLiteral("B")] = QString::number(bAmp, 'f', 2);
            signalArray.append(command);
            ++localStats.emitted;
        }
    }

    QJsonObject root;
    root[QStringLiteral("width")] = ANA_W;
    root[QStringLiteral("height")] = ANA_H;
    root[QStringLiteral("meta")] = meta;
    root[QStringLiteral("signals")] = signalArray;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    if (stats) {
        *stats = localStats;
    }
    return true;
}

BeamState PhysicsEngine::step(int iterations, bool hEnabled, bool vEnabled, QVector<DrawCall>& outDraws) {
    BeamState state;
    outDraws.clear();
    outDraws.reserve(iterations);

    const bool vectorPlayback = usesVectorPlayback();
    if (vectorPlayback && m_cmds.isEmpty()) {
        return state;
    }

    const auto emitDraw = [&](float hVolt, float vVolt, float rAmp, float gAmp, float bAmp) {
        outDraws.push_back({
            clampFloat((hVolt + 1.0f) * 0.5f, 0.0f, 1.0f),
            clampFloat((-vVolt + 1.0f) * 0.5f, 0.0f, 1.0f),
            clampFloat(rAmp, 0.0f, 1.0f),
            clampFloat(gAmp, 0.0f, 1.0f),
            clampFloat(bAmp, 0.0f, 1.0f)
        });
    };

    const QRgb* rasterPixels = vectorPlayback ? nullptr : reinterpret_cast<const QRgb*>(m_rasterSource.constBits());
    const int rasterStride = vectorPlayback ? 0 : (m_rasterSource.bytesPerLine() / static_cast<int>(sizeof(QRgb)));

    for (int i = 0; i < iterations; ++i) {
        if (vectorPlayback) {
            const VectorCmd& cmd = m_cmds[m_cmdIdx];
            state.hVolt = hEnabled ? cmd.H : 0.0f;
            state.vVolt = vEnabled ? cmd.V : 0.0f;
            state.rAmp = cmd.R;
            state.gAmp = cmd.G;
            state.bAmp = cmd.B;
            state.active = (cmd.R + cmd.G + cmd.B) > 0.01f;
            emitDraw(state.hVolt, state.vVolt, state.active ? cmd.R : 0.0f, state.active ? cmd.G : 0.0f, state.active ? cmd.B : 0.0f);
            ++m_cmdIdx;
            if (m_cmdIdx >= m_cmds.size()) {
                m_cmdIdx = 0;
                break;
            }
            continue;
        }

        const bool activeVideo = (m_rasterX < RASTER_W) && (m_rasterY < RASTER_H);

        float sweepH = 0.0f;
        float sweepV = 0.0f;

        if (activeVideo) {
            sweepH = ((static_cast<float>(m_rasterX) + 0.5f) / static_cast<float>(RASTER_W)) * 2.0f - 1.0f;
            sweepV = -(((static_cast<float>(m_rasterY) + 0.5f) / static_cast<float>(RASTER_H)) * 2.0f - 1.0f);
        } else if (m_rasterY < RASTER_H) {
            const float t = (static_cast<float>(m_rasterX - RASTER_W) + 0.5f) / static_cast<float>(H_BLANK);
            const float currentV = -(((static_cast<float>(m_rasterY) + 0.5f) / static_cast<float>(RASTER_H)) * 2.0f - 1.0f);
            const float nextV = -(((static_cast<float>(std::min(m_rasterY + 1, RASTER_H - 1)) + 0.5f)
                                    / static_cast<float>(RASTER_H)) * 2.0f - 1.0f);
            sweepH = lerpFloat(1.04f, -1.06f, t);
            sweepV = lerpFloat(currentV, nextV, t);
        } else {
            const int lineSpan = RASTER_W + H_BLANK;
            const int retraceIndex = (m_rasterY - RASTER_H) * lineSpan + m_rasterX;
            const float t = (static_cast<float>(retraceIndex) + 0.5f)
                            / static_cast<float>(lineSpan * V_BLANK);
            sweepH = -1.06f + std::sin(t * 2.0f * kPi) * 0.05f;
            sweepV = lerpFloat(-1.06f, 1.06f, t);
        }

        state.hVolt = hEnabled ? sweepH : 0.0f;
        state.vVolt = vEnabled ? sweepV : 0.0f;
        state.rAmp = 0.0f;
        state.gAmp = 0.0f;
        state.bAmp = 0.0f;
        state.active = false;

        if (activeVideo) {
            const QRgb pixel = rasterPixels[(m_rasterY * rasterStride) + m_rasterX];
            state.rAmp = qRed(pixel) / 255.0f;
            state.gAmp = qGreen(pixel) / 255.0f;
            state.bAmp = qBlue(pixel) / 255.0f;
            state.active = (state.rAmp + state.gAmp + state.bAmp) > 0.015f;

            if (state.active) {
                emitDraw(state.hVolt, state.vVolt, state.rAmp, state.gAmp, state.bAmp);
            }
        }

        ++m_rasterX;
        if (m_rasterX >= (RASTER_W + H_BLANK)) {
            m_rasterX = 0;
            ++m_rasterY;
            if (m_rasterY >= (RASTER_H + V_BLANK)) {
                m_rasterY = 0;
            }
        }
    }

    return state;
}

void PhysicsEngine::resetRasterScan() {
    m_rasterX = 0;
    m_rasterY = 0;
}
