#pragma once

//#include "bytesbuffer.h"
//#include "async/tasks.h"
#include <cold/com/ianything.h>

namespace cold::io {

struct ABSTRACT_TYPE Reader : virtual IAnything{
	virtual size_t read(std::byte* buffer, size_t readCount) = 0;
};

/// <summary>
///
// /// </summary>
// struct INTERFACE_API AsyncReader : virtual com::IAnything
// {
// 	virtual async::Task<BytesBuffer> read() = 0;
// };


}

