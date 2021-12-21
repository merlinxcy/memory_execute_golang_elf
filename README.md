# memory_execute_golang_elf
内存加载执行golang elf二进制文件

本项目基于https://github.com/0xbigshaq/runtime-unpack 进行开发，通过堆栈迁移实现了参数传递
由于只是个POC，所以目前只支持32位的elf，并且使用go build保留了完整符号表。

![image](https://user-images.githubusercontent.com/18378246/146856469-6ecc76b7-8dcb-45ff-8f1a-006b489ff2a9.png)
