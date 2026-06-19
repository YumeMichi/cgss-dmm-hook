# cgss-http-hook

面向 Windows / DMM 版 CGSS 客户端的最小 `version.dll` 代理工程。

**用途**
- 转发 `UnityPlayer.dll` 实际导入的少量 `version.dll` 导出函数
- 等待 IL2CPP 运行时初始化稳定
- 运行时 hook `Stage.NetworkUtil.GetSchemeType()`，强制返回 `0`

**目录结构**
- `src/`：代理、日志、IL2CPP 符号解析、hook 逻辑
- `deps/minhook/`：内置的 MinHook 源码

**说明**
- 本项目不依赖 `Il2CppDumper` 或 `script.json`
- `GameAssembly.dll` 出现时，IL2CPP runtime 可能还不能安全访问，因此 hook 线程会额外等待一小段时间
- 运行时日志会写到游戏目录下的 `cgss-http-hook.log`
- 生成的 DLL 包含 Windows 版本资源，定义在 `src/version.rc`

**构建**
```sh
cmake -S . -B build -G Ninja -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
cmake --build build
```

**输出**
- `build/version.dll`

**使用方法**
1. 将 `build/version.dll` 复制到 `imascgstage.exe` 同目录
2. 启动游戏
3. 如果 hook 未生效，查看 `cgss-http-hook.log`
