#include "socket.h"
#include "flags.h"

#include <iostream>
#include <thread>
#include <vector>

DEFINE_int(port, -1, "local udp port");
DEFINE_bool(reuseaddr, false, "SO_REUSEADDR on");
DEFINE_bool(reuseport, false, "SO_REUSEPORT on");
DEFINE_int(thread, 1, "thread num");

namespace {
class UDPServer {
public:
    UDPServer(int id)
        : id_(id) {
        Start();
    }

    UDPServer(UDPServer&&) = default;

    ~UDPServer() {
        Stop();
    }

    void Start() {
        server_ = testing::CreateSocket(
            SOCK_DGRAM,
            testing::WithReuseSocketOpt(FLAG_reuseaddr, FLAG_reuseport),
            testing::WithBind(testing::MakeAddress4(FLAG_port)));

        thread_ = std::thread([this] {
            std::clog << "udp server " << id_ << " startup" << std::endl;

            testing::SocketAddress peer;
            char xxx[1024];

            try {
                while (true) {
                    auto buf = testing::MakeBuffer(xxx);
                    int n = server_.RecvFrom(buf, peer);
                    if (n <= 0) {
                        break;
                    }

                    std::clog << "udp server " << id_ << " got a msg from " << peer.v4()->ip() << ',' << peer.v4()->port() << std::endl;
                    buf.second = n;
                    server_.SendTo(buf, peer);
                }
            } catch (...) {}
        });
    }

    void Stop() {
        server_.Close();

        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    int id_;
    std::thread thread_;
    testing::Socket server_;
};
}

int main(int argc, char *argv[]) {
    if (!testing::FlagList::ParseCommandLine(argc, argv)) {
        testing::FlagList::Print(std::cerr);
        return -1;
    }

    if (FLAG_port == -1) {
        std::cerr << "invalid parameter port '-1'" << std::endl;
        return -1;
    }

    try {
#ifdef _WIN32
        testing::WinsockInitializer<> wsock_initializer;
#endif
        std::vector<UDPServer> udp_servers;
        for (int i = 0; i < FLAG_thread; ++i) {
            udp_servers.emplace_back(i);
        }

        std::cin.get();
    } catch (const testing::SocketException& e) {
        std::cerr << e.what() << '\t' << e.error_code().message() << std::endl;
    }
}
