/***********************************************************
 * Test interface -- See header file for more information. *
 ***********************************************************/

#include <exception>
#include <iostream>
#include <list>
#include <utility>

#include "Test.h"

//------------------------------------------------------------------------------

struct Test::Registry
{
	using Mark = std::pair<const char *, int>;
	std::list<Test *> tests;
	std::list<Mark> marks;

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
		bool ret = false;
		try
		{
			ret = test->body();
			cout << (ret ? "Ok." : "FAILED!") << endl;
		}
		catch (std::exception &e)
			{ cout << "FAILED!" << endl << '\t' << e.what() << endl; }
		catch (...)
			{ cout << "FAILED!\n\tAn undetermined exception occurred!" << endl; }
		success &= ret;
	}

	if (Registry::getInstance().marks.size() > 0)
	{
		cout << endl << "Marked lines:" << endl;
		for (Registry::Mark &mark : Registry::getInstance().marks)
			cout << mark.first << ':' << mark.second << endl;
	}

	return success;
}

bool Test::mark(const char *file, int line)
{
	Registry::getInstance().marks.push_back(std::make_pair(file, line));
	return false;
}

//..............................................................................
