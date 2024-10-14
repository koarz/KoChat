# KoChat
服务端基于`ASIO EXAMPLE`文档里的[代码](https://think-async.com/Asio/asio-1.30.2/src/examples/cpp20/coroutines/chat_server.cpp), 在此基础上进行功能拓展
## 现有功能
- 登录验证
- 创建新账户
- 消息发送
- 消息接收
- 消息同步(最近100条消息)

## 第三方库
client使用 ftxui 提供 ui 界面

网络连接部分使用了ASIO(non-boost)库, 整体采用异步处理

数据库使用DuckDB储存用户数据

LOG是自己简单封装了一下，尝试过glog但是glog不能马上输出信息，所以继续使用klog
## 计划添加功能
- 创建更多会话, 类似于QQ群可以多人加入
- 双人会话(or创建私有会话)
- 数据加密
- 多次登录失败冷却

## 更新记录
### v1.1
1. 修复在`chat_session`的`start`函数中验证时, 一个会话会阻塞其他连接，引入了asio的线程池，使用多线程方法处理每个会话
2. client使用ftxui库封装ui
3. 将在server的处理登录的代码删除，现在在客户端进行判断，服务端仅处理传入信息，后续应该使connect阶段放在客户端每次请求登录的时候

## 现有问题
当一个client会话在home界面（也就是登录界面）时使用 Ctrl C 会导致 server 崩溃

受限于 ftxui 库本身（也可能是我太菜），message不能很好的显示，在client输入时会导致client的message界面自动回到顶部，不过鼠标再次指向message就可以恢复
