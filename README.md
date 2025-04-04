# sysDetector

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
```

## TODO

- full fill docs

- add selftest system

- ...