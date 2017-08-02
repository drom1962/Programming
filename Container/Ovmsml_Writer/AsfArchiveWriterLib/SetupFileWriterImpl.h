#pragma once
#include "ArchiveFileWriter.h"

#define WRITER_CLASS_NAME_IMPL AviFileWriter

namespace Writer
{
#ifdef WRITER_CLASS_NAME_IMPL
	class WRITER_CLASS_NAME_IMPL;
	typedef WRITER_CLASS_NAME_IMPL WriterImpl;
#else
	typedef DummyWriter WriterImpl;
#endif
}

#ifdef WRITER_CLASS_NAME_IMPL
#define QUOTEME(M)  #M
#define CONCAT(x,y) x##y
#define INCLUDE_FILE_2(x) QUOTEME(x)
#define INCLUDE_FILE(x) INCLUDE_FILE_2(CONCAT(x, .h))

#include INCLUDE_FILE(WRITER_CLASS_NAME_IMPL)

#undef INCLUDE_FILE
#undef INCLUDE_FILE_2
#undef CONCAT
#undef QUOTEME
#endif //WRITER_CLASS_NAME_IMPL