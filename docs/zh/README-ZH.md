# sysDetector

🌐 [English](./docs/README-EN.md) | 🇨🇳 [中文](./docs/zh/README_ZH.md)

[![EN Doc](https://img.shields.io/badge/Document-English-blue)](./docs/README-EN.md)
[![CN Doc](https://img.shields.io/badge/文档-中文-red)](./docs/zh/README_ZH.md)

> 📌 当前文档版本：v0.1.0 | [查看更新日志](CHANGELOG.md)

Linux 系统监控工具

## 安装指南

首先克隆代码仓库:

```bash
git clone git@github.com:Jvlegod/sysDetector.git
git submodule update --init --recursive  # 初始化并更新子模块
```

编译安装

```bash
# 需要在 "src/" 目录下执行
mkdir -p tmp && cd tmp
sudo cmake ..
sudo make && sudo make install
```

## 服务启动

基础服务单元:

```bash
sudo systemctl start sysDetector.service
systemctl status sysDetector.service  # 验证服务状态
```

启动模块单元:

```bash
sudo systemctl start <模块名称>.service
```

## 卸载指南

```bash
# 需要在 "src/" 目录下执行
systemctl disable sysDetector.service  # 禁用服务
systemctl stop sysDetector.service     # 停止服务
sudo python3 uninstall.py             # 执行卸载脚本
```

## 使用说明

```bash
# 更详细的文档
sysDetector-cli --help
```

日志文件路径：

```bash
/var/log/sysDetector/sysDetector_<模块名称>.log
```

## 待办事项

- 完善文档体系

- 添加自测试系统

- fs, disk 监控等其他功能扩展...