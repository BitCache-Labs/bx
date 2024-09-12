#pragma once

class NoCopy
{
protected:
	NoCopy() = default;
	~NoCopy() = default;

private:
	NoCopy(const NoCopy&) = delete;
	NoCopy& operator=(const NoCopy&) = delete;
};