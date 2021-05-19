#pragma once

#include "cold/async/task.h"

#include <string>
#include <string_view>
#include <uv.h>

std::string makePipeFilePath(std::string_view host, std::string_view service);

cold::async::Task<sockaddr> resolveSockAddr(uv_loop_t* loop, const char* host, const char* service, int protocol = IPPROTO_TCP);

