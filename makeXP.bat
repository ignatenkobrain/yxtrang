cd src
nmake WANT_POLL=0 -f Makefile.win
cd ..\examples
nmake -f Makefile.win
cd ..
