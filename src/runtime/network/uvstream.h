#pragma once
#include "cold/runtime/internal/uvutils.h"
#include "cold/network/stream.h"
#include "cold/network/socketcontrol.h"
#include "cold/async/task.h"
#include "transport.h"

#include <uv.h>
#include <list>

namespace cold::network {

class ABSTRACT_TYPE UvStream : public virtual Stream, public SocketControl
{
	DECLARE_CLASS_BASE(Stream, SocketControl)

public:

	static Stream::Ptr create(UvStreamHandle&& stream);

	async::Task<> write(BufferView) override;

	async::Task<BytesBuffer> read() override;

	void dispose() override;

	bool isDisposed() const;

	size_t setInboundBufferSize(size_t size) override;

protected:

	UvStream();

	UvStream(UvStreamHandle&& stream);

	~UvStream();

	void setStream(UvStreamHandle&& stream);

	UvStreamHandle& stream();

private:

	void notifyReadAwaiter();

	void doDispose(bool destructed) noexcept;

	UvStreamHandle m_stream;
	BytesBuffer m_buffer;
	async::TaskSource<> m_readAwaiter = nothing;
	std::atomic_bool m_isDisposed = false;
	size_t m_inboundBufferSize = 0;
};

}

#define THROW_IF_DISPOSED(this_) \
	if ((this_).isDisposed())\
	{\
		throw Excpt_("Object is closed"); \
	}\

