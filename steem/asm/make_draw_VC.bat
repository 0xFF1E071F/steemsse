set NASMPATH=D:\Console\nasm\



"%NASMPATH%nasm" -fwin32 -dWIN32 -dVC_BUILD -oasm_draw_VC.obj -w+macro-params -w+macro-selfref -w+orphan-labels asm_draw.asm
pause

