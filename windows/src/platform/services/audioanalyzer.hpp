#pragma once

#include <QVector>

#include <vector>

// Real-time spectrum analyser for the WASAPI loopback stream. It owns no audio
// samples once the FFT window has been analysed and allocates only at
// construction / sample-rate changes.
class AudioAnalyzer {
public:
    static constexpr int BandCount = 64;

    AudioAnalyzer();

    void setSampleRate(int sampleRate);
    void reset();
    void pushSamples(const float* samples, int count);
    [[nodiscard]] QVector<float> bands() const { return m_bands; }

private:
    void analyze();
    void rebuildBands();

    int m_sampleRate = 48000;
    int m_fftSize = 2048;
    std::vector<float> m_ring;
    int m_ringPos = 0;
    int m_filled = 0;
    std::vector<float> m_window;
    std::vector<float> m_re;
    std::vector<float> m_im;
    std::vector<int> m_bandBins;
    QVector<float> m_bands;
    QVector<float> m_peaks;
};
