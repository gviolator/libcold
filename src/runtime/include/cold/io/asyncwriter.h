#pragma once
#include <cold/async/task.h>
#include <cold/com/ianything.h>
#include <cold/memory/bytesbuffer.h>


namespace cold::io {

/// <summary>
///
/// </summary>
struct INTERFACE_API AsyncWriter : virtual IAnything
{
	virtual async::Task<> write(BufferView) = 0;
};

}
