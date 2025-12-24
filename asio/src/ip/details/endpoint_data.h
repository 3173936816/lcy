#ifndef __LCY_ASIO_IP_DETAILS_ENDPOINT_DATA_H__
#define __LCY_ASIO_IP_DETAILS_ENDPOINT_DATA_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

namespace lcy {
namespace asio {
namespace ip {
namespace details {

union data_type {
	struct sockaddr_in addrin_;
	struct sockaddr_in6 addrin6_;
};

}	// namespace details
}	// namespace ip
}	// namespace asio
}	// namespace lcy

#endif // __LCY_ASIO_IP_DETAILS_ENDPOINT_DATA_H__
