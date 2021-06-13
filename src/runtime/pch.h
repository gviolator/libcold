#pragma once

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#endif

#include <atomic>
#include <chrono>
#include <cmath>
#include <forward_list>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>
#include <thread>
#include <variant>
#include <vector>

#include <uv.h>
