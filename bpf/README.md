# bpf

## building

### bpf

Build the c source code in file `$SRC` (e.g., `xdp-accept.c`) and output it as
elf file `$FILE` (e.g., `xdp-accept.o`):

```console
$ clang -O2 -emit-llvm -c $SRC -o - -fno-stack-protector | \
	llc -march=bpf -filetype=obj -o $FILE
```

### loaders

Build the custom bpf loader in file `$SRC` (e.g., `xdp-load.c`) and output it
as file `$FILE` (e.g., `xdp-load`) with clang:

```console
$ clang $SRC -o $FILE -l bpf
```

## loading

### tc

Load bpf program in section `$SEC` (e.g., `accept_all`) of file `$FILE` (e.g.,
`tc-accept.o`) and attach it to device `$DEV` (e.g., `veth0`) as direct-action
tc filter with the tc tool:

```console
# tc qdisc add dev $DEV clsact
# tc filter add dev $DEV ingress bpf da obj $FILE sec $SEC
```

Alternatively, load the bpf program in `$FILE` and attach it to `$DEV` with the
loader tc-load:

```console
# ./tc-load $FILE $DEV
```

### xdp

Load bpf program in section `$SEC` (e.g., `accept_all`) of file `$FILE` (e.g.,
`xdp-accept.o`) and attach it to device `$DEV` (e.g., `veth0`) with the ip
tool:

```console
# ip link set dev $DEV xdp obj $FILE sec $SEC
```

Alternatively, load the bpf program in `$FILE` and attach it to `$DEV` with the
loader xdp-load:

```console
# ./xdp-load $FILE $DEV
```

## unloading

### tc

Remove the clsact qdisc from device `$DEV` (e.g., `veth0`) and, thus, the
direct-action tc filter and the active bpf program with the tc tool:

```console
# tc qdisc del dev $DEV clsact
```

Alternatively, remove the bpf program from `$DEV` with tc-unload:

```console
# ./tc-unload $DEV
```

### xdp

Detach active xdp program from device `$DEV` (e.g., `veth0`) with the ip tool:

```console
# ip link set dev $DEV xdp off
```

Alternatively, remove the bpf program from `$DEV` with xdp-unload:

```console
# ./xdp-unload $DEV
```
