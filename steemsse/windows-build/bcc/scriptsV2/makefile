# makefile sor Steem SSE Borland C/C++ 5.5 build
# used by the bat scripts for user and debug (boiler) versions

# set those variables to the correct directories if needed
# ROOT is the folder above 3rdparty & steem, it is the top folder 
# for source files, C_ST on my computer
# schema:
# from wherever on your disk
#  |
#  C_ST
#   |
#   steem
#    |
#    code
#    ...
#   3rdparty
#   include
#   windows-build
#   X-build
#  NASM (folder containing the NASM exe)
#  BCC (Borland C++ 5.5 folder)
#   |
#   bin
#  emu
#   |
#   Steem (WE ARE HERE: makefile & bat scripts)
#    |
#    BCCDebugBuild       you must create this folder
#    BCCNormalBuild      you must create this folder
#    obj                 you must create this folder
#  ...
STEEMROOT = $(ROOT)\steem
ASMINCOBJDIR = obj
INCDIR = $(ROOT)\include
ASMOBJDIR = obj
3RDPARTYROOT = $(ROOT)\3rdparty

# Note that we don't use unrarlib anymore. Borland linker is smart enough to 
# not include anything useless in the EXE, but we remove it from the project
# to avoid useless compile time.
#3RDPARTYOBJS = obj\unrarlib.obj obj\div68kCycleAccurate.obj obj\dsp.obj obj\6301.obj
3RDPARTYOBJS = obj\div68kCycleAccurate.obj obj\dsp.obj obj\6301.obj

INCLUDEPATHS = $(STEEMROOT)\code;$(BCCROOT)\Include;$(INCDIR);$(3RDPARTYROOT);$(3RDPARTYROOT)\unrarlib;$(3RDPARTYROOT)\pasti;$(3RDPARTYROOT)\6301

ALLOBJS = $(ASMOBJDIR)\asm_draw.obj obj\emu.obj c0w32.obj obj\Steem.obj $(ASMOBJS) obj\helper.obj
ALLOBJS = $(ALLOBJS) $(ASMOBJDIR)\asm_osd_draw.obj $(ASMINCOBJDIR)\asm_portio.obj
ALLOBJS = $(ALLOBJS) obj\SSE.obj obj\SSEOption.obj

!ifndef ONEGAME

ALLOBJS = $(ALLOBJS) $(3RDPARTYOBJS)
NAME = Steem
RC_PATH = $(STEEMROOT)\rc\resource.rc

!else

NAME = $(ONEGAME_NAME)
RC_PATH = $(STEEMROOT)\rc\onegame\$(ONEGAME_NAME).rc

!endif

###################################################################
CPP        = $(BCCROOT)\Bin\bcc32
CPP32      = $(BCCROOT)\Bin\cpp32
LIBRARIAN  = $(BCCROOT)\Bin\tlib /P128
LINKER     = $(BCCROOT)\Bin\ilink32
RC         = $(BCCROOT)\Bin\brcc32

#    Set the linker options
LINKOPTS = -L$(BCCROOT)\Lib;$(BCCROOT)\Lib\PSDK -aa -Tpe -t -x -w -v- -C -Gn

#    Set the compiler options
STEEMFLAGS = -DBCC_BUILD -DWIN32 -DSTEVEN_SEAGAL
CCOPTS = -D_NO_VCL -DWINVER=0x0400 -D_WIN32_WINNT=0x0300
CCOPTS = -w  -O2 -OS -Oi -Ov -O -Oc -K-                  $(CCOPTS)
CCOPTS = -jb -j10 -tWM  -r -a4  -6  -vi -d -ff -fp       $(CCOPTS)
CCOPTS = -v- -x -xf -xp- -xd -xs- -H- -y- -X             $(CCOPTS)


!ifndef ONEGAME

OUTPUTDIR = obj

!ifdef _DONT_ALLOW_DEBUG
CCOPTS = -DNO_DEBUG_BUILD                                $(CCOPTS)
OUTPUTDIR = $(OUT)
!endif

!ifdef _FORCE_DEBUG_BUILD
CCOPTS = -DDEBUG_BUILD                                   $(CCOPTS)
OUTPUTDIR = $(OUT)
!endif

!else

ONEGAME_IDX=0
!if $(ONEGAME_NAME)==nm1
ONEGAME_IDX=1
!elif $(ONEGAME_NAME)==nm2
ONEGAME_IDX=2
!elif $(ONEGAME_NAME)==aw1
ONEGAME_IDX=3
!elif $(ONEGAME_NAME)==aw2
ONEGAME_IDX=4
!elif $(ONEGAME_NAME)==sat1
ONEGAME_IDX=5
!elif $(ONEGAME_NAME)==sat2
ONEGAME_IDX=6
!endif

CCOPTS = -DOG_NM1_IDX=1;OG_NM2_IDX=2;OG_AW1_IDX=3        $(CCOPTS)
CCOPTS = -DOG_AW2_IDX=4;OG_SAT1_IDX=5;OG_SAT2_IDX=6      $(CCOPTS)
CCOPTS = -DONEGAME;ONEGAME_NAME="\"$(ONEGAME_NAME)\""    $(CCOPTS)
CCOPTS = -DONEGAME_IDX=$(ONEGAME_IDX)                    $(CCOPTS)
OUTPUTDIR = $(ONEGAME_PATH)

!endif

!ifdef BCC_BUILD
CCOPTS = -DBCC_BUILD                                     $(CCOPTS)
!endif

CPPFLAGS= -I$(INCLUDEPATHS) $(CCOPTS)

######################################################################################
######################################################################################
######################################################################################

ALL: Steem.res Steem.obj helper.obj emu.obj SSE.obj SSEOption.obj $(NAME).exe

Steem.res:
           $(RC) $(RC_PATH) -foobj\Steem.res

Steem.obj:
           $(CPP) $(CPPFLAGS) $(STEEMFLAGS) -c -oobj\Steem.obj $(STEEMROOT)\Steem.cpp

helper.obj:
           $(CPP) $(CPPFLAGS) $(STEEMFLAGS) -c -oobj\helper.obj $(STEEMROOT)\helper.cpp

emu.obj:
           $(CPP) $(CPPFLAGS) $(STEEMFLAGS) -c -oobj\emu.obj $(STEEMROOT)\emu.cpp

SSE.obj:
           $(CPP) $(CPPFLAGS) $(STEEMFLAGS) -c -oobj\SSE.obj $(STEEMROOT)\code\SSE\SSE.cpp

SSEOption.obj:
           $(CPP) $(CPPFLAGS) $(STEEMFLAGS) -c -oobj\SSEOption.obj $(STEEMROOT)\code\SSE\SSEOption.cpp

$(NAME).exe:
           $(LINKER) $(LINKOPTS) $(ALLOBJS), "$(OUTPUTDIR)\$(NAME).exe", ,import32.lib cw32mt.lib, ,obj\Steem.res

#3rdparty:	unrarlib div68kCycleAccurate dsp 6301

3rdparty:	div68kCycleAccurate dsp 6301

#unrarlib:
#	$(CPP) $(CPPFLAGS) -DWIN32 -c -oobj\unrarlib.obj $(3RDPARTYROOT)\unrarlib\unrarlib\unrarlib.c $(CFLAGS) 
div68kCycleAccurate:
        $(CPP) $(CPPFLAGS) -DWIN32 -c -oobj\div68kCycleAccurate.obj $(3RDPARTYROOT)\pasti\div68kCycleAccurate.c $(CFLAGS) 
dsp:
        $(CPP) $(CPPFLAGS) -DWIN32 -c -oobj\dsp.obj $(3RDPARTYROOT)\dsp\dsp.cpp
6301:
!ifdef SS_DEBUG
        $(CPP) $(CPPFLAGS) -DWIN32 -DSS_IKBD_6301_TRACE -c -oobj\6301.obj $(3RDPARTYROOT)\6301\6301.c $(CFLAGS) 
!else
        $(CPP) $(CPPFLAGS) -DWIN32 -c -oobj\6301.obj $(3RDPARTYROOT)\6301\6301.c $(CFLAGS) 
!endif