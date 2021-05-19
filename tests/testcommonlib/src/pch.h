#pragma once
#include <SDKDDKVer.h>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <future>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <string>
#include <typeindex>
#include <tuple>
#include <thread>
#include <variant>
#include <random>
#include <sstream>
#include <type_traits>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/gtest-param-test.h>
#include <uv.h>

#include <cold/compiler/coroutine.h>
#include <cold/diagnostics/logging.h>
