cd src
nmake WANT_POLL=1 -f Makefile.win
cd ..\examples
nmake -f Makefile.win
cd ..
