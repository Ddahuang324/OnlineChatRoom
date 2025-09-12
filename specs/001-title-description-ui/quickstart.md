````markdown
# 快速开始（本地开发）

**所在路径(绝对)**: /Users/dahuang/Desktop/编程/项目/OnlineChat/specs/001-title-description-ui/quickstart.md

前提: 已安装 Qt (包含 qmake 或 CMake 支持)、C++ 编译器、CMake、以及 MiniEvent 库的本地构建输出。

步骤摘要:
1. 构建并运行 MiniEvent 本地服务（用于开发/集成测试）
   - 在仓库 /MiniEvent 下执行构建（已存在 CMake 配置）
2. 构建应用
   - 使用 CMake: `mkdir build && cd build && cmake .. && make`
3. 启动应用并连接到本地 MiniEvent 服务

注意: 具体命令取决于目标平台；在阶段 1 完成后 quickstart 将包含精确命令示例。

````
