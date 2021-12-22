# memory_execute_golang_elf
内存加载执行golang elf二进制文件

本项目基于https://github.com/0xbigshaq/runtime-unpack 进行开发，通过堆栈迁移实现了参数传递
由于只是个POC，所以目前只支持32位的elf，并且使用go build保留了完整符号表。


![image](https://user-images.githubusercontent.com/18378246/146856469-6ecc76b7-8dcb-45ff-8f1a-006b489ff2a9.png)

同样也可以实现远程http加载golang

![11ac41c004e693748a55b84d3ad8aa6](https://user-images.githubusercontent.com/18378246/147041326-b0be7fba-bc14-481a-874b-1bc5e0ca8ecc.png)


用法loader：
./bin/loader \[HTTP地址，不要带上http://] \[参数]

Example: 
> ./bin/loader localhost:8888/lib/gost.packed -V
