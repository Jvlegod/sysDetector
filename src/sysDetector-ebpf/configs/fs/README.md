## fs detector

文件监测, 每次更新 `.conf` 文件之后需要 `systemctl restart <module_name>.service`

当前第一版 `fs` 使用 `inotify` 监控配置目录中的目标路径, 记录写入、删除、重命名、权限/属性变更等文件事件。

## 监控指标

暂时的规划有下面, 内容视情况可以更加丰富.

|功能|完成情况|概括|
|---|---|---|
|文件基础事件监控|Y|基于 inotify 记录 write/delete/rename/chmod 类事件|
|文件事件列表查询|Y|通过 `fs list events` 查看最近事件|
|文件事件告警命令|Y|通过 `ALARM_COMMAND` 触发自定义命令|
|eBPF 文件监控|N|当前第一版暂未使用 eBPF|
|递归目录监控|N|当前仅监控配置中的单个目录|

### 文件说明

各个文件监控配置单独存在, 且以 `.conf` 结尾, 内容可以参考 [example.conf](./example.conf)。

安装后配置目录为 `/etc/sysDetector/fs/`。`fs` 模块只解析 `.conf` 文件, 其他说明文档不会作为配置读取。

### 参数说明

各个命令只能存在一行(不能重复出现), 否则解析失败.

|参数|解释|默认值||
|---|---|---|---|
|MONITOR_SWITCH|监控开关，用于控制是否开启对该路径的文件监控|off|可以通过 `fs start <id>` / `fs stop <id>` 修改|
|NAME|文件监控规则名称，用于在事件列表和日志中标识规则|||
|PATH|需要监控的目录路径||当前第一版监控该目录下的直接文件事件|
|OPERATIONS|期望监控的操作类型|write,delete,rename,chmod|当前第一版记录 write/delete/rename/chmod 类事件|
|ALARM_COMMAND|当监控到文件事件时触发的命令|true|设置为 `true` 表示不执行额外命令|

### 手动测试文件事件

`fs list` 默认展示配置列表。`fs list events` 展示当前 `sysDetector-fs.service` 进程内缓存的最近事件；服务重启后事件缓存会清空。

可以用 `/tmp/sysdetector-fs-test` 创建一个临时测试目录:

```bash
sudo mkdir -p /etc/sysDetector/fs /tmp/sysdetector-fs-test
sudo tee /etc/sysDetector/fs/tmp.conf >/dev/null <<'EOF'
MONITOR_SWITCH=off
NAME=tmp-write
PATH=/tmp/sysdetector-fs-test
OPERATIONS=write,delete,rename,chmod
ALARM_COMMAND=true
EOF

sudo systemctl restart sysDetector-fs.service
fs_id=$(stat -c '%i' /etc/sysDetector/fs/tmp.conf)
sudo sysDetector-cli fs list
sudo sysDetector-cli fs start "$fs_id"

# 触发文件事件
echo hello | sudo tee /tmp/sysdetector-fs-test/a.txt >/dev/null
sudo mv /tmp/sysdetector-fs-test/a.txt /tmp/sysdetector-fs-test/b.txt
sudo chmod 600 /tmp/sysdetector-fs-test/b.txt
sudo rm /tmp/sysdetector-fs-test/b.txt

sudo sysDetector-cli fs list events
```

预期 `fs list events` 输出中可以看到 `Operation` 为 `write`、`rename`、`chmod`、`delete` 等事件, `Path` 为 `/tmp/sysdetector-fs-test` 下对应文件。

如果需要排查 worker 原始日志, 可以辅助查看:

```bash
sudo tail -100 /var/log/sysDetector/fs.log
```
