# bpf

## building

Build the c source code in file `$SRC` (e.g., `xdp-accept.c`) and output it as
elf file `$FILE` (e.g., `xdp-accept.o`):

```console
$ clang -O2 -emit-llvm -c $SRC -o - -fno-stack-protector | \
	llc -march=bpf -filetype=obj -o $FILE
```

## tc

### Loading

Load bpf program in section `$SEC` (e.g., `accept_all`) of file `$FILE` (e.g.,
`tc-accept.o`) and attach it to device `$DEV` (e.g., `veth0`) as direct-action
tc filter:

```console
# tc qdisc add dev $DEV clsact
# tc filter add dev $DEV ingress bpf da obj $FILE sec $SEC
```

### Unloading

Remove the clsact qdisc from device `$DEV` (e.g., `veth0`) and, thus, the
direct-action tc filter and the active bpf program:

```console
# tc qdisc del dev $DEV clsact
```

## xdp

### Loading

Load bpf program in section `$SEC` (e.g., `accept_all`) of file `$FILE` (e.g.,
`xdp-accept.o`) and attach it to device `$DEV` (e.g., `veth0`):

```console
# ip link set dev $DEV xdp obj $FILE sec $SEC
```

### Unloading

Detach active xdp program from device `$DEV` (e.g., `veth0`):

```console
# ip link set dev $DEV xdp off
```
