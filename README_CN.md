# CrossDesk

#### 跨界连接，高效如一

----
[English](README.md) / [中文](README_CN.md)

![sup_example](https://github.com/dijunkun/continuous-desk/assets/29698109/46536bc8-3ddd-438d-bf52-dccf143f1c20)

## 简介

CrossDesk 是一个轻量级的跨平台远程桌面软件。

CrossDesk 是 [MiniRTC](https://github.com/kunkundi/minirtc.git) 实时音视频传输库的实验性应用。MiniRTC 是一个轻量级的跨平台实时音视频传输库。它具有网络透传（[RFC5245](https://datatracker.ietf.org/doc/html/rfc5245)），视频软硬编解码（H264/AV1），音频编解码（[Opus](https://github.com/xiph/opus)），信令交互，网络拥塞控制（[TCP over UDP](https://libnice.freedesktop.org/)），传输加密（[SRTP](https://tools.ietf.org/html/rfc3711)）等基础能力。


## 使用

在菜单栏“对端ID”处输入远端桌面的ID，点击“→”即可发起远程连接。

![usage1](https://github.com/dijunkun/continuous-desk/assets/29698109/2ad59e6d-bdba-46d0-90cf-cbc9c06c2278)

如果远端桌面设置了连接密码，则本端需填写正确的连接密码才能成功发起远程连接。

![incorrect password](https://github.com/dijunkun/continuous-desk/assets/29698109/cb05501c-ec4e-4adf-952d-7a55ef770a97)

发起连接前，可在设置中自定义配置项，如语言、视频编码格式等。

## 编译

依赖：
- [xmake](https://xmake.io/#/guide/installation)
- [cmake](https://cmake.org/download/)

Linux环境下需安装以下包：

```
sudo apt-get install -y software-properties-common git curl unzip build-essential libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxcb-randr0-dev libxcb-xtest0-dev libxcb-xinerama0-dev libxcb-shape0-dev libxcb-xkb-dev libxcb-xfixes0-dev libxv-dev libxtst-dev libasound2-dev libsndio-dev libxcb-shm0-dev libasound2-dev libpulse-dev
```

编译
```
git clone https://github.com/kunkundi/crossdesk.git

cd crossdesk

git submodule init 

git submodule update

xmake b crossdesk
```
运行
```
xmake r crossdesk
```

## 许可证

CrossDesk 使用 MIT 许可证，其中使用到的第三方库根据自身许可证进行分发。
