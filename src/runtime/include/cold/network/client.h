#pragma once
#include <cold/network/networkexport.h>
#include <cold/network/stream.h>
#include <cold/utils/cancellationtoken.h>

#include <string>


namespace cold::network {


struct Client
{
	static NETWORK_EXPORT async::Task<Stream::Ptr> connect(std::string address, Expiration = Expiration::never());
};

}
