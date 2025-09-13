#include "../../include/Sever/HttpContaner.hpp"  // 修正路径与文件名（原来指向不存在的 Container/HttpContainer.hpp）
#include "../../include/EventBase.hpp"
#include "../../include/AbstractSever.hpp"
#include "../../include/Configure/Configure.hpp"
#include "../../include/MiniEventLog.hpp"

HttpContainer::HttpContainer() {
    log_info("HttpContainer created.");
}

HttpContainer::~HttpContainer() {
    // 智能指针会自动释放所有服务器、EventBase和Configure资源
    log_info("HttpContainer destroyed.");
}

void HttpContainer::init() {
    // 创建核心组件
    base_ = std::make_unique<EventBase>();
    config_ = std::make_unique<Configure>("data.cfg"); // 假设配置文件名为data.cfg

    if (!config_->isLoaded()) {
        log_error("Failed to load configuration file. Exiting.");
        exit(1);
    }
}

void HttpContainer::addServer(std::unique_ptr<AbstractServer> server) {
    if (server) {
        servers_.push_back(std::move(server));
    }
}

void HttpContainer::loop() {
    if (!base_) {
        log_error("Container not initialized. Call init() before loop().");
        return;
    }

    // 启动所有被添加的服务器
    for (const auto& server : servers_) {
        // server->run() 会调用每个服务器自己的 listen() 方法
        // 这里体现了多态的强大威力
        server->run(base_.get());
    }

    log_info("HttpContainer starting event loop...");
    // 启动事件循环，程序将在此处阻塞，等待事件发生
    base_->loop();

    log_info("HttpContainer event loop finished.");
}
