#pragma once

namespace Writer
{
	enum VideoCodecType
	{
		NONE = 0,
		MJPEG,
		MPEG4,
		H264,
		H265
	};

	enum AudioCodecType
	{
		MUTE = 0,
		PCM,
		ALAW,
		MULAW,
		G726,
		AAC
	};

	struct VideoParameters
	{
		VideoCodecType codecType;

		unsigned int Witdh;
		unsigned int Height;

		unsigned int BitRate;
		unsigned int BitsPerPixel;
		unsigned int Fps;
	};

	struct AudioParameters
	{
		AudioCodecType codecType;

		unsigned int Channels;
		unsigned int BitsPerSample;
		unsigned int SamplesPerSec;
	};
}