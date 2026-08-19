#pragma once

#include <memory>

class OpenVRTransportDetector
{
public:
    enum class Transport
    {
        kUnknown,
        kWired,
        kWireless
    };

    OpenVRTransportDetector();
    ~OpenVRTransportDetector();

    OpenVRTransportDetector(const OpenVRTransportDetector&) = delete;
    OpenVRTransportDetector& operator=(const OpenVRTransportDetector&) = delete;

    Transport GetTransport() const;

    // Converts the OpenVR property result into the public transport state.
    // VisibleForTesting
    static Transport DetermineTransport_(bool propertyReadSucceeded, bool isWireless) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
