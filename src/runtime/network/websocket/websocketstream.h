#pragma once
#include <cold/network/websocketcontrol.h>
#include <cold/network/stream.h>


namespace cold::network {


struct ABSTRACT_TYPE WebSocketStream : Stream, WebSocketControl
{
	DECLARE_CLASS_BASE(Stream, WebSocketControl)


	static ComPtr<WebSocketStream> create(Stream::Ptr rawStream);


	virtual async::Task<> clientToServerHandshake(std::string_view host) = 0;

	virtual async::Task<> serverToClientHandshake() = 0;
};


}
