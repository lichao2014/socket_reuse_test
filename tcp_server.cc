#include "socket.h"
#include "flags.h"

#include <vector>
#include <thread>
#include <memory>
#include <atomic>

DEFINE_int(port, 1234, "local port");
DEFINE_bool(reuseport, false, "SO_REUSEPORT");
DEFINE_bool(reuseaddr, false, "SO_REUSEADDR");

namespace {
class TCPClient : public std::enable_shared_from_this<TCPClient> {
public:
    TCPClient(int id, testing::Socket&& client) : id_(id), client_(std::move(client)) {}

    void Start() {
       std::thread([sp = shared_from_this()] {
            char xxx[1024];

            while (true) {
                auto buf = testing::MakeBuffer(xxx);
                int n = sp->client_.Recv(buf);
                if (n <= 0) {
                    break;
                }

                std::clog << "client #" << sp->id_ << " got a msg " << n << std::endl;

                buf.second = n;
                sp->client_.Send(buf);
            }
        }).detach();
    }

    void Stop() {
        client_.Close();
    }

private:
    int id_;
    testing::Socket client_;
};

class TCPServer {
public:
    TCPServer() { Start(); }
    ~TCPServer() { Stop();  }

    void Start() {
        server_ = testing::CreateSocket(
            SOCK_STREAM,
            testing::WithReuseSocketOpt(FLAG_reuseaddr, FLAG_reuseport),
            testing::WithBind(testing::MakeAddress4(FLAG_port)));

        server_.Listen();

        thread_ = std::thread([this] {
            std::clog << "tcp server startup" << std::endl;

            try {
                testing::Socket c;
                testing::SocketAddress addr;
                while (server_.Accept(&c, &addr)) {
                    std::clog << "got a client " << addr.v4()->ip() << ',' << addr.v4()->port() << std::endl;

                    auto client = std::make_shared<TCPClient>(client_index_++, std::move(c));
                    clients_.emplace_back(client);
                    client->Start();
                }
            }
            catch (...) {}
        });
    }

    void Stop() {
        for (auto&& c : clients_) {
            if (auto sp = c.lock(); sp) {
                sp->Stop();
            }
        }

        server_.Close();

        if (thread_.joinable()) {
            thread_.join();
        }
    }
private:
    testing::Socket server_;
    std::thread thread_;
    std::vector<std::weak_ptr<TCPClient>> clients_;
    std::atomic_int client_index_ = 0;
};
}

int main(int argc, char *argv[]) {
    if (!testing::FlagList::ParseCommandLine(argc, argv)) {
        testing::FlagList::Print(std::clog);
        return -1;
    }

    try {
#ifdef _WIN32
        testing::WinsockInitializer<> winsock_initializer;
#endif 
        TCPServer server;
        std::cin.get();
    } catch (const testing::SocketException& e) {
        std::cerr << e.what() << '\t' << e.error_code().message() << std::endl;
    }

    return 0;
}