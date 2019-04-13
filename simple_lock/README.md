## Simple ticket lock implementation

Uses x86 instruction `xadd` to ensure atomic opperation for fetch-and-add operation.

### References
1. [ticket lock](http://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf)
2. [fetch and add](https://en.wikipedia.org/wiki/Fetch-and-add)
3. [using x86 assembly in C](https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html)
