#pragma  once
#include <MMSystem.h>
#include <string.h>


namespace Writer
{
	const unsigned int fourccMJPEG = MAKEFOURCC('M', 'J', 'P', 'G');
	const unsigned int fourccMPEG4 = MAKEFOURCC('M', 'P', '4', 'V');  //MAKEFOURCC('X', 'V', 'I', 'D');
	const unsigned int fourccH264 = MAKEFOURCC('H', '2', '6', '4');   //MAKEFOURCC('A', 'V', 'C', '1');


	inline unsigned convertCodecType(const char * codec)
	{
		if (0 == ::strcmp(codec, "MJPG"))
			return fourccMJPEG;
	
		if (0 == ::strcmp(codec, "MPEG4"))
			return fourccMPEG4;
	
		if (0 == ::strcmp(codec, "H.264"))
			return fourccH264;

		return 0;
	}
}