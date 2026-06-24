## proc detector

进程监测, 每次更新 `.confg` 文件之后需要 `systemctl restart <module_name>.service`

## 监控指标

暂时的规划有下面, 内容视情况可以更加丰富.

|功能|完成情况|概括|
|---|---|---|
|进程基础监控|Y||
|进程内存泄漏监控|N||
|系统调用异常检测|N||
|进程血缘图谱|N||


### 文件说明

各个进程配置文件单独存在, 且以 `.conf` 结尾, 内容可以参考[example.conf](./example.conf)

### 参数说明

各个命令只能存在一行(不能重复出现), 否则解析失败.

|参数|解释|默认值||
|---|---|---|---|
|MONITOR_PERIOD|监控的时间间隔，即多久进行一次进程监控操作||时间间隔是100ms * MONITOR_PERIOD|
|MONITOR_SWITCH|监控开关，用于控制是否开启对进程的监控功能||每次开关会重置监控时间|
|USER|执行监控、恢复、停止、告警等相关命令时所使用的用户身份|root|暂不支持其他用户|
|NAME|需要监控的进程的名称，在监控过程中用于标识目标进程|||
|RECOVER_COMMAND|当监控到进程出现异常时，用于恢复进程正常运行的命令|||
|MONITOR_COMMAND|用于监控进程状态的命令，程序会执行该命令来判断进程是否处于正常运行状态|||
|STOP_COMMAND|用于停止目标进程的命令|||
|ALARM_COMMAND|当监控到进程异常且恢复操作失败时，用于触发告警的命令|||

### 手动测试 tracked 进程

`proc list tracked` 只展示已经开启监控配置、发生过 `exec` 事件、且当前仍然存活的进程。短生命周期进程可能只会出现在 `/var/log/sysDetector/proc.log` 中，不一定能在 `tracked` 列表中看到。

可以用 `sleep` 创建一个长生命周期测试目标:

```bash
sudo tee /etc/sysDetector/proc/sleep.conf >/dev/null <<'EOF'
MONITOR_PERIOD=50
MONITOR_SWITCH=off
USER=root
NAME=sleep
RECOVER_COMMAND=true
MONITOR_COMMAND=true
STOP_COMMAND=true
ALARM_COMMAND=true
EOF

sudo systemctl restart sysDetector-proc.service
sleep_id=$(stat -c '%i' /etc/sysDetector/proc/sleep.conf)
sudo sysDetector-cli proc start "$sleep_id"
sleep 60 &
sudo sysDetector-cli proc list tracked
```

预期 `proc list tracked` 输出中可以看到 `COMM` 为 `sleep`、`ARGV` 类似 `sleep 60` 的进程记录。
