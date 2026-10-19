#include <iostream>

#include "lcy/system/error.hpp"

/********************************************
* Test error_code print message
********************************************/
void test_print()
{
#define XX(xx_errnum, xx_system_errnum) 									\
		{																	\
			lcy::system::error_code ec(lcy::system::error::xx_errnum);		\
			std::cout << #xx_errnum	<< " : " << ec.message() << std::endl;	\
		}
	LCY_SYSTEM_ERROR_NUMBER_MAP(XX)
#undef XX
}


/********************************************
* Test custom category print message
********************************************/
#define CUSTOM_ERROR_MAP(XX)	\
	XX(success, 0, Success)		\
	XX(failure, 1, Failure)		\
	XX(eof, 	3, End of file)	\

enum custom_error
{
#define XX(xx_errnum, xx_custom_errnum, xx_msg)	\
		xx_errnum = xx_custom_errnum,
	CUSTOM_ERROR_MAP(XX)
#undef XX
};

class custom_category :
	public lcy::system::error_category
{
private:
	friend const custom_category& fetch_custom_category();

public:
	const char* name() const override
	{
		return "custom";
	}

	const char* message(int errnum) const override
	{
	#define XX(xx_errnum, xx_custom_errnum, xx_msg)		\
			if ( errnum == xx_custom_errnum ) {			\
				return #xx_msg;							\
			}
		CUSTOM_ERROR_MAP(XX)
	#undef XX
		return "Unknown";
	}
};

inline const custom_category& fetch_custom_category()
{
	static custom_category m;
	return m;
}

void test_custom_print()
{
#define XX(xx_errnum, xx_system_errnum, xx_msg) 									\
		{																			\
			lcy::system::error_code ec(												\
				custom_error::xx_errnum, fetch_custom_category());					\
			std::cout << #xx_errnum	<< " : " << ec.message() << std::endl;			\
		}
	CUSTOM_ERROR_MAP(XX)
#undef XX
}

int main()
{
	// test_print();

	test_custom_print();

	return 0;
}
