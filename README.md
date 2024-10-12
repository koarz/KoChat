# KoChat
服务端基于`ASIO EXAMPLE`文档里的[代码](https://think-async.com/Asio/asio-1.30.2/src/examples/cpp20/coroutines/chat_server.cpp), 在此基础上进行功能拓展
## 现有功能
- 登录验证
- 创建新账户
- 消息发送
- 消息接收
- 消息同步(最近1000条消息)

## 第三方库
网络连接部分使用了ASIO(non-boost)库, 整体采用异步处理

数据库使用DuckDB储存用户数据

LOG是自己简单封装了一下
## 计划添加功能
- 创建更多会话, 类似于QQ群可以多人加入
- 双人会话(or创建私有会话)
- 数据加密
- 多次登录失败冷却

## 现有问题
在`chat_session`的`start`函数中验证时, 一个会话会阻塞其他连接需要解决这个问题以提高使用体验

可行的方法:
1. 多线程处理
2. 将前端部分都留在`client`里, 让`client`做更多事情`server`只负责处理`client`传进来的数据