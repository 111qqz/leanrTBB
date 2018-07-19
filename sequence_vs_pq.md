
sequence 和 优先队列 是两种Buffer nodes
sequence 是每个message有一个sequence ID
优先队列是可以自己定义一个比较函数
感觉有些类似(如果定义的比较函数恰好为sequence ID的话)
但是实际上,是很不一样的.
首先,sequence ID会reject掉有重复sequence ID的message,而PQ的比较函数不会
其次,每个message的完成都需要一定时间.
如果是sequence,前面的message没有接收到,就会一直等待,保证执行是按照顺序的.
但是对于PQ,如果下一个优先级的message还没有完成,那么不会等待,会直接跳过.

