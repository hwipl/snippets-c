# netlink

## interface addresses

* add-addr: add ip address to interface
* del-addr: remove ip address from interface

## interface events

* if-addrs: listen to interface events and print address changes
* if-events: listen to interface events and print them
* if-flags: listen to interface events and print flags
* if-rtas: listen to interface events and print rtnetlink attributes
* if-stats: listen to interface events and print stats

## interface network namespaces

* set-netns-fd: set network namespace of interface to namespace identified by
  file descriptor
* set-netns-pid: set network namespace of interface to namespace identified by
  process id

## interface properties

* set-ifname: set name of interface
* set-link-up: set interface up
* set-mtu: set mtu of interface

## interfaces

* get-links: get list of interfaces
* del-link: remove interface

## tc qdiscs

* add-qdisc: add qdisc to interface

## veth interfaces

* add-veth: add veth interface pair
* add-veth-ifname1: add veth interface pair with one name specified
* add-veth-ifname2: add veth interface pair with both names specified
