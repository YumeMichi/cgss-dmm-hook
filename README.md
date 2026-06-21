# cgss-dmm-hook

面向 Windows / DMM 版 CGSS 客户端的最小 `version.dll` 代理工程。

**用途**
- 转发 `UnityPlayer.dll` 实际导入的少量 `version.dll` 导出函数
- 等待 IL2CPP 运行时初始化稳定
- 可选地 hook `Stage.NetworkUtil.GetSchemeType()`，在需要时强制切到 `http`
- 支持通过 `config.json` 覆盖 `api_url` 和 `asset_url`
- 自动拉起同目录下的 `cgss-borderless-helper.exe`，用外部 helper 处理 `F11` 无边框全屏

**目录结构**
- `src/`：代理、日志、IL2CPP 符号解析、hook 逻辑
- `deps/minhook/`：内置的 MinHook 源码

**说明**
- 本项目不依赖 `Il2CppDumper` 或 `script.json`
- `GameAssembly.dll` 出现时，IL2CPP runtime 可能还不能安全访问，因此 hook 线程会额外等待一小段时间
- 运行时日志会写到游戏目录下的 `cgss-dmm-hook.log`
- 生成的 DLL 包含 Windows 版本资源，定义在 `src/version.rc`
- 支持通过 `force_http` 控制是否 hook scheme，默认 `true`
- `config.json` 中的 URL 会自动规范化：若写了 `http://` 或 `https://` 会自动去掉，若末尾缺少 `/` 会自动补上

**构建**
```sh
cmake -S . -B build -G Ninja -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
cmake --build build
```

或直接使用：
```sh
make
```

**输出**
- `build/version.dll`
- `build/cgss-borderless-helper.exe`

**config.json**
```json
{
  "force_http": true,
  "api_url": "apis.game.starlight-stage.jp/",
  "asset_url": "asset-starlight-stage.akamaized.net/",
  "launch_borderless_helper": true
}
```

字段说明：
- `force_http`：是否强制将 scheme 改成 `http`。默认 `true`
- `api_url`：API 主机名，按原始字段格式填写，不带 scheme
- `asset_url`：资源主机名，按原始字段格式填写，不带 scheme
- `launch_borderless_helper`：是否自动启动 `cgss-borderless-helper.exe`。默认 `true`

**使用方法**
1. 将 `build/version.dll` 和 `build/cgss-borderless-helper.exe` 一起复制到 `imascgstage.exe` 同目录
2. 如需覆盖地址，在同目录放置 `config.json`
3. 启动游戏
4. 进入游戏后按 `F11` 切换无边框全屏
5. 如果 hook 未生效，查看 `cgss-dmm-hook.log` 和 `cgss-borderless-helper.log`

说明：
- `cgss-borderless-helper.exe` 现在只负责外部窗口 borderless 行为，不再修改 Unity 内部 `Screen.SetResolution`。
- 这样更接近 Borderless Gaming 的工作方式，也能减少退出时的干扰。
- `version.dll` 只在 `imascgstage.exe` 主进程中继续启动 hook 和 helper；被其他进程间接加载时会跳过。

**致谢**
- 本项目的 `version.dll` 代理注入思路与运行时 IL2CPP hook 方向，受 `gkms-localify-dmm` 项目启发。
- `cgss-borderless-helper.exe` 的外部无边框全屏处理思路，参考了 Borderless Gaming 项目；本项目同样按 GPL 开源。
