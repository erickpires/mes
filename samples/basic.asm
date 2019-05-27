NGT     equ     SB Zero ; T = 0 - T, não pede emprestado se T for 0
INV     equ     SB HFFFF;	T = T\, não pede emprestado;

      ORG 0
Zero: DW 0
Um:   DW 1
Dois: DW 2
Tres: DW 3
Sete: DW 7
Quinze: DW 15
H8000:  DW 8000h
HFFFF:  DW 0FFFFh               ; menos 1

IniRam equ 400h
Sai    equ 3FFEh
Ent    equ 3FFFh
