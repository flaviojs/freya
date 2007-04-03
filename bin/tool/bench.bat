@echo off
@rem Remove the line below if you have already add it to the environment variable 'path'.
:set path=%path%;C:\borland\bcc55\bin;C:\borland\bcc55\Include;C:\borland\bcc55\lib

@echo Generating execution file of each option...
bcc32 -w-ccc -w-aus -3 -ebench bench.c
ren bench.exe bench_386.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -4 -ebench bench.c
ren bench.exe bench_486.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -5 -ebench bench.c
ren bench.exe bench_586.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -6 -ebench bench.c
ren bench.exe bench_686.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -O2 -3 -ebench bench.c
ren bench.exe bench_386o2.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -O2 -4 -ebench bench.c
ren bench.exe bench_486o2.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -O2 -5 -ebench bench.c
ren bench.exe bench_586o2.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -O2 -6 -ebench bench.c
ren bench.exe bench_686o2.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -O1 -O2 -3 -ebench bench.c
ren bench.exe bench_386o12.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -O1 -O2 -4 -ebench bench.c
ren bench.exe bench_486o12.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -O1 -O2 -5 -ebench bench.c
ren bench.exe bench_586o12.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -O1 -O2 -6 -ebench bench.c
ren bench.exe bench_686o12.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -Oc -Ov -3 -ebench bench.c
ren bench.exe bench_386ocv.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -Oc -Ov -4 -ebench bench.c
ren bench.exe bench_486ocv.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -Oc -Ov -5 -ebench bench.c
ren bench.exe bench_586ocv.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -Oc -Ov -6 -ebench bench.c
ren bench.exe bench_686ocv.exe
del bench.obj
del bench.tds

@echo Generating files for extended arithmetic tests...

bcc32 -w-ccc -w-aus -Oc -Ov -f -ff -5 -ebench bench.c
ren bench.exe bench_586ocf.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -Oc -Ov -f -ff -6 -ebench bench.c
ren bench.exe bench_686ocf.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -Oc -Ov -Oi -f -ff -5 -ebench bench.c
ren bench.exe bench_586ocfi.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -Oc -Ov -Oi -f -ff -6 -ebench bench.c
ren bench.exe bench_686ocfi.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -tWM -Oc -Ov -Oi -f -ff -5 -ebench bench.c
ren bench.exe bench_586ocfiw.exe
del bench.obj
del bench.tds

bcc32 -w-ccc -w-aus -tWM -Oc -Ov -Oi -f -ff -6 -ebench bench.c
ren bench.exe bench_686ocfiw.exe
del bench.obj
del bench.tds

@echo Running bench... (It takes about 1-3 minutes with a 2GHz CPU.)
bench_386.exe>>bench.csv
@echo ,[-3]>>bench.csv
bench_486.exe>>bench.csv
@echo ,[-4]>>bench.csv
bench_586.exe>>bench.csv
@echo ,[-5]>>bench.csv
bench_686.exe>>bench.csv
@echo ,[-6]>>bench.csv

bench_386o2.exe>>bench.csv
@echo ,[-3 -o2]>>bench.csv
bench_486o2.exe>>bench.csv
@echo ,[-4 -o2]>>bench.csv
bench_586o2.exe>>bench.csv
@echo ,[-5 -o2]>>bench.csv
bench_686o2.exe>>bench.csv
@echo ,[-6 -o2]>>bench.csv

bench_386o12.exe>>bench.csv
@echo ,[-3 -o1 -o2]>>bench.csv
bench_486o12.exe>>bench.csv
@echo ,[-4 -o1 -o2]>>bench.csv
bench_586o12.exe>>bench.csv
@echo ,[-5 -o1 -o2]>>bench.csv
bench_686o12.exe>>bench.csv
@echo ,[-6 -o1 -o2]>>bench.csv

bench_386ocv.exe>>bench.csv
@echo ,[-3 -oc -ov]>>bench.csv
bench_486ocv.exe>>bench.csv
@echo ,[-4 -oc -ov]>>bench.csv
bench_586ocv.exe>>bench.csv
@echo ,[-5 -oc -ov]>>bench.csv
bench_686ocv.exe>>bench.csv
@echo ,[-6 -oc -ov]>>bench.csv

bench_586ocf.exe>>bench.csv
@echo ,[-5 -oc -ov -f -ff]>>bench.csv
bench_686ocf.exe>>bench.csv
@echo ,[-6 -oc -ov -f -ff]>>bench.csv
bench_586ocfi.exe>>bench.csv
@echo ,[-5 -oc -ov -oi -f -ff]>>bench.csv
bench_686ocfi.exe>>bench.csv
@echo ,[-6 -oc -ov -oi -f -ff]>>bench.csv
bench_586ocfiw.exe>>bench.csv
@echo ,[-5 -tWM -oc -ov -oi -f -ff]>>bench.csv
bench_686ocfiw.exe>>bench.csv
@echo ,[-6 -tWM -oc -ov -oi -f -ff]>>bench.csv

del *.exe
@echo The measurement with the bench was completed.
@echo Please choose the optimization option referring
@echo to log file [bench.csv].
pause

