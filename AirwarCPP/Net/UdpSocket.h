#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <uv.h>
#include <vector>
#include <cstdint>
#include <cstring>

// Portable ntohs (avoids linking ws2_32.lib on Windows)
inline uint16_t portable_ntohs(uint16_t v) {
    auto* p = (const uint8_t*)&v;
    return (uint16_t)p[0] << 8 | p[1];
}
inline uint16_t portable_htons(uint16_t v) {
    auto* p = (const uint8_t*)&v;
    return (uint16_t)p[1] << 8 | p[0];
}
#include <string>
#include <functional>
#include <stdexcept>

// RAII wrapper around libuv UDP handle
class UdpSocket {
    uv_loop_t* loop_ = nullptr;
    uv_udp_t handle_;
    bool initialized_ = false;
    bool bound_ = false;

public:
    using RecvCallback = std::function<void(const std::string& data,
                                            const sockaddr_storage& addr)>;

    UdpSocket() { handle_.data = this; }
    ~UdpSocket() { close(); }

    bool init(uv_loop_t* loop) {
        loop_ = loop;
        int r = uv_udp_init(loop, &handle_);
        if (r != 0) { fprintf(stderr, "uv_udp_init error: %s\n", uv_strerror(r)); return false; }
        initialized_ = true;
        return true;
    }

    // Bind with IPv6 dual-stack, fallback to IPv4
    bool bind(int port) {
        if (!initialized_) return false;

        // Try IPv6 dual-stack first
        struct sockaddr_in6 addr6;
        int r = uv_ip6_addr("::", port, &addr6);
        if (r == 0) {
            uv_udp_bind(&handle_, (const struct sockaddr*)&addr6, 0);  // 0 = IPV6_V6ONLY=0
            bound_ = true;
            return true;
        }

        // Fallback to IPv4
        struct sockaddr_in addr4;
        r = uv_ip4_addr("0.0.0.0", port, &addr4);
        if (r == 0) {
            uv_udp_bind(&handle_, (const struct sockaddr*)&addr4, 0);
            bound_ = true;
            return true;
        }
        return false;
    }

    bool connect(const std::string& ip, int port) {
        if (!initialized_) return false;
        // For UDP client, we use sendTo with the server address
        return true;  // UDP is connectionless; we'll store the address separately
    }

    bool startRecv(RecvCallback cb) {
        if (!bound_) return false;
        recvCb_ = std::move(cb);
        int r = uv_udp_recv_start(&handle_, allocCb, recvCbStatic);
        if (r != 0) { fprintf(stderr, "uv_udp_recv_start error: %s\n", uv_strerror(r)); return false; }
        return true;
    }

    void stopRecv() {
        uv_udp_recv_stop(&handle_);
    }

    bool sendTo(const std::string& data, const sockaddr_storage& addr) {
        return sendTo(data.data(), data.size(), (const struct sockaddr*)&addr);
    }

    bool sendTo(const std::string& data, const std::string& ip, int port) {
        struct sockaddr_storage addr;
        if (makeAddr(ip, port, addr)) return sendTo(data, addr);
        return false;
    }

    bool sendTo(const char* buf, size_t len, const struct sockaddr* addr) {
        if (!initialized_) return false;
        struct SendReq {
            uv_udp_send_t req;
            uv_buf_t buf;
            std::vector<char> data;
        };
        auto* sr = new SendReq;
        sr->data.assign(buf, buf + len);
        sr->buf.base = sr->data.data();
        sr->buf.len = (ULONG)sr->data.size();
        sr->req.data = sr;

        int r = uv_udp_send(&sr->req, &handle_, &sr->buf, 1, addr, [](uv_udp_send_t* r, int) {
            delete (SendReq*)r->data;
        });
        if (r != 0) { delete sr; return false; }
        return true;
    }

    void close() {
        if (initialized_) {
            uv_udp_recv_stop(&handle_);
            uv_close((uv_handle_t*)&handle_, [](uv_handle_t*){});
            initialized_ = false;
            bound_ = false;
        }
    }

    uv_udp_t* handle() { return &handle_; }
    bool isBound() const { return bound_; }

    static bool makeAddr(const std::string& ip, int port, sockaddr_storage& out) {
        struct sockaddr_in6 addr6;
        if (uv_ip6_addr(ip.c_str(), port, &addr6) == 0) {
            memcpy(&out, &addr6, sizeof(addr6)); return true;
        }
        struct sockaddr_in addr4;
        if (uv_ip4_addr(ip.c_str(), port, &addr4) == 0) {
            memcpy(&out, &addr4, sizeof(addr4)); return true;
        }
        return false;
    }

    static void addrToString(const sockaddr_storage& addr, std::string& ip, int& port) {
        char ipStr[64];
        if (addr.ss_family == AF_INET6) {
            uv_ip6_name((const struct sockaddr_in6*)&addr, ipStr, sizeof(ipStr));
            ip = ipStr;
            port = portable_ntohs(((const struct sockaddr_in6*)&addr)->sin6_port);
        } else {
            uv_ip4_name((const struct sockaddr_in*)&addr, ipStr, sizeof(ipStr));
            ip = ipStr;
            port = portable_ntohs(((const struct sockaddr_in*)&addr)->sin_port);
        }
    }

private:
    RecvCallback recvCb_;

    static void allocCb(uv_handle_t*, size_t suggested, uv_buf_t* buf) {
        buf->base = new char[suggested];
        buf->len = (ULONG)suggested;
    }

    static void recvCbStatic(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf,
                              const struct sockaddr* addr, unsigned flags) {
        auto* self = (UdpSocket*)handle->data;
        if (nread > 0 && addr && self->recvCb_) {
            sockaddr_storage storage;
            memcpy(&storage, addr, sizeof(storage));
            self->recvCb_(std::string(buf->base, nread), storage);
        }
        delete[] buf->base;
    }
};
