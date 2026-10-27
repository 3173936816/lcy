#ifndef __LCY_ASIO_SSL_CONTEXT_HPP__
#define __LCY_ASIO_SSL_CONTEXT_HPP__

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <string>

#include "lcy/noncopyable.hpp"
#include "lcy/ssl/detail/error.hpp"
#include "lcy/ssl/detail/throw_ssl_exception.hpp"

namespace lcy
{
namespace asio
{
namespace ssl
{

class context :
    private noncopyable
{
public:
    enum method
    {
        tls,
        tls_client,
        tls_server
    };

    enum file_format
    {
        asn1,
        pem
    };

    enum verify_mode
    {
        none = SSL_VERIFY_NONE,
        peer = SSL_VERIFY_PEER,
        fail_if_no_peer_cert = SSL_VERIFY_FAIL_IF_NO_PEER_CERT
    };

    static const uint64_t default_workarounds = SSL_OP_ALL;
    static const uint64_t no_sslv2 = SSL_OP_NO_SSLv2;
    static const uint64_t no_sslv3 = SSL_OP_NO_SSLv3;
    static const uint64_t no_tlsv1 = SSL_OP_NO_TLSv1;
    static const uint64_t no_tlsv1_1 = SSL_OP_NO_TLSv1_1;
    static const uint64_t no_tlsv1_2 = SSL_OP_NO_TLSv1_2;
    static const uint64_t no_tlsv1_3 = SSL_OP_NO_TLSv1_3;
    static const uint64_t no_compression = SSL_OP_NO_COMPRESSION;

public:
    typedef SSL_CTX ssl_ctx_type;

    explicit context(method m = method::tls) :
        ssl_ctx_(0)
    {
        const SSL_METHOD* ssl_method = 0;

        switch ( m ) {
            case method::tls_client : {
                ssl_method = ::TLS_client_method();
                break;
            }
            case method::tls_server : {
                ssl_method = ::TLS_server_method();
                break;
            }
            default : {
                ssl_method = ::TLS_method();
                break;
            }
        }

        ssl_ctx_ = ::SSL_CTX_new(ssl_method);
        if ( !ssl_ctx_ ) {
            lcy::system::error_code ec = detail::make_ssl_error_code();
            detail::throw_ssl_exception(ec, "context::context[method]");
        }

        ::SSL_CTX_set_options(ssl_ctx_,
            no_sslv2 | no_sslv3 | no_tlsv1 | no_tlsv1_1 | no_compression);
    }

    ~context()
    {
        if ( ssl_ctx_ ) {
            ::SSL_CTX_free(ssl_ctx_);
        }
    }

    void use_certificate_file(const std::string& file,
        file_format fmt, lcy::system::error_code& ec)
    {
        if ( fmt == file_format::pem ) {
            int ret = ::SSL_CTX_use_certificate_chain_file(ssl_ctx_, file.c_str());
            if (ret != 1) {
                ec = detail::make_ssl_error_code();
                return;
            }
        } else {
            int ret = ::SSL_CTX_use_certificate_file(ssl_ctx_, file.c_str(), SSL_FILETYPE_ASN1);
            if (ret != 1) {
                ec = detail::make_ssl_error_code();
                return;
            }
        }

        ec = lcy::system::error_code();
    }

    void use_private_key_file(const std::string& file,
        file_format fmt, lcy::system::error_code& ec)
    {
        int type = (fmt == file_format::pem)
            ? SSL_FILETYPE_PEM : SSL_FILETYPE_ASN1;
        int ret = ::SSL_CTX_use_PrivateKey_file(ssl_ctx_, file.c_str(), type);
        if ( ret != 1 ) {
            ec = detail::make_ssl_error_code();
            return;
        }

        if ( ::SSL_CTX_check_private_key(ssl_ctx_) != 1 ) {
            ec = detail::make_ssl_error_code();
            return;
        }

        ec = lcy::system::error_code();
    }

    void load_verify_file(const std::string& file, lcy::system::error_code& ec)
    {
        int ret = ::SSL_CTX_load_verify_locations(ssl_ctx_, file.c_str(), 0);
        if ( ret != 1 ) {
            ec = detail::make_ssl_error_code();
            return;
        }

        ec = lcy::system::error_code();
    }

    void set_verify_mode(int modes)
    {
        ::SSL_CTX_set_verify(ssl_ctx_, modes, 0);
    }

    void set_options(uint64_t opts)
    {
        ::SSL_CTX_set_options(ssl_ctx_, opts);
    }

    ssl_ctx_type* native_handle()
    {
        return ssl_ctx_;
    }

private:
    ssl_ctx_type* ssl_ctx_;
};

}   // namespace ssl
}   // namespace asio
}   // namespace lcy

#endif  // __LCY_ASIO_SSL_CONTEXT_HPP__