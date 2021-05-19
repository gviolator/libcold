#pragma once
#include <cold/com/ianything.h>

namespace cold::network {

struct ABSTRACT_TYPE SocketControl
{
	virtual size_t setInboundBufferSize(size_t size) = 0;
};

}
