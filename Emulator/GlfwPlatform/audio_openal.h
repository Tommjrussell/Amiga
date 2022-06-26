#pragma once

#include "amiga/audio.h"

#include "al.h"
#include "alc.h"

#include <array>

namespace guru
{
	namespace audio
	{
		constexpr int kNumOfStereoSources = 2;
		constexpr int kNumBuffers = 8;
		constexpr int kMinBuffersToStartPlayback = 6;
		constexpr int kPlaybackFrequency = 35469;
	}

	class OpenAlPlayer : public am::AudioPlayer
	{
	public:
		void Setup();
		void Shutdown();
		virtual void AddAudioBuffer(const am::AudioBuffer* buffer) override;

	private:

		struct AudioChannel
		{
			std::array<ALuint, audio::kNumBuffers> buffers;
			bool playing = false;
		};
		ALCdevice* m_device = nullptr;
		ALCcontext* m_context = nullptr;

		std::array<ALuint, 2> m_source;
		std::array<AudioChannel, 2> m_channel;

		bool m_soundEnabled = false;
	};

}