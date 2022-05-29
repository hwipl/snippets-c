# bpf

bpf programs:
* tc-accept: minimal tc bpf program that accepts all packets
* xdp-accept: minimal xdp program that accepts all packets
* xdp-bytes: xdp program that counts number of received bytes
* xdp-count: xdp program that counts number of received packets
* xdp-tcp6count: xdp program that counts number of received tcp/ipv6 packets
* xdp-udp4count: xdp program that counts number of received udp/ipv4 packets

custom loaders:
* tc-attach: load tc bpf program and attach it to an interface
* tc-detach: detach the tc bpf program attached with tc-attach
* tc-attach2: load tc bpf program and attach it to an interface
* tc-detach2: detach the tc bpf program attached with tc-attach2
* xdp-attach: load xdp program and attach it to an interface
* xdp-detach: detach the xdp program attached with xdp-attach

## building

### bpf

Build the c source code in file `$SRC` (e.g., `xdp-accept.c`) and output it as
elf file `$FILE` (e.g., `xdp-accept.o`):

```console
$ clang -O2 -emit-llvm -c $SRC -o - -fno-stack-protector | \
	llc -march=bpf -filetype=obj -o $FILE
```

### loaders

Build the custom bpf loader in file `$SRC` (e.g., `xdp-attach.c`) and output it
as file `$FILE` (e.g., `xdp-attach`) with clang:

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
loader tc-attach or tc-attach2:

```console
# ./tc-attach $FILE $DEV
```

```console
# ./tc-attach2 $FILE $DEV
```

### xdp

Load bpf program in section `$SEC` (e.g., `accept_all`) of file `$FILE` (e.g.,
`xdp-accept.o`) and attach it to device `$DEV` (e.g., `veth0`) with the ip
tool:

```console
# ip link set dev $DEV xdp obj $FILE sec $SEC
```

Alternatively, load the bpf program in `$FILE` and attach it to `$DEV` with the
loader xdp-attach:

```console
# ./xdp-attach $FILE $DEV
```

## unloading

### tc

Remove the clsact qdisc from device `$DEV` (e.g., `veth0`) and, thus, the
direct-action tc filter and the active bpf program with the tc tool:

```console
# tc qdisc del dev $DEV clsact
```

Alternatively, remove the bpf program from `$DEV` with tc-detach or tc-detach2:

```console
# ./tc-detach $DEV
```

```console
# ./tc-detach2 $DEV
```

### xdp

Detach active xdp program from device `$DEV` (e.g., `veth0`) with the ip tool:

```console
# ip link set dev $DEV xdp off
```

Alternatively, remove the bpf program from `$DEV` with xdp-detach:

```console
# ./xdp-detach $DEV
```
