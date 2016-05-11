truecrypt-tools
===============

A collection of support tools to integrate TrueCrypt into Linux.

Features
--------
* mount TrueCrypt volumes with `mount -o truecrypt` (and consequently from
  fstab)
    * respect default keyfiles of the mountpoint owner
* unmount with `umount`
* unmount on hibernate
    * terminate blocking processes and possibly kill them after a grace period

Prerequesites:
* TrueCrypt
* xpath
* waitproc (from src/waitproc)
