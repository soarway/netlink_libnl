# netlink_libnl
Kernel Is Server ,  Userspace is client,   Server used netlink api,   client used libnl library;

# 安装和使用
内核与应用层通信的例子
你可能需要修改内核模块的Makefile文件中的KERNEL_DIR目录
我的是 KERNEL_DIR := /usr/src/kernels/4.18.0-240.10.1.el8_3.x86_64
修改好后执行 make

正常应该能编译出ko文件，然后进行加载
insmod kernspace.ko

查看是否已经加载成功
lsmod|grep kernspace


编译应用层APP需要先编译libnl库， 我用的是libnl-3.5.0
我采用的是静态库的依赖  libnl-3.a 和 libnl-genl-3.a
然后修改Makefile文件， 找到对应的.a文件路径进行make
./userApp -l 0,1,2  这是广播模式
./userApp -m abcd   这是发送消息到内核模式，并等待内核返回数据
