#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

struct connection {
    SSL_CTX *ssl_ctx;
    SSL *ssl;
    BIO *ssl_in;
    BIO *ssl_out;
    int sockfd;
    int failed;
    int write_alerts;
} *conn;

void init_conn() {
    SSL_CTX *ctx;
    SSL *ssl;
    long options;

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    ctx = SSL_CTX_new(SSLv23_client_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    ssl = SSL_new(ctx);
    if (!ssl) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        exit(EXIT_FAILURE);
    }

    conn->ssl_ctx = ctx;
    conn->ssl = ssl;

    SSL_set_app_data(conn->ssl, conn);
    SSL_set_msg_callback_arg(conn->ssl, conn);
    options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_SINGLE_DH_USE;
    SSL_set_options(conn->ssl, options);

    conn->ssl_in = BIO_new(BIO_s_mem());
    conn->ssl_out = BIO_new(BIO_s_mem());
    SSL_set_bio(conn->ssl, conn->ssl_in, conn->ssl_out);
}

void send_msg(uint8_t *buf, int len) {
    send(conn->sockfd, buf, len, 0);
}

int recvbuf_msg(uint8_t *buf, int bufsize) {
    int ret = 0;
    ret = recv(conn->sockfd, buf, bufsize, 0);
    return ret;
}

int connection_handshake() {
    char tmpbuf[10240];
    char bufin[10240];
    int res = 0;
    SSL_set_connect_state(conn->ssl);
    while (1) {
        res = SSL_connect(conn->ssl);
        if (res != 1) {
            int err = SSL_get_error(conn->ssl, res);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                res = BIO_ctrl_pending(conn->ssl_out);
                if (res > 0) {
                    res = BIO_read(conn->ssl_out, tmpbuf, sizeof(tmpbuf));
                    send_msg((uint8_t *)tmpbuf, res);
                }
                res = recvbuf_msg((uint8_t *)bufin, sizeof(bufin));
                if (res > 0) {
                    BIO_write(conn->ssl_in, bufin, res);
                }
            } else {
                conn->failed++;
                fprintf(stderr, "OpenSSL: Could not generate ClientHello\n");
                conn->write_alerts++;
                return -1;
            }
        }

        if (SSL_is_init_finished(conn->ssl)) {
            printf("SSL handshake finished\n");
            return 1;
        }
    }
    return 0;
}

void send_https_get_request() {
    const char *request = "GET / HTTP/1.1\r\nHost: www.baidu.com\r\nConnection: close\r\n\r\n";
    int request_len = strlen(request);
    char tmpbuf[10240];
    char bufin[10240];
    int res;

    // Send the HTTP GET request
    SSL_write(conn->ssl, request, request_len);

    // Read the SSL data from the ssl_out BIO and send it to the socket
    res = BIO_ctrl_pending(conn->ssl_out);
    if (res > 0) {
        res = BIO_read(conn->ssl_out, tmpbuf, sizeof(tmpbuf));
        send_msg((uint8_t *)tmpbuf, res);
    }

    // Receive the server response and feed it to ssl_in BIO
    while ((res = recvbuf_msg((uint8_t *)bufin, sizeof(bufin))) > 0) {
        BIO_write(conn->ssl_in, bufin, res);

        // Read decrypted data from SSL object
        while ((res = SSL_read(conn->ssl, tmpbuf, sizeof(tmpbuf) - 1)) > 0) {
            tmpbuf[res] = '\0';
            printf("%s", tmpbuf);
        }

        if (res < 0) {
            int err = SSL_get_error(conn->ssl, res);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                continue;
            } else {
                break;
            }
        }
    }
}

int main() {
    struct sockaddr_in server_addr;
    conn = (struct connection *)malloc(sizeof(struct connection));

    init_conn();

    conn->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn->sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(443);
    inet_pton(AF_INET, "183.2.172.177", &server_addr.sin_addr); // www.baidu.com

    if (connect(conn->sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(conn->sockfd);
        exit(EXIT_FAILURE);
    }

    if (connection_handshake() == 1) {
        printf("Handshake successful\n");
        send_https_get_request();
    } else {
        printf("Handshake failed\n");
    }

    close(conn->sockfd);
    SSL_free(conn->ssl);
    SSL_CTX_free(conn->ssl_ctx);
    free(conn);
    return 0;
}