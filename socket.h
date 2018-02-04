#ifndef _SOCKET_H_INCLUDED
#define _SOCKET_H_INCLUDED

#ifdef _WIN32
    #include <winsock.h>
    typedef int socklen_t;
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>

    #define GetLastError()    errno
#endif

#include <cstdint>
#include <system_error>
#include <string>
#include <functional>
#include <cassert>
#include <string.h>

namespace testing {
class SocketException : public std::runtime_error {
public:
    SocketException(const char *msg, const std::error_code& ec)
        : std::runtime_error(msg)
        , ec_(ec) {}

    const std::error_code& error_code() const { return ec_; }
private:
    std::error_code ec_;
};

using ErrNoType = decltype(GetLastError());

inline void 
CheckAndThrowIfERR(const char *msg, const std::error_code& ec) {
    if (ec) {
        throw SocketException(msg, ec);
    }
}

inline void 
CheckAndThrowIfERR(const char *msg, ErrNoType err = GetLastError()) {
    CheckAndThrowIfERR(msg, { static_cast<int>(err), std::system_category() });
}

struct SocketAddress4 : sockaddr_in {
    int af() const {
        return this->sin_family;
    }

    uint16_t port() const {
        return ntohs(this->sin_port);
    }

    void port(uint16_t u16) {
        this->sin_port = htons(u16);
    }

    const char *ip() const {
        return inet_ntoa(this->sin_addr);
    }

    void ip(const char *str) {
        this->sin_addr.s_addr = inet_addr(str);
    }
};

struct SocketAddress : sockaddr {
    SocketAddress() {
        memset(this, 0, sizeof(SocketAddress));
    }

    SocketAddress4 *v4() {
        return reinterpret_cast<SocketAddress4 *>(this);
    }

    const SocketAddress4 *v4() const {
        return reinterpret_cast<const SocketAddress4 *>(this);
    }
};

inline SocketAddress 
MakeAddress4(uint16_t port, const char *str = "127.0.0.1") {
    SocketAddress a;
    a.v4()->sin_family = AF_INET;
    a.v4()->port(port);
    a.v4()->ip(str);

    return a;
}

#ifdef _WIN32
template<int Major = 2, int Minor = 0>
class WinsockInitializer {
public:
    WinsockInitializer() {
        WSADATA wd;
        if (WSAStartup(MAKEWORD(Major, Minor), &wd) < 0) {
            CheckAndThrowIfERR("WSAStartup");
        }

        initialized_ = true;
    }

    ~WinsockInitializer() {
        if (initialized_) {
            WSACleanup();
        }
    }

private:
    bool initialized_ = false;
};
#endif

template<int Level, int Name, typename T = int>
struct SockOpt {
    T val;
};

#ifdef _WIN32
template<int Level, int Name>
struct TimeoutSockOpt : SockOpt<Level, Name, int> {
    explicit TimeoutSockOpt(int sec, int usec = 0) {
        set_timeout(sec, usec);
    }

    void set_timeout(int sec, int usec) {
        this->val = sec * 1000 + usec / 1000;
    }

    int get_timeout() const {
        return this->val;
    }
};
#else
template<int Level, int Name>
struct TimeoutSockOpt : SockOpt<Level, Name, struct timeval> {
    explicit TimeoutSockOpt(int sec, int usec = 0) {
        set_timeout(sec, usec);
    }

    void set_timeout(int sec, int usec) {
        this->val.tv_sec = sec;
        this->val.tv_usec = usec;
    }

    struct timeval get_timeout() const {
        return this->val;
    }
};
#endif

template<int Level, int Name>
struct BoolSockOpt : SockOpt<Level, Name, int> {
    explicit BoolSockOpt(bool on = true) {
        enabled(on);
    }

    void enabled(bool on) {
        this->val = on ? 1 : 0;
    }

    void enabled() const {
        return 0 != this->val;
    }
};

using RcvTimeoutSockOpt = TimeoutSockOpt<SOL_SOCKET, SO_RCVTIMEO>;

using SndTimeoutSockOpt = TimeoutSockOpt<SOL_SOCKET, SO_SNDTIMEO>;

using ReuseAddrSockOpt = BoolSockOpt<SOL_SOCKET, SO_REUSEADDR>;

#ifndef _WIN32
using ReusePortSockOpt = BoolSockOpt<SOL_SOCKET, SO_REUSEPORT>;
#endif

using MutableBuffer = std::pair<char *, size_t>;

using ConstBuffer = std::pair<const char *, size_t>;

template<typename Container, typename = std::void_t<typename Container::value_type>>
constexpr MutableBuffer MakeBuffer(Container& c) {
    return {
        reinterpret_cast<char *>(&c[0]), 
        c.size() * sizeof(typename Container::value_type)
    };
}

template<typename Container, typename = std::void_t<typename Container::value_type>>
constexpr ConstBuffer MakeBuffer(const Container& c) {
    return {
        reinterpret_cast<const char *>(&c[0]),
        c.size() * sizeof(typename Container::value_type)
    };
}

template<typename T, size_t N>
constexpr MutableBuffer MakeBuffer(T (&a)[N]) {
    return {
        reinterpret_cast<char *>(a),
        N * sizeof(T)
    };
}

template<typename T, size_t N>
constexpr ConstBuffer MakeBuffer(const T(&a)[N]) {
    return {
        reinterpret_cast<const char *>(a),
        N * sizeof(T)
    };
}

class Socket {
public:
#ifdef _WIN32
    using RawSocketHandle = SOCKET;
    static constexpr RawSocketHandle kInvalidSocketHandle = INVALID_SOCKET;
#else
    using RawSocketHandle = int;
    static constexpr RawSocketHandle kInvalidSocketHandle = -1;
#endif

    static constexpr int kListenBacklogDefault = 64;

    Socket() = default;
    ~Socket() { Close(); }

    Socket(Socket&& rhs) : h_(rhs.Detach()) {}
    Socket& operator=(Socket&& rhs) noexcept {
        if (this != std::addressof(rhs)) {
            Close();
            h_ = rhs.Detach();
        }
        return *this;
    }

    bool Opened() const {
        return kInvalidSocketHandle != h_;
    }

    operator bool() const {
        return Opened();
    }

    RawSocketHandle Detach() {
        RawSocketHandle h = h_;
        h_ = kInvalidSocketHandle;
        return h;
    }

    void Open(std::error_code& ec, 
              int type = SOCK_STREAM, 
              int protocol = 0, 
              int af = AF_INET) noexcept {
        h_ = socket(af, type, protocol);
        if (!Opened()) {
            ec.assign(GetLastError(), std::system_category());
        }
    }

    void Open(int type = SOCK_STREAM,
              int protocol = 0,
              int af = AF_INET) noexcept {
        std::error_code ec;
        Open(ec, type, protocol, af);
        CheckAndThrowIfERR("socket", ec);
    }

    void Close() {
        if (Opened()) {
#ifdef _WIN32
            closesocket(Detach());
#else
            close(Detach());
#endif
        }
    }

    void Bind(std::error_code& ec, const SocketAddress& addr) noexcept {
        if (bind(h_, &addr, sizeof addr) < 0) {
            ec.assign(GetLastError(), std::system_category());
        }
    }

    void Bind(const SocketAddress& addr) {
        std::error_code ec;
        Bind(ec, addr);
        CheckAndThrowIfERR("bind", ec);
    }

    void Listen(std::error_code& ec, int backlog = kListenBacklogDefault) noexcept {
        if (listen(h_, backlog) < 0) {
            ec.assign(GetLastError(), std::system_category());
        }
    }

    void Listen(int backlog = kListenBacklogDefault) noexcept {
        std::error_code ec;
        Listen(ec, backlog);
        CheckAndThrowIfERR("listen", ec);
    }

    bool Accept(Socket *client, SocketAddress *addr = nullptr) noexcept {
        assert(client);

        SocketAddress taddr;
        if (!addr) {
            addr = &taddr;
        }

        socklen_t addrlen = sizeof taddr;
        RawSocketHandle h;
        h = accept(h_, &taddr, &addrlen);
        if (kInvalidSocketHandle == h) {
            return false;
        }

        client->h_ = h;
        return true;
    }

    void Connect(std::error_code& ec, const SocketAddress& addr) noexcept {
        if (connect(h_, &addr, sizeof addr) < 0) {
            ec.assign(GetLastError(), std::system_category());
        }
    }

    void Connect(const SocketAddress& addr) {
        std::error_code ec;
        Connect(ec, addr);
        CheckAndThrowIfERR("connect", ec);
    }

    template<int Level, int Name, typename T = int>
    void SetOpt(std::error_code& ec, const SockOpt<Level, Name, T>& opt) noexcept {
        if (setsockopt(h_,
                        Level,
                        Name,
                        reinterpret_cast<const char *>(&opt.val),
                        sizeof opt) < 0) {
            ec.assign(GetLastError(), std::system_category());
        }
    }

    template<int Level, int Name, typename T = int>
    void SetOpt(const SockOpt<Level, Name, T>& opt) {
        std::error_code ec;
        SetOpt(ec, opt);
        CheckAndThrowIfERR("setsockopt", ec);
    }

    int Send(ConstBuffer buf, int flags = 0) noexcept {
        return send(h_, buf.first, buf.second, flags);
    }

    int Recv(MutableBuffer buf, int flags = 0) noexcept {
        return recv(h_, buf.first, buf.second, flags);
    }

    int SendTo(ConstBuffer buf, const SocketAddress& peer, int flags = 0) noexcept {
        return sendto(h_, buf.first, buf.second, flags, &peer, sizeof peer);
    }

    int RecvFrom(MutableBuffer buf, SocketAddress& peer, int flags = 0) noexcept {
        socklen_t addrlen = sizeof peer;
        return recvfrom(h_, buf.first, buf.second, flags, &peer, &addrlen);
    }
private:
    RawSocketHandle h_ = kInvalidSocketHandle;
};

template<typename ... CreateOpts>
Socket CreateSocket(int type, CreateOpts&& ... opts) {
    Socket socket;
    socket.Open(type);

    // expression fold since c++17
    (std::forward<CreateOpts>(opts)(socket), ...);

    return std::move(socket);
}

using CreateSocketOption = std::function<void(Socket&)>;

inline CreateSocketOption
WithBind(const SocketAddress& addr) {
    return [=] (Socket& socket) {
        socket.Bind(addr);
    };
}

inline CreateSocketOption
WithListen(int backlog) {
    return [=](Socket& socket) {
        socket.Listen(backlog);
    };
}

inline CreateSocketOption
WithTimeoutOpt(int rcvtimeout, int sndtimeout) {
    return [=](Socket& socket) {
        if (rcvtimeout > 0) {
            socket.SetOpt(RcvTimeoutSockOpt(rcvtimeout));
        }

        if (sndtimeout > 0) {
            socket.SetOpt(SndTimeoutSockOpt(sndtimeout));
        }
    };
}

template<typename ... Opts>
CreateSocketOption WithSocketOpts(Opts&& ... opts) {
    return [=](Socket& socket) {
        (socket.SetOpt(opts), ...);
    };
}

inline CreateSocketOption
WithReuseSocketOpt(bool reuseaddr, bool reuserport) {
    return [=](Socket& socket) {
        socket.SetOpt(ReuseAddrSockOpt(reuseaddr));

#ifndef _WIN32
        socket.SetOpt(ReusePortSockOpt(reuseaddr));
#endif
    };
}

}

#endif //_SOCKET_H_INCLUDED
