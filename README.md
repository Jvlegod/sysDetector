# sysDetector

🌐 [English](./README-EN.md) | 🇨🇳 [中文](./docs/zh/README-ZH.md)

[![EN Doc](https://img.shields.io/badge/Document-English-blue)](./README-EN.md)
[![CN Doc](https://img.shields.io/badge/文档-中文-red)](./docs/zh/README-ZH.md)

> 📌 version：v0.1.0 | [CHANGELOG](CHANGELOG.md)

Monitoring tools for linux systems.


## How to install

git clone first.

```bash
git clone git@github.com:Jvlegod/sysDetector.git
git submodule update --init --recursive
```

compile

```bash
# It should be executed in the “src/” directory
mkdir -p tmp && cd tmp
sudo cmake ..
sudo make && sudo make install
```

## How to start

"sysDetector.service" is the basic service.

```bash
sudo systemctl start sysDetector.service
systemctl status sysDetector # check if it is OK.
```

we can start with model.

```bash
# model exists in misc dir
sudo systemctl start <model_name>.service
```

## How to uninstall

```bash
# It should be executed in the "src/" directory
systemctl disable sysDetector.service
systemctl stop sysDetector.service
sudo python3 uninstall.py
```

## How to use

```bash
# TODO
sysDetector-cli --help
```

we can find out log in "/var/log/sysDetector/sysDetector_<model_name>.log"

## TODO

- full fill docs

- add selftest system

- ...