支持标注输入和socket连接输出

fd READ事件 callback：
如果是server fd
1. 建立新连接
2. 新连接的fd设为非阻塞
3. 加入loop，监听新连接的fd
如果不是server fd
1. 读取数据
2. 原封不动写到fd


