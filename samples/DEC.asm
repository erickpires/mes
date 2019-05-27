#include basic.asm

; Inicio do programa

Halt  MACRO
      LM Zero
      NGT
      DNP $
      ENDM

      ORG 15

      Var1 equ IniRam
      LM Quinze
Loop: EM Var1 + 2
      LM Um
      SB Var1
      DNP Loop
      Halt
