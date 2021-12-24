# my_game

The source code release of my_game.

---

## Table of contents

- [License](#License)
- [Assets](#Assets)
- [Compiling](#Compiling)

---

## License

See [LICENSE](LICENSE) for the BSD 3-Clause License.

Some source code in this release is not covered by the BSD 3-Clause License:

---

BSP loading and rendering based on work by Krzysztof Kondrak.
[https://github.com/kondrak/quake_bsp_vulkan](https://github.com/kondrak/quake_bsp_vulkan)

```
file(s):
  source/bsp/bsp_loader.c
  source/bsp/bsp_map.c
  source/bsp/bsp_patch.c
```

```
MIT License

Copyright (c) 2018 Krzysztof Kondrak

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

All projects contained in `third_party` are copyright of their respective owners.

[gunslinger](https://github.com/MrFrenik/gunslinger)

- File(s): `third_party/include/gs/*`
- License: [third_party/include/gs/LICENSE](third_party/include/gs/LICENSE)

---

## Assets

// TODO

## Compiling

// TODO

### Linux

```sh
sudo apt install gcc mesa-common-dev libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev
bash proc/linux/gcc_dbg.sh
```
