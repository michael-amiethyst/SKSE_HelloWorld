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

    Transport GetTransport();

    // Converts the OpenVR property result into the public transport state.
    // Kept separate from DLL access so the decision can be tested without OpenVR.
    static Transport DetermineTransport(bool propertyReadSucceeded, bool isWireless) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
