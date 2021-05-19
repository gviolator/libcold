#include "pch.h"
#include "cold/utils/cancellationtoken.h"
#include "cold/diagnostics/runtimecheck.h"
#include "cold/com/comclass.h"

using namespace std::chrono;

namespace cold {

namespace {

inline bool timeover(const system_clock::time_point& timePoint, const std::optional<milliseconds>& timeout)
{
	return timeout && (timeout->count() == 0 || *timeout <= (system_clock::now() - timePoint));
}

}

//-----------------------------------------------------------------------------
class CancellationTokenState final : public CancellationToken, public CancellationTokenSource
{
	COMCLASS_(CancellationToken, CancellationTokenSource)

public:

	~CancellationTokenState() = default;

	bool isCancelled() const override
	{
		return m_isCancelled.load();
	}

	Cancellation token() override
	{
		return com::Acquire{static_cast<CancellationToken*>(this)};
	}

	void cancel() override
	{
		m_isCancelled.exchange(true);
	}

	std::atomic_bool m_isCancelled = false;
};

//-----------------------------------------------------------------------------
class CancellationTokenSourceImpl final : public CancellationTokenSource
{
	DECLARE_CLASS_BASE(CancellationTokenSource)

public:

	CancellationTokenSourceImpl(): m_token(com::createInstance<CancellationTokenState>())
	{}

	~CancellationTokenSourceImpl()
	{
		this->cancel();
	}

	bool isCancelled() const override
	{
		return m_token->isCancelled();
	}

	Cancellation token() override
	{
		return m_token;
	}

	void cancel() override
	{
		m_token->cancel();
	}


	const ComPtr<CancellationTokenState> m_token;
};

//-----------------------------------------------------------------------------
Expiration::Expiration() = default;

Expiration::Expiration(Cancellation cancellation, std::chrono::milliseconds timeout)
	: m_cancellation(std::move(cancellation))
	, m_timeout(timeout)
	, m_timePoint(system_clock::now())
{}

Expiration::Expiration(Cancellation cancellation)
	: m_cancellation(std::move(cancellation))
	, m_timeout(std::nullopt)
	, m_timePoint(system_clock::now())
{}

Expiration::Expiration(std::chrono::milliseconds timeout): Expiration({}, timeout)
{}

bool Expiration::isExpired() const
{
	return (m_cancellation && m_cancellation->isCancelled()) || timeover(m_timePoint, m_timeout);
}

Expiration::operator Cancellation () const
{
	return m_cancellation;
}

//-----------------------------------------------------------------------------
std::unique_ptr<CancellationTokenSource> CancellationTokenSource::create()
{
	return std::make_unique<CancellationTokenSourceImpl>();
}


}
