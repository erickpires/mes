; Meu programa
#include basic.asm

Halt MACRO
     LM Zero
     NGT
     DNP $
     ENDM

    ORG 15

CP	MACRO	D,F	; 4.Copia F para D: T = F, Pede emprestado não é alterado
    LM	F
    EM	D
    ENDM

LmInv MACRO Data
      LM Data
      INV
      ENDM

TROCA MACRO	OP0,OP1,TMP	; 16.Troca os valores de OP0 e OP1 usando variavel TMP
      LM	OP0		;   T recebe o valor original de OP0, não altera o pede
      EM	TMP	; TMP = OP0
      LM	OP1
      EM	OP0	; OP0 = OP1
      LM	TMP
      EM	OP1	; OP1 = TMP
      ENDM

Espera	MACRO	; // Duração da espera: 7 * T + 3 ciclos de relógio
        LOCAL	Esp, FimEsp
Esp:	SB	Zero	; Enquanto ( T )	// 0 - T, pede se T != 0
        DNP	FimEsp
        SB	HFFFF	;   T = -1 - ( 0 - T );	// T = T - 1, nunca há pede
        DNP	Esp
FimEsp:
        ENDM

    EspSai MACRO
    LOCAL Espe,Fin
Espe:
        LM Sai
        SB Zero
        DNP Fin
        INV
        DNP Espe
Fin:
        ENDM


    Var1 equ IniRam
    Var2 equ 401h               ; IniRam + 1
    Var3 equ 402h               ; IniRam + 2

CharZero:   DW 30h

      LM Quinze
Loop: EM Var1
      NGT
      SB CharZero
      EM Sai
      EspSai
      LM Um
      SB Var1
      DNP Loop
      Halt

      LM Sete
      EM Iniram

      CP Var2, HFFFF

      LmInv Sete
      TROCA Var1,Var2, Var3
      Espera
      TROCA Var2, Var3, Var1
      Espera
