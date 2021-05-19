#pragma once
#include <cold/memory/bytesbuffer.h>
#include <gtest/gtest.h>
#include <optional>

void fillBufferWithDefaultContent(cold::BytesBuffer& buffer, size_t offset = 0, std::optional<size_t> size = std::nullopt);

cold::BytesBuffer createBufferWithDefaultContent(size_t size);

testing::AssertionResult buffersEqual(const cold::BufferView& buffer1, const cold::BufferView& buffer2);
