#include basic.asm

; Inicio do programa

Halt  MACRO
      LM Zero
      NGT
      DNP $
      ENDM

      ORG 15

      Var1 equ IniRam
      Var2 equ (Var1 + 1)
      LM Quinze
Loop: EM Var2
      LM Um
      SB Var1
      DNP Loop
      Halt
