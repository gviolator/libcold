#pragma once
#include <cold/com/ianything.h>

namespace cold::io {

/// <summary>
///
/// </summary>
struct INTERFACE_API Writer : virtual IAnything
{
	virtual void write(const std::byte*, size_t size) = 0;

	virtual size_t offset() const = 0;
};


}
