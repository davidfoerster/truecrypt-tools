truecrypt-tools
===============

A collection of support tools to integrate TrueCrypt into Linux.


Features
--------

 * mount TrueCrypt volumes with `mount -t truecrypt` (and consequently from
  `fstab`)
  
    * respect default keyfiles of the mountpoint owner
    
 * unmount with `umount`
 
 * unmount on suspend-to-disk
    * terminate blocking processes and possibly kill them after a grace period


Prerequesites
-------------

 * [VeraCrypt] or [TrueCrypt] (legacy)
 * `xpath(1p)` (package `libxml-xpath-perl` on Debian-based distributions)
 * `waitproc` (compile from `src/waitproc`)


[TrueCrypt]: http://truecrypt.sourceforge.net/
[VeraCrypt]: https://github.com/veracrypt/VeraCrypt
