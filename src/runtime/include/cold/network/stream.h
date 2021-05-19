#pragma once
#include <cold/com/comptr.h>
#include <cold/io/asyncreader.h>
#include <cold/io/asyncwriter.h>
#include <cold/utils/disposable.h>


namespace cold::network {

struct INTERFACE_API Stream : IRefCounted, io::AsyncWriter, io::AsyncReader, Disposable
{
	DECLARE_CLASS_BASE(io::AsyncWriter, io::AsyncReader, Disposable)

	using Ptr = ComPtr<Stream>;
};

}
