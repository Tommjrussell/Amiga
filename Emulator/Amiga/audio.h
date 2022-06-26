#pragma once

#include <array>
#include <vector>
#include <stdint.h>

namespace am
{
	constexpr int kAudioBufferLength = 1024;

	using AudioBuffer = std::array<std::vector<uint8_t>, 2>;

	class AudioPlayer
	{
	public:
		virtual void AddAudioBuffer(const AudioBuffer* buffer) = 0;
	};
}