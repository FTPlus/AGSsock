/***********************************************************
 * Test interface -- See header file for more information. *
 ***********************************************************/

#include <iostream>
#include <list>

#include "Test.h"

//------------------------------------------------------------------------------

struct Test::Registry
{
	std::list<Test *> tests;

	static Registry &getInstance()
	{
		static Registry *registry = nullptr;
		if (registry == nullptr)
			registry = new Registry();
		return *registry;
	}
};

Test::Test(const char *description, Body body)
	: description(description), body(body)
{
	Registry::getInstance().tests.push_back(this);
}

bool Test::run_tests()
{
	using namespace std;

	bool success = true;
	for (Test *test : Registry::getInstance().tests)
	{
		cout << "Testing: " << test->description << "... " << flush;
		bool ret = test->body();
		cout << (ret ? "Ok." : "FAILED!") << endl;
		success |= ret;
	}
	return success;
}

//..............................................................................
