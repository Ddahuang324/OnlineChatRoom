# Quickstart: 构建与运行最小演示

先决条件:
- Qt (5.15+ 或 Qt6) 开发套件
- CMake 3.16+
- C++17 编译器

构建示例:

```bash
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH="/path/to/Qt" ..
cmake --build . --config Release
```

运行:
- 在开发阶段可用本地 MiniEvent 模拟器或 loopback 模式运行演示。
