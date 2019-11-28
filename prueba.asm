; Example of String handling in BASIC CALL-commands
; Made By: NYYRIKKI 16.11.2011


        OUTPUT "PRUEBA.ROM"
        ORG     #4000

;---------------------------

; External variables & routines
CHPUT   EQU     #A2
CALBAS	EQU		#159
ERRHAND EQU     #406F
FRMEVL  EQU     #4C64
CHRGTR  EQU     #4666
VALTYP  EQU     #F663
USR     EQU     #F7F8
PROCNM	EQU		#FD89

;---------------------------

; ROM-file header

        DEFW    #4241,0,CALLHAND,0,0,0,0,0


;---------------------------

; General BASIC CALL-command handler

CALLHAND:

	PUSH    HL
	LD	HL,CMDS	        ; Table with "_" commands
.CHKCMD:
	LD	DE,PROCNM
.LOOP:	LD	A,(DE)
	CP	(HL)
	JR	NZ,.TONEXTCMD	; Not equal
	INC	DE
	INC	HL
	AND	A
	JR	NZ,.LOOP	; No end of command name, go checking
	LD	E,(HL)
	INC	HL
	LD	D,(HL)
	POP	HL		; routine address
	CALL	GETPREVCHAR
	CALL	.CALLDE		; Call routine
	AND	A
	RET

.TONEXTCMD:
	LD	C,0FFH
	XOR	A
	CPIR			; Skip to end of commandname
	INC	HL
	INC	HL			; Skip address
	CP	(HL)
	JR	NZ,.CHKCMD	; Not end of table, go checking
	POP	HL
    SCF
	RET
	
.CALLDE:
	PUSH	DE
	RET

;---------------------------
CMDS:

; List of available commands (as ASCIIZ) and execute address (as word)

	DEFB	"TEST",0      ; Print upper case string
	DEFW	_TEST
	DEFB	"DEC",0      ; Print upper case string
	DEFW	_DEC
	DEFB	"INSTALL",0      ; Print lower case string
	DEFW	_INSTALL

	DEFB	0               ; No more commands

;---------------------------
HEX:
	DEFB	"0123456789ABCDEF"

_TEST:
	;CALL	EVALTXTPARAM	; Evaluate text parameter
	PUSH	HL
    ;CALL    GETSTRPNT
	LD      A,#01
	CALL	HEXA
	LD		A,#23
	CALL	HEXA
	LD		A,#45
	CALL	HEXA
	LD		A,#67
	CALL	HEXA
	LD		A,#89
	CALL	HEXA
	LD		A,#AB
	CALL	HEXA
	LD		A,#CD
	CALL	HEXA
	LD		A,#EF
	CALL	HEXA
	POP	HL
	OR      A
	RET

HEXA:
	PUSH	HL
	PUSH	BC
	PUSH	DE
	LD		C,A
	AND		#0F
	LD		E,A
	SRL		C
	SRL		C
	SRL		C
	SRL		C
	XOR		A
	LD		B,A
	LD		D,A
	LD		HL,HEX
	PUSH	HL
	ADD		HL,BC
	LD		A,(HL)
	CALL    CHPUT  ;Print
	POP		HL
	ADD		HL,DE
	LD		A,(HL)
	CALL    CHPUT  ;Print
	POP		DE
	POP		BC
	POP		HL
	RET
	
_DEC:
	CALL	EVALINTARAM
	PUSH	HL
	LD      A,(#F7F8)
	CALL	PRINT_DEC
	POP		HL
	OR		A
	RET
	
PRINT_DEC:
	PUSH	AF
	LD		A,32
	CALL	CHPUT
	CALL	CHPUT
	CALL	CHPUT
	CALL	CHPUT
	POP		AF
.LOOP1
	LD		L,0
.LOOP
	CP		11
	JR		C,.IMPRIME
	SUB		10
	INC		L
	JR		.LOOP
.IMPRIME
	ADD		A,48
	CALL	CHPUT
	LD		A,8
	CALL	CHPUT
	CALL	CHPUT
	LD		A,L
	OR		A
	JR		NZ,.LOOP1
	RET
;---------------------------
_INSTALL:
	PUSH	HL

	DI
	IN		A,(#A8)
	PUSH	AF
	LD		C,A
	AND		#0C
	LD		B,A
	SLA		B
	SLA		B
	LD		A,C
	AND		%11001111
	OR		B
	;LD		A,%11010100 ;ram,cart,cart,rom
	OUT		(#A8),A
	LD		C,A
	CALL	INSTALL
	POP		AF
	OUT		(#A8),A
	EI

	POP		HL
	OR		A			;clear C flag
	RET

_LCASE:
	CALL	EVALTXTPARAM	; Evaluate text parameter
	PUSH	HL
	CALL    GETSTRPNT
.LOOP
	LD      A,(HL)
	CALL    .LCASE
	CALL    CHPUT  ;Print
	INC     HL
	DJNZ    .LOOP

	POP	HL
	OR      A
	RET

.LCASE:
	CP      "A"
	RET     C
	CP      "Z"+1
	RET     NC
	OR      %00100000
	RET
;---------------------------

GETSTRPNT:
; OUT:
; HL = String Address
; B  = Lenght

	LD      HL,(#F7F8)
	LD      B,(HL)
	INC     HL
	LD      E,(HL)
	INC     HL
	LD      D,(HL)
	EX      DE,HL
	RET

EVALTXTPARAM:
	CALL	CHKCHAR
	DEFB	"("             ; Check for (
	LD		IX,FRMEVL
	CALL	CALBAS			; Evaluate expression
	LD      A,(VALTYP)
	CP      3               ; Text type?
	JP      NZ,TYPE_MISMATCH
	CALL	CHKCHAR
	DEFB	")"             ; Check for )
	RET

EVALINTARAM:
	CALL	CHKCHAR
	DEFB	"("             ; Check for (
	LD		IX,FRMEVL
	CALL	CALBAS			; Evaluate expression
	LD      A,(VALTYP)
	CP      2               ; Int type?
	JP      NZ,TYPE_MISMATCH
	CALL	CHKCHAR
	DEFB	")"             ; Check for )
	RET
	

CHKCHAR:
	CALL	GETPREVCHAR	; Get previous basic char
	EX	(SP),HL
	CP	(HL) 	        ; Check if good char
	JR	NZ,SYNTAX_ERROR	; No, Syntax error
	INC	HL
	EX	(SP),HL
	INC	HL		; Get next basic char

GETPREVCHAR:
	DEC	HL
	LD	IX,CHRGTR
	JP      CALBAS


TYPE_MISMATCH:
	LD      E,13
	DB      1

SYNTAX_ERROR:
	LD      E,2
	LD	IX,ERRHAND	; Call the Basic error handler
	JP	CALBAS
	
;---------------------------

	DS      #8000-$
ROM2:
	ORG		#8000
	DEFW    #4241,#4010,0,0,0,0,0,0

INIT2:
	LD		A,'I'
	CALL	CHPUT
	LD		A,'N'
	CALL	CHPUT
	LD		A,'I'
	CALL	CHPUT	
	LD		A,'T'
	CALL	CHPUT
	LD		A,'2'
	CALL	CHPUT
	LD		A,'\n'
	CALL	CHPUT
	LD		B,255
.RETARDO1
	LD		A,B
	AND		#1F
	ADD		A,65
	CALL	CHPUT
	PUSH	BC
	LD		B,255
.RETARDO2
	PUSH	BC
	LD		B,20
.RETARDO3
	DJNZ	.RETARDO3
	POP		BC
	DJNZ	.RETARDO2
	POP		BC
	DJNZ	.RETARDO1
	RET
	
INSTALL:
	LD		A,65
	CALL	CHPUT
	LD		A,C
	AND		%11000000
	LD		B,A
	SRL		B
	SRL		B
	SRL		B
	SRL		B
	LD		A,C
	AND		%11110011
	OR		B
	;LD		A,%11011100 ;ram,cart,ram,rom
	OUT		(#A8),A
	LD		A,66
	CALL	CHPUT

	LD		HL,#8000
	LD		DE,#4000
	LD		BC,#4000
	LDIR
	LD		A,67
	CALL	CHPUT
	;LD		A,%11010100 ;ram,cart,ram,rom
	;OUT		(#A8),A
	JP		#0000 ;RET

HEX2:
	DEFB	"0123456789ABCDEF"

HEXA2:
	PUSH	BC
	PUSH	DE
	LD		C,A
	AND		#0F
	LD		E,A
	SRL		C
	SRL		C
	SRL		C
	SRL		C
	XOR		A
	LD		B,A
	LD		D,A
	LD		HL,HEX2
	PUSH	HL
	ADD		HL,BC
	LD		A,(HL)
	CALL    CHPUT  ;Print
	POP		HL
	ADD		HL,DE
	LD		A,(HL)
	CALL    CHPUT  ;Print
	POP		DE
	POP		BC
	RET

	
	DS		#BFFF-$
	DB		#CE