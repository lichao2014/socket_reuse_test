# socket_reuse_test
用来测试 socket选项SO_REUSEADDR和SO_REUSEPORT在不同平台的表现

## 编译方法
1. 切换到当前目录，cd socket_reuse_test
2. 新建并切换build目录， mkdir build && cd build
3. cmake ..
4. make 或者cmake --build .

## 测试方法
1. udp_server -port 1234 -reuseraddr -reuserport -thread 2
-port 本地端口
-reuseaddr 设置SO_REUSEADDR，默认不设置
-reuseaport 设置SO_REUSEPORT，默认不设置
-thread server及线程个数

2. udp_client -port 1235 -dstport 1234 -reuseraddr -reuserport -msg 123
-port 本地端口
-dstport 远端端口
-reuseaddr 设置SO_REUSEADDR，默认不设置
-reuseaport 设置SO_REUSEPORT，默认不设置
-msg 测试消息内容

3. tcp_server -port 1234 -reuseraddr -reuserport
-port 本地端口
-reuseaddr 设置SO_REUSEADDR，默认不设置
-reuseaport 设置SO_REUSEPORT，默认不设置

4. tcp_client -port 1235 -dstport 1234 -reuseraddr -reuserport -msg 123
-port 本地端口
-dstport 远端端口
-reuseaddr 设置SO_REUSEADDR，默认不设置
-reuseaport 设置SO_REUSEPORT，默认不设置
-msg 测试消息内容
