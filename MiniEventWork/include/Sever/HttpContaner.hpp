#pragma once

#include <vector>
#include <memory>

// 前向声明，避免不必要的头文件包含
class AbstractServer;
class EventBase;
class Configure;

class HttpContainer {
public:
    HttpContainer();
    ~HttpContainer();

    // 禁止拷贝和赋值，因为容器是唯一的
    HttpContainer(const HttpContainer&) = delete;
    HttpContainer& operator=(const HttpContainer&) = delete;

    // 初始化容器和所有底层服务
    void init();

    // 添加一个服务器到容器中进行管理
    void addServer(std::unique_ptr<AbstractServer> server);

    // 启动事件循环，这是整个程序的阻塞点
    void loop();

private:
    std::unique_ptr<EventBase> base_;
    std::unique_ptr<Configure> config_;
    std::vector<std::unique_ptr<AbstractServer>> servers_;
};
