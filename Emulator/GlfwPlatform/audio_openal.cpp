#include "audio_openal.h"

void guru::OpenAlPlayer::Setup()
{
	m_device = alcOpenDevice(0);
	if (m_device)
	{
		m_context = alcCreateContext(m_device, 0);
		alcMakeContextCurrent(m_context);

		for (int i = 0; i < audio::kNumOfStereoSources; i++)
		{
			auto& channel = m_channel[i];
			alGenBuffers(ALsizei(channel.buffers.size()), channel.buffers.data());
		}

		alGenSources(audio::kNumOfStereoSources, m_source.data());
		if (alGetError() == AL_NO_ERROR)
		{
			m_soundEnabled = true;
		}

		float newVolume = 0.1f;
		alSourcef(m_source[0], AL_GAIN, newVolume);
		alSourcef(m_source[1], AL_GAIN, newVolume);
	}
}

void guru::OpenAlPlayer::Shutdown()
{
	if (m_context)
	{
		alDeleteSources(audio::kNumOfStereoSources, m_source.data());

		for (int i = 0; i < audio::kNumOfStereoSources; i++)
		{
			auto& channel = m_channel[i];
			alDeleteBuffers(ALsizei(channel.buffers.size()), channel.buffers.data());
		}

		alcMakeContextCurrent(0);
		alcDestroyContext(m_context);
		m_context = nullptr;
	}

	if (m_device)
	{
		alcCloseDevice(m_device);
		m_device = nullptr;
	}
}

void guru::OpenAlPlayer::AddAudioBuffer(const am::AudioBuffer* buffer)
{
	for (int s = 0; s < audio::kNumOfStereoSources; s++)
	{
		auto& source = m_source[s];
		auto& audioChannel = m_channel[s];

		ALint playing;
		alGetSourcei(source, AL_SOURCE_STATE, &playing);
		if (playing != AL_PLAYING)
		{
			if (audioChannel.playing)
			{
				printf("Audio stalled... rebuffering...\n");
				alSourcei(source, AL_BUFFER, 0); // unqueue all buffers
				audioChannel.playing = false;
			}

			ALint buffersQueued;
			alGetSourcei(source, AL_BUFFERS_QUEUED, &buffersQueued);

			if (buffersQueued < audio::kNumBuffers)
			{
				alBufferData(audioChannel.buffers[buffersQueued], AL_FORMAT_STEREO8, (*buffer)[s].data(), am::kAudioBufferLength * 2, audio::kPlaybackFrequency);
				ALenum error = alGetError();
				if (error != AL_NO_ERROR)
					__debugbreak();

				alSourceQueueBuffers(source, 1, &audioChannel.buffers[buffersQueued]);
				error = alGetError();
				if (error != AL_NO_ERROR)
					__debugbreak();

				buffersQueued++;
			}

			if (buffersQueued == audio::kMinBuffersToStartPlayback)
			{
				// All buffers full - Restart playing.
				alSourcePlay(source);
				audioChannel.playing = true;
			}
		}
		else
		{
			ALint bufferProcessed;
			alGetSourcei(source, AL_BUFFERS_PROCESSED, &bufferProcessed);
			if (bufferProcessed > 0)
			{
				ALuint alBuffer;
				alSourceUnqueueBuffers(source, 1, &alBuffer);
				alBufferData(alBuffer, AL_FORMAT_STEREO8, (*buffer)[s].data(), am::kAudioBufferLength * 2, audio::kPlaybackFrequency);

				alSourceQueueBuffers(source, 1, &alBuffer);
				ALenum error = alGetError();
				if (error != AL_NO_ERROR)
					__debugbreak();
			}
			else
			{
				ALint buffersQueued;
				alGetSourcei(source, AL_BUFFERS_QUEUED, &buffersQueued);


				if (buffersQueued < audio::kNumBuffers)
				{
					alBufferData(audioChannel.buffers[buffersQueued], AL_FORMAT_STEREO8, (*buffer)[s].data(), am::kAudioBufferLength * 2, audio::kPlaybackFrequency);
					alSourceQueueBuffers(source, 1, &audioChannel.buffers[buffersQueued]);
				}
				else
				{
					// No buffers have finished processing!
					printf("Buffer full - audio dropped!\n");
				}
			}
		}
	}
}