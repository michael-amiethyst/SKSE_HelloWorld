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

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
