# 上位机（qt_code）编译说明

面向自动化 agent / 开发者的构建指南。工程为 Qt 5.15.2 / C++11 Windows 桌面程序（工程名 `1202`）。

## 工具链位置

本机 Qt 安装在 `D:\Industry\Qt`（**不在 PATH 中**，编译前需手动加入）：

| 工具 | 路径 |
|------|------|
| qmake（MinGW 64-bit） | `D:\Industry\Qt\5.15.2\mingw81_64\bin\qmake.exe` |
| mingw32-make | `D:\Industry\Qt\Tools\mingw810_64\bin\mingw32-make.exe` |

## 命令行编译（Git Bash）

```bash
export PATH="/d/Industry/Qt/5.15.2/mingw81_64/bin:/d/Industry/Qt/Tools/mingw810_64/bin:$PATH"
cd qt_code/build/Desktop_Qt_5_15_2_MinGW_64_bit-Debug
qmake ../../1202.pro -spec win32-g++ "CONFIG+=debug"
mingw32-make -j8
```

注意：

- 必须 `cd` 到构建目录 `qt_code/build/Desktop_Qt_5_15_2_MinGW_64_bit-Debug/` 再执行 qmake（影子构建，源码目录不被污染）。若该目录不存在则先 `mkdir -p` 创建。
- 修改了 `.pro` 或 `.ui` 文件后需重新执行 qmake；仅改 `.cpp/.h` 可直接 `mingw32-make`。
- 产物：`build/Desktop_Qt_5_15_2_MinGW_64_bit-Debug/debug/1202.exe`。整个 `build/` 已被 `.gitignore` 忽略。
- Release 构建：换用 `build/Desktop_Qt_5_15_2_MinGW_64_bit-Release` 目录，`CONFIG+=release`。

也可以用 Qt Creator 直接打开 `1202.pro` 构建（本机 QtCreator 15.0.1，套件已配好）。

## 运行依赖

可执行文件运行时需要 ZMotion SDK 动态库：

- `qt_code/third_party/zaux/zaux.dll`
- `qt_code/third_party/zaux/zmotion.dll`

二者需与 `1202.exe` 同目录或在 PATH 中。链接库 `libzaux` 已在 `.pro` 中配置（`-L.../third_party/zaux -lzaux`），无需额外设置。

## 工程结构提示

- 第三方库 **QXlsx** 以源码形式内嵌（`qt_code/QXlsx/`，由 `1202.pro` 引入其 `.pri`），编译时会看到 "This project is using private headers" 的提示，属正常。
- `mainwindow.ui` 修改后由 uic 自动生成 `ui_mainwindow.h`；`btn_xxx` 命名控件在 `MainWindow` 中通过 `on_btn_xxx_clicked()` 自动连接（QMetaObject 命名约定），无需手动 connect。
- 源码中文注释以 UTF-8 保存，行尾为 CRLF。
- 构建日志较长且大量 QXlsx Project MESSAGE 属正常输出，判断是否成功看最后是否生成 `debug/1202.exe` 且无 `error:`。
