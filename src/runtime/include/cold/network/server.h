#pragma once
#include "networkexport.h"
#include "stream.h"
#include <optional>

namespace cold::network {


struct ABSTRACT_TYPE Server: IRefCounted, Disposable
{
	DECLARE_CLASS_BASE(Disposable)

	using Ptr = ComPtr<Server>;

	static NETWORK_EXPORT async::Task<Server::Ptr> listen(std::string address, std::optional<unsigned> = std::nullopt);


	virtual async::Task<Stream::Ptr> accept() = 0;
};

}
