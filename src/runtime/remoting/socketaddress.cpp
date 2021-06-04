#include "pch.h"
#include "cold/remoting/socketaddress.h"
#include "cold/diagnostics/exception.h"
#include "cold/utils/strings.h"
#include "cold/utils/stringconv.h"

#include <regex>

using namespace cold::strings;

namespace cold::remoting {

namespace {

constexpr auto DefaultMatchOptions = std::regex_constants::ECMAScript | std::regex_constants::icase;
}

std::tuple<std::string_view, std::string_view> SocketAddress::parseAddressString(std::string_view address)
{
	std::match_results<std::string_view::iterator> match;

	if (!std::regex_match(address.begin(), address.end(), match, std::regex {"^([A-Za-z0-9\\+_-]+)://(.+)$", DefaultMatchOptions}))
	{
		throw Excpt_("Invalid address: [%1]", address);
	}

	DEBUG_CHECK(match.size() == 3)

	std::string_view protocol(match[1].first, match[1].second);
	std::string_view addressString(match[2].first, match[2].second);

	return {protocol, addressString};
}

std::tuple<std::string_view, std::string_view> SocketAddress::parseIpcAddress(std::string_view address)
{
	std::string_view host;
	std::string_view service;

	std::match_results<std::string_view::iterator> match;

	if (std::regex_match(address.begin(), address.end(), match, std::regex{"^([A-Za-z0-9_\\.\\-]+)\\\\([^\\\\]+)$", DefaultMatchOptions}))
	{
		decltype(auto) match1 = match[1];
		decltype(auto) match2 = match[2];

		host = std::string_view(match1.first, match1.second);
		if (host == ".")
		{
			host = "";
		}

		service = std::string_view(match2.first, match2.second);
	}
	else
	{
		throw Excpt_("Invalid ipc address:{0}", address);
	}

	return std::tuple{std::move(host), std::move(service)};
}

std::tuple<std::string, std::string> SocketAddress::parseTcpAddress(std::string_view address)
{
	std::match_results<std::string_view::iterator> match;

	if (!(
		std::regex_match(address.begin(), address.end(), match, std::regex{"^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})?:(\\d{1,5})$", DefaultMatchOptions}) ||
		std::regex_match(address.begin(), address.end(), match, std::regex{"^([A-Za-z0-9\\._\\-]+):(\\d{1,5})$", DefaultMatchOptions}))
		)
	{
		throw Excpt_("Invalid tcp address:{0}", address);
	}

	decltype(auto) match1 = match[1];
	decltype(auto) matchn = match[match.size() - 1];

	std::string host(match1.first, match1.second);
	std::string service(matchn.first, matchn.second);

	return std::tuple{std::move(host), std::move(service)};
}

}
