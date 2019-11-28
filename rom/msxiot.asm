;Author: Carlos Escobar
;ROM para el cartucho MSX-IoT

        OUTPUT "MSXIOT.ROM"
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
CMDPORT	EQU		1
DATPORT	EQU		0

;---------------------------
; ROM Header

        DEFW    #4241,0,HANDLER,0,0,0,0,0

;---------------------------

; BASIC CALL EXTENSION handler

HANDLER:

	PUSH    HL
	LD		HL,CMDS	        ; Table with CALL extensions
.CHKCMD:
	LD		DE,PROCNM
.LOOP:		
	LD		A,(DE)
	CP		(HL)
	JR		NZ,.TONEXTCMD	; Not equal
	INC		DE
	INC		HL
	AND		A
	JR		NZ,.LOOP	; No end of command name, go checking
	LD		E,(HL)
	INC		HL
	LD		D,(HL)
	POP		HL		; routine address
	CALL	GETPREVCHAR
	CALL	.CALLDE		; Call routine
	AND		A
	RET

.TONEXTCMD:
	LD	C,0FFH
	XOR	A
	CPIR			; Skip to end of command name
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

	DEFB	"WLIST",0      ; 
	DEFW	_WLIST
	
	DEFB	"WNET",0     ; 
	DEFW	_WNET

	DEFB	0               ; No more commands

;---------------------------
_WLIST:
	PUSH	HL
	LD		A,#10
	OUT		(CMDPORT),A
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
.LOOP1
	IN		A,(CMDPORT)
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	AND		#80
	JR		NZ,.LOOP1
.LOOP2
	IN		A,(CMDPORT)
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	AND		#40
	JR		Z,.END
	IN		A,(DATPORT)
	CALL	CHPUT
	JR		.LOOP2
.END	
	POP		HL
	OR      A
	RET
	
;---------------------------
_WNET:
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
	LD	IX,FRMEVL
	CALL	CALBAS		; Evaluate expression
	LD      A,(VALTYP)
	CP      3               ; Text type?
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