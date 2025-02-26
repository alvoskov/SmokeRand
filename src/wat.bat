PATH D:\WATCOM\BINNT64;D:\WATCOM\BINNT;%PATH%
SET INCLUDE=C:\WATCOM\H;D:\WATCOM\H\NT;D:\WATCOM\H\NT\DIRECTX;D:\WATCOM\H\NT\DDK;%INCLUDE%
SET WATCOM=D:\WATCOM
SET EDPATH=D:\WATCOM\EDDAT
SET WHTMLHELP=D:\WATCOM\BINNT\HELP
SET WIPFC=D:\WATCOM\WIPFC

wcc386
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=bat_brief.obj bat_brief.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=bat_default.obj bat_default.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=bat_express.obj bat_express.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=bat_file.obj bat_file.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=bat_full.obj bat_full.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=core.obj core.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=coretests.obj coretests.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -DNO_X86_EXTENSIONS -fo=entropy.obj entropy.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=extratests.obj extratests.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=fileio.obj fileio.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=hwtests.obj hwtests.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=lineardep.obj lineardep.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=smokerand.obj smokerand.c
wcc386 -q -ot -4s -za99 -br -bm -bt=nt -I../include -fo=specfuncs.obj specfuncs.c



rem wcc386 -q -ot -4s -za99 -bt=nt -bd -I../include -fo=lcg69069_shared.obj ..\generators\lcg69069_shared.c


wcc386 ..\generators\lcg69069_shared.c -br -bm -ot -mf -6s -za99 -bd -I../include -fo=lcg69069_shared.obj 
wcc386 ..\generators\chacha_shared.c -br -bm -ot -mf -6s -za99 -bd -I../include -fo=chacha_shared.obj 



del smokerand_core.lib
wlib smokerand_core.lib +-core.obj +-coretests.obj +-entropy.obj +-extratests.obj +-fileio.obj +-hwtests.obj +-lineardep.obj +-specfuncs.obj
wcl386 -4s smokerand.obj bat_brief.obj bat_default.obj bat_express.obj bat_file.obj bat_full.obj  smokerand_core.lib


rem wlink system nt file lcg69069_shared.obj
wcl386 lcg69069_shared.obj -fe=lcg69069_shared.dll -l=nt_dll -bd -q -I../include -DNO_X86_EXTENSIONS 
wcl386 chacha_shared.obj -fe=chacha_shared.dll -l=nt_dll -bd -q -I../include -DNO_X86_EXTENSIONS 
