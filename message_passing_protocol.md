## 信息传输协议

在tbb flow graph中,edge根据使用的消息协议的不同,分为两种,分别是push protocol, pull protocol.
对于使用push protocol 的边,消息是被sender初始化,然后sender试图把消息传送给recevier.
对于使用pull protocol的边,消息是被receiver初始化,然后receiver试图从sender处get到消息.
**==
当消息沿着某条边发送失败时,边会自动得从一个协议换成另一个协议.==** 这样避免了消息反复传送失败.

需要注意的是,有些节点当消息传送失败时,并不会自动切换,而是会直接将消息丢弃掉.比如function node f1 连接到function node f2的情况. 如果f1给f2一条消息,此时f2在忙,那么消息就丢了.
能使用try_gey()方法的node是不会丢信息的.