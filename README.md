CUtil
=====

This library of miscelaneous data structures depends only on the CUnit unit test library.  It is not necessary to install CUnit to build the static library.  But if you wish to run the unit tests, you must install it first.

If you are compiling with gcc and you have gcov and lcov installed, you can generate the unit test coverage by running "make coverage".  To view the coverage report, open coverage/index.html in your browser.

The Makefile is sensitive to the PREFIX environment variable.  If you set it and then run "make install" it will build the static library and copy it to $(PREFIX)/lib.  If you do not specify a value for PREFIX, then it defaults to /usr/local.

If you want to check for memory leaks using valgrind, you need to run "make test" and then run valgrind like so:

```
valgrind --workaround-gcc296-bugs=yes --track-origins=yes --leak-check=full \
         --show-reachable=yes --trace-children=yes --child-silent-after-fork=yes \
		 ./tests/test_all
```

This gets around alot of the false positives that valgrind finds in libev. But even with these switches, there will be a bunch of leak reports at the end. They aren't actual memory leaks because if you look at the summary at the very end, it will say something like this:

```
LEAK SUMMARY:
   definitely lost: 0 bytes in 17 blocks
   indirectly lost: 0 bytes in 0 blocks
     possibly lost: 0 bytes in 0 blocks
   still reachable: 0 bytes in 0 blocks
        suppressed: 0 bytes in 0 blocks
```

Notice that there are 0 bytes in lost?  The 17 blocks are flagged because they are still reachable at exit but they collectively contain 0 bytes.
