#include "audioanalyzer.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;

void fft(std::vector<float>& re, std::vector<float>& im)
{
    const int n = static_cast<int>(re.size());
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j) {
            std::swap(re[i], re[j]);
            std::swap(im[i], im[j]);
        }
    }

    for (int len = 2; len <= n; len <<= 1) {
        const float angle = -2.0f * kPi / static_cast<float>(len);
        const float wLenRe = std::cos(angle);
        const float wLenIm = std::sin(angle);
        for (int i = 0; i < n; i += len) {
            float wRe = 1.0f;
            float wIm = 0.0f;
            const int half = len / 2;
            for (int k = 0; k < half; ++k) {
                const int a = i + k;
                const int b = a + half;
                const float vRe = re[b] * wRe - im[b] * wIm;
                const float vIm = re[b] * wIm + im[b] * wRe;
                re[b] = re[a] - vRe;
                im[b] = im[a] - vIm;
                re[a] += vRe;
                im[a] += vIm;

                const float nextRe = wRe * wLenRe - wIm * wLenIm;
                wIm = wRe * wLenIm + wIm * wLenRe;
                wRe = nextRe;
            }
        }
    }
}
} // namespace

AudioAnalyzer::AudioAnalyzer()
{
    m_ring.assign(m_fftSize, 0.0f);
    m_window.resize(m_fftSize);
    m_re.resize(m_fftSize);
    m_im.resize(m_fftSize);
    m_bands.fill(0.0f, BandCount);
    m_peaks.fill(0.08f, BandCount);
    for (int i = 0; i < m_fftSize; ++i) {
        m_window[i] = 0.5f * (1.0f - std::cos(2.0f * kPi * i / (m_fftSize - 1)));
    }
    rebuildBands();
}

void AudioAnalyzer::setSampleRate(int sampleRate)
{
    if (sampleRate <= 0 || sampleRate == m_sampleRate)
        return;
    m_sampleRate = sampleRate;
    reset();
    rebuildBands();
}

void AudioAnalyzer::reset()
{
    std::fill(m_ring.begin(), m_ring.end(), 0.0f);
    m_ringPos = 0;
    m_filled = 0;
    m_bands.fill(0.0f, BandCount);
    m_peaks.fill(0.08f, BandCount);
}

void AudioAnalyzer::rebuildBands()
{
    m_bandBins.clear();
    const float nyquist = static_cast<float>(m_sampleRate) / 2.0f;
    const float low = 30.0f;
    const float high = std::min(18000.0f, nyquist * 0.95f);
    if (high <= low) {
        m_bandBins.resize(BandCount + 1, 1);
        return;
    }

    const float ratio = high / low;
    for (int i = 0; i <= BandCount; ++i) {
        const float freq = low * std::pow(ratio, static_cast<float>(i) / BandCount);
        int bin = static_cast<int>(std::round(freq * m_fftSize / m_sampleRate));
        bin = std::clamp(bin, 1, m_fftSize / 2 - 1);
        m_bandBins.push_back(bin);
    }
}

void AudioAnalyzer::pushSamples(const float* samples, int count)
{
    if (!samples || count <= 0)
        return;

    for (int i = 0; i < count; ++i) {
        m_ring[m_ringPos] = std::clamp(samples[i], -1.0f, 1.0f);
        m_ringPos = (m_ringPos + 1) % m_fftSize;
        if (m_filled < m_fftSize)
            ++m_filled;
    }

    if (m_filled >= m_fftSize)
        analyze();
}

void AudioAnalyzer::analyze()
{
    for (int i = 0; i < m_fftSize; ++i) {
        const int idx = (m_ringPos + i) % m_fftSize;
        m_re[i] = m_ring[idx] * m_window[i];
        m_im[i] = 0.0f;
    }

    fft(m_re, m_im);

    for (int band = 0; band < BandCount; ++band) {
        const int first = m_bandBins[band];
        const int last = std::max(first + 1, m_bandBins[band + 1]);
        float magnitude = 0.0f;
        for (int bin = first; bin < last && bin < m_fftSize / 2; ++bin) {
            const float re = m_re[bin];
            const float im = m_im[bin];
            magnitude = std::max(magnitude, std::sqrt(re * re + im * im));
        }
        magnitude *= 2.0f / static_cast<float>(m_fftSize);

        const float db = 20.0f * std::log10(magnitude + 1e-6f);
        float target = std::clamp((db + 72.0f) / 72.0f, 0.0f, 1.0f);

        m_peaks[band] = std::max(target, m_peaks[band] * 0.995f);
        target = std::clamp(target / std::max(0.12f, m_peaks[band]), 0.0f, 1.0f);

        const float previous = m_bands[band];
        const float factor = target > previous ? 0.55f : 0.095f;
        m_bands[band] = previous + (target - previous) * factor;
        if (m_bands[band] < 0.002f)
            m_bands[band] = 0.0f;
    }
}
