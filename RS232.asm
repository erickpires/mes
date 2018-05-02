Duplica	MACRO	x	; // T = 2 * x ( 6 ciclos )
    LM	x
    SB	Zero	; T = 0 - x
    SB	x	; T = x - T  // T = x - ( 0 - x )
    ENDM

InvOrdBits	MACRO	x Peso y
    ; LOCAL	Inv1Bit  Bit0
    LM	Zero
    EM	y	; y = 0
    LM	Um
    EM	Peso	; Peso = 1
Inv1Bit:LM	x	; Repete {
    SB	x7FFF	;    T = 0x7FFF - x  // pede se x > 0x7FFF
    DNP	bit0	;    Se pede ( x >= 0x8000 )
    LM	Peso
    SB	Zero
    SB	y
    EM	y	;	y += Peso
Bit0:	Duplica	x
    EM	x	;    x = 2 * x
    Duplica	Peso
    EM	Peso	;    Peso = 2 * Peso
    SB	xFF	;    T = 0xFF - Peso   // pede se Peso > 0xFF
    DNP	Inv1Bit	;     } enquanto ( Peso <= 0xFF )
    ENDM

Espera	MACRO	; // Duração da espera: 7 * T + 3 ciclos de relógio
    ; LOCAL	Esp  FimEsp
Esp:	SB	Zero	; Enquanto ( T )	// 0 - T  pede se T != 0
    DNP	FimEsp
    SB	0xFFFF	;   T = -1 - ( 0 - T )  // T = T - 1  nunca há pede
    DNP	Esp
FimEsp: LM	Aqui
    EM
    ENDM


EscTTY:	EM	Aux2	; EscTTy ( Carac : Aux1 )
    InvOrdBits	Aux1 Aux3 Aux4
    LM	Um
    EM	Aux3	; Aux3 = 1  // contador de bits
    EM	ES	; ES = 0    // Início do start bit
Env1Bit: Duplica	TempoMeioBit;     T = Duplica ( TempoMeioBit )  // 6
    SB	Cinco	;					// 2
    SB	Zero	;     T = T - 5             // 2
    Espera		;     Espera ( T )          // TempoMeioBit * 14 - 32
    Duplica	Aux4	;     T = 2 * Aux4          // 6
    EM	Aux4	;     Aux4 = T          //	^
    EM	ES	;     ES = T   // envia o bit 8
    LM  XFFFF	;					//	12
    SB	Aux3
    EM	Aux3	;     Aux3++
    SB	Oito	;     T = 8 - Aux3  // pede se Aux3 > 8  V
    DNP	Env1Bit	;	} enquanto ( Aux3 <= 8 )     // 1
    LM	xFFFF	;				// Total:  TempoMeioBit * 14 - 3
    EM	ES	; ES = 1    // Início do stop bit
    Duplica TempoMeioBit
    Espera		; Espera ( Duplica ( TempoMeioBit )
    SB	XFFFF
    DNP	Aux2

LeTTY:	EM	Aux2
    LM	xFF
    EM	Aux1	; Aux1 = 0xFF   // byte recebido
    LM	Um
    EM	Aux3	; Aux3 = 1  // Peso do bit
EspIni:	LM	x8000	; repete { repete
    SB	ES		;	T = ES - 0x8000     // pede se ES < 0x8000
    DNP	EspIni	;		enquanto ( T >= 0x8000 )
    LM	TempoMeioBit
    Espera		;      Espera ( TempoMeioBit )
    LM	x8000
    SB	ES	;      T = ES - 0x8000  // pede se ES < 0x8000
    DNP	EspIni	;    } enquanto ( T >= 0x8000 )
Rec1Bit: Duplica TempoMeioBit
    Espera		;	Espera ( DuplicaTempoMeioBit )
    LM	x8000
    SB	ES	;	// T = ES - 0x8000  pede se ES < 0x8000
    DNP	RecBt1	;	Se ( ES < 0x8000 )
    LM	Aux3	;		// peso
    SB	Aux1
    EM	Aux1	;		Aux1 -= Aux3
    Duplica	Aux3
    EM	Aux3	;		Aux3 = Aux3 * 2
    SB	xFF	;		// T = 0xFF - Aux3  pede se Aux3 > 0xFF
    DNP	Rec1Bit	;     } enquanto ( Aux3 <= 0xFF )
    SB	0xFFFF
    LM	Aux1
    DNP	Aux2
