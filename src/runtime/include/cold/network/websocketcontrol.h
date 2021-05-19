#pragma once
#include <cold/network/socketcontrol.h>
#include <cold/com/comptr.h>

namespace cold::network {

/// <summary>
///
/// </summary>
enum class WebSocketMode
{
	Text,
	Binary
};

/// <summary>
///
/// </summary>
struct ABSTRACT_TYPE WebSocketControl : SocketControl
{
	DECLARE_CLASS_BASE(SocketControl)


	virtual void setWebSocketMode(WebSocketMode mode) = 0;

	virtual WebSocketMode webSocketMode() const = 0;

	virtual void setMasked(bool) = 0;

	virtual bool isMasked() const = 0;

	virtual ComPtr<struct Stream> rawStream() const = 0;

	virtual void setWriteStreamFrameSize(size_t streamFrameSize) = 0;

	virtual size_t writeStreamFrameSize() const = 0;
};


}
