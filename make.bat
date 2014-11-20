cd src
nmake WANT_POLL=1 -f Makefile.win
cd ..\samples
nmake -f Makefile.win
cd ..
