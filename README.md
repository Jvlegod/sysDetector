# sysDetector

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
cmake ..
make && make install
```