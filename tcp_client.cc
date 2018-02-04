#include "socket.h"
#include "flags.h"

#include <iostream>
#include <thread>
#include <vector>

DEFINE_int(port, -1, "local udp port");
DEFINE_int(dstport, -1, "remote udp port");
DEFINE_bool(reuseaddr, false, "SO_REUSEADDR on");
DEFINE_bool(reuseport, false, "SO_REUSEPORT on");
DEFINE_string(msg, "", "send msg");

int main(int argc, char *argv[]) {
    using namespace testing;

    if (!FlagList::ParseCommandLine(argc, argv)) {
        FlagList::Print(std::cerr);
        return -1;
    }

    if (FLAG_port == -1) {
        std::cerr << "invalid parameter port '-1'" << std::endl;
        return -1;
    }

    try {
#ifdef _WIN32
        WinsockInitializer<> wsock_initializer;
#endif
        auto client = CreateSocket(
            SOCK_STREAM,
            WithReuseSocketOpt(FLAG_reuseaddr, FLAG_reuseport),
            WithTimeoutOpt(2, 2),
            WithBind(MakeAddress4(FLAG_port)));

        client.Connect(testing::MakeAddress4(FLAG_dstport));

        while (true) {
            int n = client.Send({ FLAG_msg, strlen(FLAG_msg) });
            if (n <= 0) {
                std::cerr << "send err" << std::endl;
                return -1;
            }

            std::string buf(1024, '\0');
            n = client.Recv(MakeBuffer(buf));
            if (n <= 0) {
                std::cerr << "recv err" << std::endl;
                return -1;
            }

            std::clog << "got a response " << buf << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cin.get();
    } catch (const testing::SocketException& e) {
        std::cerr << e.what() << '\t' << e.error_code().message() << std::endl;
    }

    return 0;
}
