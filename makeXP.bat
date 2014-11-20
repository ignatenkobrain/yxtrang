cd src
nmake WANT_POLL=0 -f Makefile.win
cd ..\samples
nmake -f Makefile.win
cd ..
