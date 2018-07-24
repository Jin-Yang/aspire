
支持自动扩容。

## Buffer

正常接口返回的是当前缓存可用大小(包括0)，如果返回 `-1` 且入参正确，那么就是因为内存不足导致。

另外，在执行自动扩容缓存大小时，提供了几个通用的扩展方法接口。

1. 可以提供预期期望大小 `int abuffer_resize(struct abuffer *buf, int expect);` 。
2. 以固定的方式增长，例如指数 `int abuffer_exponent_expand(struct abuffer *buf);` 。

