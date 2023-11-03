# LD\_PRELOAD

## setsockopt

Implementation of `setsockopt` that filters socket options.

Building:

```console
$ gcc -Wall -fPIC -shared -o setsockopt.so setsockopt.c -ldl
```

Example:

```console
$ LD_PRELOAD=./setsockopt.so ping example.com
```
