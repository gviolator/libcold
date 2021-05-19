#pragma once
#include <cold/runtime/runtimeexport.h>
#include <cold/com/comptr.h>

#include <chrono>
#include <optional>

namespace cold {


struct ABSTRACT_TYPE CancellationToken : IRefCounted
{
	using Ptr = ComPtr<CancellationToken>;

	virtual bool isCancelled() const = 0;
};

using Cancellation = CancellationToken::Ptr;


struct ABSTRACT_TYPE CancellationTokenSource
{
	using Ptr = std::unique_ptr<CancellationTokenSource>;

	RUNTIME_EXPORT static CancellationTokenSource::Ptr create();


	virtual ~CancellationTokenSource() = default;

	virtual bool isCancelled() const = 0;

	virtual Cancellation token() = 0;

	virtual void cancel() = 0;
};


class RUNTIME_EXPORT Expiration final
{
public:

	Expiration();

	Expiration(Cancellation cancellation, std::chrono::milliseconds timeout);

	Expiration(Cancellation cancellation);

	Expiration(std::chrono::milliseconds timeout);

	bool isExpired() const;

	operator Cancellation () const;

	inline static Expiration never()
	{
		return {};
	}

private:

	Cancellation m_cancellation;
	std::optional<std::chrono::milliseconds> m_timeout;
	const std::chrono::system_clock::time_point m_timePoint;
};

}
