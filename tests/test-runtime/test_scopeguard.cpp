#include "pch.h"
#include <cold/utils/scopeguard.h>


TEST(Common_ScopeGuard, ScopeLeave) {

	bool leave11 = false;
	bool leave21 = false;
	bool leave22 = false;

	SCOPE_Leave {
		leave11 = true;
	};

	ASSERT_FALSE(leave11);

	{
		SCOPE_Leave { leave21 = true; };
		SCOPE_Leave { leave22 = true; };

		ASSERT_FALSE(leave21);
		ASSERT_FALSE(leave22);
	}

	ASSERT_TRUE(leave21);
	ASSERT_TRUE(leave22);
}


TEST(Common_ScopeGuard, ScopeFailure) {

	bool leave = false;
	bool failure = false;
	bool success = false;
	bool neverBeHere = false;


	const auto throwInScope = [&]
	{
		SCOPE_Leave { leave = true; };
		SCOPE_Fail { failure = true; };
		SCOPE_Success { success = true; };

		throw std::exception("out of scope");

		neverBeHere = true;
	};

	try {
		throw std::exception("XL");
	}
	catch(std::exception )
	{
		std::cout << "CATCH\n";
	}


	try
	{
		throwInScope();
	}
	catch(std::exception)
	{
		std::cout << "catch" << std::endl;
	}


	ASSERT_TRUE(leave);
	ASSERT_TRUE(failure);
	ASSERT_FALSE(success);
	ASSERT_FALSE(neverBeHere);
}


TEST(Common_ScopeGuard, ScopeSuccess) {

	bool leave = false;
	bool failure = false;
	bool success = false;

	{
		SCOPE_Leave { leave = true; };
		SCOPE_Fail { failure = true; };
		SCOPE_Success { success = true; };
	}

	ASSERT_TRUE(leave);
	ASSERT_FALSE(failure);
	ASSERT_TRUE(success);
}


TEST(Common_ScopeGuard, ScopeNestedException) {




}
