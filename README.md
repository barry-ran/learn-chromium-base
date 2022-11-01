# learn-chromium-base
learn chromium base on win&mac

编译chromium base并学习用法，支持win&mac平台

# 开发环境
## win
- 安装vs2019/vs2022(安装在默认路径)，选择C++桌面开发组件
- 安装后给win sdk安装Debugging Tools For Windows(必须的，编译要求，默认没安装)

    进入 控制面板\程序\程序和功能，选择Windows Software Development Kit，右键更改，选择Change，勾选Debugging Tools For Windows，点击Change

## mac
- xcode

# 编译步骤
- 编译
```
# 只有第一次需要clone sync
# 只有增加/删除文件后需要再次gen
./build.sh clone sync gen build release
```

# 参考文档
- [cronet build instructions](https://chromium.googlesource.com/chromium/src/+/master/components/cronet/build_instructions.md)
- [chromium docs](https://chromium.googlesource.com/chromium/src/+/refs/tags/103.0.5060.126/docs)
- [chromium_demo](https://github.com/keyou/chromium_demo/blob/c/91.0.4472/README_zh.md)
