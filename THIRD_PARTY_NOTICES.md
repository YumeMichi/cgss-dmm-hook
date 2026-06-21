# Third-Party Notices

本项目主体代码按 `GPL-2.0-only` 开源。

项目当前直接使用的第三方代码如下：

## MinHook

- 路径：`deps/minhook/`
- 上游：MinHook
- 许可证：BSD 2-Clause
- 本项目直接编译使用：
  - `deps/minhook/src/buffer.c`
  - `deps/minhook/src/hook.c`
  - `deps/minhook/src/trampoline.c`
  - `deps/minhook/src/hde/hde64.c`
- 原始许可证文件：
  - `deps/minhook/LICENSE.txt`

MinHook 中包含的 HDE 代码也在其许可证文件中一并声明。

## RapidJSON

- 路径：`deps/rapidjson/`
- 上游：RapidJSON
- 许可证：MIT
- 本项目使用方式：仅作为头文件库使用
- 原始许可证文件：
  - `deps/rapidjson/license.txt`

## RapidJSON 源树中的额外说明

RapidJSON 自带源码树中包含少量未参与本项目构建的附带内容，其中 `deps/rapidjson/bin/jsonchecker/` 相关文件带有 JSON License 条款。该目录当前未被本项目编译或链接使用。

如果你后续需要对外重新打包精简源码分发，且希望避免携带这部分额外条款，可以删除未使用的 `deps/rapidjson/bin/jsonchecker/` 目录及其相关测试数据，再进行发布。
