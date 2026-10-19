#include <iostream>

#include "lcy/object_pool.hpp"

int main()
{
	lcy::object_pool<int> int_pool;

	int* ob1 = int_pool.alloc(10);
	std::cout << *ob1 << std::endl;

	int* ob2 = int_pool.alloc(20);
	std::cout << *ob2 << std::endl;

	int_pool.free(ob1);
	int_pool.free(ob2);
	
	return 0;
}
