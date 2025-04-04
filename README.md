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

```bash
systemctl start sysDetector.service
systemctl status sysDetector # check if it is OK.
```

## How to uninstall

```python
# It should be executed in the "src/" directory
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