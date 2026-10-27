#pragma once

#include <memory>

class HeadMountedDisplay
{
public:
    enum class Transport
    {
        kUnknown,
        kWired,
        kWireless
    };

    HeadMountedDisplay();
    ~HeadMountedDisplay();

    HeadMountedDisplay(const HeadMountedDisplay&) = delete;
    HeadMountedDisplay& operator=(const HeadMountedDisplay&) = delete;

    Transport GetTransport() const;

    // Converts the OpenVR property result into the public transport state.
    // VisibleForTesting
    static Transport DetermineTransport_(bool propertyReadSucceeded, bool isWireless) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
