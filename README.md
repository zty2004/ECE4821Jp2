[![License](https://img.shields.io/badge/license-MIT-blue.svg?style=plastic)](LICENSE)
[![C++](https://img.shields.io/badge/C++-20-00599C.svg?style=plastic&logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
![push status](https://focs.ji.sjtu.edu.cn/git/ece482/p2team03/actions/workflows/push.yaml/badge.svg)
![p2m1 release status](https://focs.ji.sjtu.edu.cn/git/ece482/p2team03/actions/workflows/release.yaml/badge.svg?tag=p2m1-1)

# ECE4821J Introduction to Operating Systems

## Author

p2team03

- Jiang Ruiyu
- Yao Yunxiang
- Zuo Tianyou

## Installation
### Build Instructions
1. **Clone the project**:
   ```bash
   git clone ssh://git@focs.ji.sjtu.edu.cn:2222/ece482/p2team03.git
   ```
2. **Build the project**:
   ```bash
   cd p2team03
   tools/compile
   ```
3. **Run mum shell**:
   ```bash
   ./build/lemondb
   ./build/lemondb-asan      # AddressSanitizer version
   ./build/lemondb-msan      # UndefinedBehaviorSanitizer version
   ./build/lemondb-ubsan     # MemorySanitizer version
   ```

### Clean Build
To remove all compiled files:
```bash
rm -rf build
```
