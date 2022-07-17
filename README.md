# build-cronet
build chromium cronet on win&mac&linux and Integrated into cmake

# 开发环境
## win
- 安装vs2019/vs2022，选择C++桌面开发组件
- 安装后给win sdk安装Debugging Tools For Windows(必须的，编译要求，默认没安装)

    进入 控制面板\程序\程序和功能，选择Windows Software Development Kit，右键更改，选择Change，勾选Debugging Tools For Windows，点击Change

## mac
xcode

# 编译步骤
- 编译cronet
```
./build_cronet.sh release
```

# 参考文档
- [cronet build instructions](https://chromium.googlesource.com/chromium/src/+/master/components/cronet/build_instructions.md)
