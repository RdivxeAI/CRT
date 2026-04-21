#pragma once

#include <QImage>
#include <QVector>
#include <QString>

struct BeamState {
    float hVolt = 0.f;   // -1 to +1
    float vVolt = 0.f;
    float rAmp = 0.f;    //  0 to  1
    float gAmp = 0.f;
    float bAmp = 0.f;
    bool active = false;
};

struct DrawCall {
    float normX = 0.f;
    float normY = 0.f;
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
};

struct VectorCmd {
    float H = 0.f;
    float V = 0.f;
    float R = 0.f;
    float G = 0.f;
    float B = 0.f;
};

struct ExportStats {
    int emitted = 0;
    int skipped = 0;
    int total = 0;

    double compressionRatio() const {
        return total > 0 ? (static_cast<double>(skipped) / static_cast<double>(total)) * 100.0 : 0.0;
    }
};

class PhysicsEngine {
public:
    static constexpr int ANA_W = 160;
    static constexpr int ANA_H = 120;
    static constexpr int RASTER_W = 320;
    static constexpr int RASTER_H = 240;
    static constexpr int H_BLANK = 48;
    static constexpr int V_BLANK = 18;

    PhysicsEngine();

    void generateDefaultPattern();
    bool loadImage(const QString& path);
    bool loadCommands(const QString& path);
    bool exportCommands(const QString& path, ExportStats* stats = nullptr) const;

    BeamState step(int iterations, bool hEnabled, bool vEnabled, QVector<DrawCall>& outDraws);

    bool isVectorMode() const { return m_vectorMode; }
    bool usesVectorPlayback() const { return m_vectorMode && !m_rasterizedCommandPlayback; }
    int commandCount() const { return m_cmds.size(); }

private:
    void resetRasterScan();
    void updateRasterBuffersFromCommands(const QVector<VectorCmd>& commands);

    QVector<quint8> m_pixels;
    QImage m_rasterSource;
    QVector<VectorCmd> m_cmds;
    int m_rasterX = 0;
    int m_rasterY = 0;
    int m_cmdIdx = 0;
    bool m_vectorMode = false;
    bool m_rasterizedCommandPlayback = false;
};
