#include "pch.h"
#include <cold/utils/typeutility.h>

using namespace cold;

TEST(Common_Utils, TemplateOf) {

	static_assert(IsTemplateOf<std::shared_ptr, std::shared_ptr<std::string>>);
	static_assert(IsTemplateOf<std::optional, std::optional<std::string>>);
	static_assert(IsTemplateOf<std::basic_string, std::wstring>);

	static_assert(!IsTemplateOf<std::basic_string, int>);
	static_assert(!IsTemplateOf<std::vector, std::list<std::string>>);
	static_assert(IsTemplateOf<std::vector, std::vector<float>>);

	static_assert(IsTemplateOf<std::tuple, std::tuple<float, int>>);
	static_assert(!IsTemplateOf<std::tuple, float>);
}


