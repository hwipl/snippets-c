# af\_xdp

AF\_XDP socket code using libbpf:
* xdp-sock-rx: receive packets on xdp socket
* xdp-sock-tx: send packets on xdp socket

Building:

```console
$ gcc xdp-sock-rx.c -o xdp-sock-rx -lbpf
$ gcc xdp-sock-tx.c -o xdp-sock-tx -lbpf
```
