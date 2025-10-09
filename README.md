# CrossDesk

#### Bridging work, uniting efficiency

----
[中文](README_CN.md) / [English](README.md)

![sup_example](https://github.com/dijunkun/continuous-desk/assets/29698109/46536bc8-3ddd-438d-bf52-dccf143f1c20)

# Intro

CrossDesk is a lightweight cross-platform remote desktop software.

CrossDesk is an experimental application of [MiniRTC](https://github.com/kunkundi/minirtc.git), a lightweight cross-platform real-time audio and video transmission library. MiniRTC provides fundamental capabilities including network traversal ([RFC5245](https://datatracker.ietf.org/doc/html/rfc5245)), video software/hardware encoding and decoding (H264/AV1), audio encoding/decoding ([Opus](https://github.com/xiph/opus)), signaling interaction, network congestion control ([TCP over UDP](https://libnice.freedesktop.org/)), and transmission encryption ([SRTP](https://tools.ietf.org/html/rfc3711)).

## Usage

Enter the remote desktop ID in the menu bar’s “Remote ID” field and click “→” to initiate a remote connection.

![usage1](https://github.com/dijunkun/continuous-desk/assets/29698109/2ad59e6d-bdba-46d0-90cf-cbc9c06c2278)

If the remote desktop requires a connection password, you must enter the correct password on your side to successfully establish the connection.

![incorrect password](https://github.com/dijunkun/continuous-desk/assets/29698109/cb05501c-ec4e-4adf-952d-7a55ef770a97)

Before connecting, you can customize configuration options in the settings, such as language and video encoding format.

## How to build

Requirements:
- [xmake](https://xmake.io/#/guide/installation)
- [cmake](https://cmake.org/download/)

Following packages need to be installed on Linux:

```
sudo apt-get install -y software-properties-common git curl unzip build-essential libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxcb-randr0-dev libxcb-xtest0-dev libxcb-xinerama0-dev libxcb-shape0-dev libxcb-xkb-dev libxcb-xfixes0-dev libxv-dev libxtst-dev libasound2-dev libsndio-dev libxcb-shm0-dev libasound2-dev libpulse-dev
```

Build:
```
git clone https://github.com/kunkundi/crossdesk.git

cd crossdesk

git submodule init 

git submodule update

xmake b crossdesk
```
Run:
```
xmake r crossdesk
```

## LICENSE

CrossDesk is licenced under MIT, and some third-party libraries are distributed under their licenses.

