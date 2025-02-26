PATH C:\WATCOM\BINNT64;C:\WATCOM\BINNT;%PATH%
SET INCLUDE=C:\WATCOM\H;C:\WATCOM\H\NT;C:\WATCOM\H\NT\DIRECTX;C:\WATCOM\H\NT\DDK;%INCLUDE%
SET WATCOM=C:\WATCOM
SET EDPATH=C:\WATCOM\EDDAT
SET WHTMLHELP=C:\WATCOM\BINNT\HELP
SET WIPFC=C:\WATCOM\WIPFC

wcc386
wcc386 -q -ot -za99 -I../include -fo=bat_brief.obj bat_brief.c
wcc386 -q -ot -za99 -I../include -fo=bat_default.obj bat_default.c
wcc386 -q -ot -za99 -I../include -fo=bat_express.obj bat_express.c
wcc386 -q -ot -za99 -I../include -fo=bat_file.obj bat_file.c
wcc386 -q -ot -za99 -I../include -fo=bat_full.obj bat_full.c
wcc386 -q -ot -za99 -I../include -fo=core.obj core.c
wcc386 -q -ot -za99 -I../include -fo=coretests.obj coretests.c
wcc386 -q -ot -za99 -I../include -DNO_X86_EXTENSIONS -fo=entropy.obj entropy.c
wcc386 -q -ot -za99 -I../include -fo=extratests.obj extratests.c
wcc386 -q -ot -za99 -I../include -fo=fileio.obj fileio.c
wcc386 -q -ot -za99 -I../include -fo=hwtests.obj hwtests.c
wcc386 -q -ot -za99 -I../include -fo=lineardep.obj lineardep.c
wcc386 -q -ot -za99 -I../include -fo=smokerand.obj smokerand.c
wcc386 -q -ot -za99 -I../include -fo=specfuncs.obj specfuncs.c

rem wcc386 -q -za99 -I../include -DNO_X86_EXTENSIONS -fo=kiss93_shared.obj ..\generators\kiss99_shared.c

wlib smokerand_core.lib +-core.obj +-coretests.obj +-entropy.obj +-extratests.obj +-fileio.obj +-hwtests.obj +-lineardep.obj +-specfuncs.obj
wcl386 smokerand.obj bat_brief.obj bat_default.obj bat_express.obj bat_file.obj bat_full.obj  smokerand_core.lib
wcl386 ..\generators\kiss93_shared.c -ecd -fe=kiss93_shared.dll -bd -q -za99 -I../include -DNO_X86_EXTENSIONS 

rem +-bat_brief.obj +-bat_default.obj +-bat_file.obj +-bat_full.obj 