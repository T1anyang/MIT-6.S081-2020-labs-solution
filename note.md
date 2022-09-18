## goal
add a kernel pagetable for every process.

ALL user's PTEs are in kernel pagetable.

$user\_pagetable \subseteq kernel\_pagetable$

- on user mode, use user's pagetable. user's pagetable only contain user's PTE, only user's virtual address can be translated & accessed

- on kernel mode, use process's kernel pagetable but not global kernel pagetable. it contains kernel's and user's PTEs. both user's and kernel's virtual address can be translated & accessed

so in kernel mode, we can also use user's addresses. they are translated by CPU, which is faster than by the `walk` function.

in Linux, a process's pagetable can translate both user's address and kernel's address. while kernel's address is acessed on user mode, a segmentation fault will be raised.

## todo

1. add a kernel pagetable for each process
    - kernel pagetable init. done
    - map kernel stack. done
    - modify scheduler. done
    - free process's kernel pagetable

2. make sure that EVERY PTE in user pagetable exists in process's kernel pagetable.
    - when the user pagetable is modified, modify the kernel pagetable too.

3. modify copyin & copyinstr
