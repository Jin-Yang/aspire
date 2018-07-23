# 简介

用于将字符串解析成 `argc` 和 `argv` 。

## 编译

可以通过 `make` 生成了两个程序，其中 `test` 用于命令行的测试，也就是对应了程序的输入和输出，例如：

```
$ ./test "your args"
argv[0]: './test'
argv[1]: 'your args'
```

那么上述接口解析后对输出为 `argc=2` ，其参数分别是 `./test` 和 `your args` 。


