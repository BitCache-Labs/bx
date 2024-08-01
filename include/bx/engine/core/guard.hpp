#pragma once

#include "bx/engine/containers/string.hpp"

class NoCopy
{
protected:
	NoCopy() = default;
	~NoCopy() = default;

private:
	NoCopy(const NoCopy&) = delete;
	NoCopy& operator=(const NoCopy&) = delete;
};

class Exception
{
public:
    Exception(const String& message)
        : message(message) {}

    inline const String& Message() const { return message; }

private:
    std::string message;
};