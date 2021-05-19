#pragma once
#include "com/interface.h"
#include "async/task.h"

namespace cold {

struct ABSTRACT_TYPE AsyncDisposable
{
	virtual async::Task<> disposeAsync() = 0;
};

}
