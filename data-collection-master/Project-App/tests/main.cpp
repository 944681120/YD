#include "gtest/gtest.h"

int main(int argc, char* argv[])
{
	extern void init_zlog();
	init_zlog();
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}