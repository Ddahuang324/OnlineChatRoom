#include "../../include/Sever/TCPMsgHandler.hpp"
#include "../../include/MiniEventLog.hpp"

void TCPMsgHandler::handleMessage(void* arg) {
    // 处理消息的具体实现
    // 这里可以根据实际需求处理传入的 arg 参数
    log_info("TcpServerMsgHandler triggered. Ready to send an HTTP request.");
}
