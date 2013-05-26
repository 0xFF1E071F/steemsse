set NASMPATH=D:\Console\nasm\



"%NASMPATH%nasm" -fwin32 -oasm_osd_VC.obj -dWIN32 -dVC_BUILD -w+macro-params -w+macro-selfref -w+orphan-labels asm_osd_draw.asm
pause