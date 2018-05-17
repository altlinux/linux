====
AltHa
====

AltHa is a Linux Security Module currently has three userspace hardening options:
    * ignore SUID on binaries (with exceptions possible);
    * prevent running selected script interprers in interactive move;
    * disable open file unlinking in selected dirs.


It is selectable at build-time with ``CONFIG_SECURITY_ALTHA``, and should be
enabled in runtime by command line option ``altha=1`` and configuded
through sysctls in ``/proc/sys/kernel/altha``.

NoSUID
============
Modern Linux systems can be used with minimal (or even zero at least for OWL and ALT) usage of SUID programms, but in many cases in full-featured desktop or server systems there ara plenty of them: uncounted and sometimes unnessesary. Privileged programms are always a attack surface, but mounting filesystems with ``nosuid`` flag doesn't provide enouth granularity in SUID binaries manageent. This LSM module provides a single control point for all SUID binaries. When this submodule is enabled, SUID bits on all binaries except explicitally listed are system-wide ignored.

Sysctl parameters and defaults:

* ``kernel.altha.nosuid.enabled = 0``, set to 1 to enable
* ``kernel.altha.nosuid.exceptions =``, colon-separated list of enabled SUID binaries, for example: ``/bin/su:/usr/libexec/hasher-priv/hasher-priv``

RestrScript
============
There is a one way to hardening: prevent users from executing ther own arbitrary code. Thraditionally it can be done setting on user-writable filesystems ``noexec`` flag. But modern script languages such as Python also can be used to write exploits or even load arbitary machine code via ``dlopen`` and users can start scripts from ``noexec`` filesystem starting interpreter directly.
Restrscript LSM submodule provides a way to restrict some programms to be executed directly, but allows to execute them as shebang handler.

Sysctl parameters and defaults:

* ``kernel.altha.rstrscript.enabled = 0``, set to 1 to enable
* ``kernel.altha.rstrscript.interpreters =``, colon-separated list of restricted interpreters for example: ``/usr/bin/python:/usr/bin/python3:/usr/bin/perl:/usr/bin/tclsh``. Simlinks are suporrted in both ways: you can set symlink to interpreter as exception and interpreter and all symlinks on it will be restricted.

Note: in this configuration all scripts starting with ``#!/usr/bin/env python`` will be blocked.

OLock
============
Unlink disabling for open files needed for Russian sertification, but this is a nasty feature leading to DOS.

Sysctl parameters and defaults:

* ``kernel.altha.olock.enabled = 0``, set to 1 to enable
* ``kernel.altha.olock.dirs =``, colon-separated list of dirs, for example: ``/var/lib/something:/tmp/something``.
