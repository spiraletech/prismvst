#include "../Source/SurgeryOnyx.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr double kPi = std::numbers::pi;
constexpr int kImpulseLength = 2048;

void requireNear(double actual, double expected, double tolerance, const char* message) {
    if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance)
        throw std::runtime_error(std::string(message) + " actual=" + std::to_string(actual)
                                 + " expected=" + std::to_string(expected));
}

std::complex<double> measuredResponse(const std::vector<double>& impulse, double hz, double sr) {
    std::complex<double> response{0.0, 0.0};
    for (std::size_t n = 0; n < impulse.size(); ++n) {
        const double phase = -2.0 * kPi * hz * static_cast<double>(n) / sr;
        response += impulse[n] * std::complex<double>{std::cos(phase), std::sin(phase)};
    }
    return response;
}

void checkImpulse(double sr, double amount) {
    surgery::Onyx processor;
    processor.prepare(sr);
    const std::array<double, 5> controls{0.0, 0.0, 0.0, 0.0, amount};
    const double a = 1.0 - std::exp(-2.0 * kPi * std::min(3600.0, sr * .16) / sr);
    const double g = .72 * surgery::Onyx::law(amount);
    std::vector<double> impulse(kImpulseLength);
    for (int n = 0; n < kImpulseLength; ++n) {
        const double x = n == 0 ? 1.0 : 0.0;
        impulse[static_cast<std::size_t>(n)] = processor.sample(x, controls);
        const double expected = n == 0 ? 1.0 - g + g * a : g * a * std::pow(1.0 - a, n);
        requireNear(impulse[static_cast<std::size_t>(n)], expected, 3e-13,
                    "QUARTZ impulse mismatch");
    }
    for (double hz : {100.0, 1000.0, 4000.0, 12000.0}) {
        const double phase = -2.0 * kPi * hz / sr;
        const std::complex<double> z{std::cos(phase), std::sin(phase)};
        const auto expected = std::complex<double>{1.0 - g, 0.0} + g * a / (1.0 - (1.0 - a) * z);
        const auto measured = measuredResponse(impulse, hz, sr);
        requireNear(std::abs(measured - expected), 0.0, 2e-12,
                    "QUARTZ measured complex transfer mismatch");
    }
}

void checkNeutralAndWarmSwitch(double sr) {
    surgery::Onyx neutral, switched, alwaysOn, legacy;
    neutral.prepare(sr);
    switched.prepare(sr);
    alwaysOn.prepare(sr);
    legacy.prepare(sr);
    const std::array<double, 5> off{0.0, 0.0, 0.0, 0.0, 0.0};
    const std::array<double, 5> on{0.0, 0.0, 0.0, 0.0, 1.0};
    const std::array<double, 4> oldApi{0.0, 0.0, 0.0, 0.0};
    for (int n = 0; n < 4096; ++n) {
        const double x = .23 * std::sin(2.0 * kPi * 1103.0 * n / sr)
                         + (n % 103 == 0 ? .17 : 0.0);
        requireNear(neutral.sample(x, off), x, 0.0, "QUARTZ zero changed dry output");
        requireNear(legacy.sample(x, oldApi), x, 0.0, "QUARTZ legacy four-knob bypass changed");
        const double warm = switched.sample(x, n < 2048 ? off : on);
        const double continuous = alwaysOn.sample(x, on);
        if (n >= 2048)
            requireNear(warm, continuous, 2e-13,
                        "QUARTZ switch used stale filter state");
    }
    // prepare() must reset filter state, not retain prior audio between sessions.
    switched.prepare(sr);
    surgery::Onyx fresh;
    fresh.prepare(sr);
    requireNear(switched.sample(1.0, on), fresh.sample(1.0, on), 0.0,
                "QUARTZ prepare retained filter history");
}
} // namespace

int main() {
    for (double sr : {44100.0, 48000.0, 96000.0}) {
        for (double amount : {0.2, 0.5, 1.0})
            checkImpulse(sr, amount);
        checkNeutralAndWarmSwitch(sr);
    }
    std::cout << "QUARTZ: impulse/complex transfer/neutral/legacy/warm-switch/reset"
              << " verified at 44.1, 48 and 96 kHz\n";
}
