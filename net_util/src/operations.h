#ifndef __LCY_NET_UTIL_OPERATIONS_H__
#define __LCY_NET_UTIL_OPERATIONS_H__

#include <functional>

namespace lcy {
namespace asio {
	class DynamicBuffer;
}	// namespace asio
namespace net_util {

class TCPConnection;

typedef
std::function<
	void
	(
		TCPConnection& conn
	)
> new_conn_op_type;


typedef 
std::functional<
	void
	(
		TCPConnection& conn,
		lcy::asio::dynamic_buffer& read_buf
	)
> message_op_type;


typedef 
std::functional<
	void 
	(
		TCPConnection& conn
	)
> send_finished_op_type;


typedef
std::function<
	void
	(
		TCPConnection& conn
	)
> peer_close_op_type;


typedef
std::function<
	void
	(
		TCPConnection& conn,
		lcy::asio::errcode_type ec
	)
> error_op_type;

}	// namespace net_util
}	// namespace lcy

#endif // __LCY_NET_UTIL_OPERATIONS_H__
