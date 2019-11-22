; Example of String handling in BASIC CALL-commands
; Made By: NYYRIKKI 16.11.2011


        OUTPUT "PRINT.ROM"
        ORG     #4000

;---------------------------

; External variables & routines
CHPUT   EQU     #A2
CALBAS	EQU	#159
ERRHAND EQU     #406F
FRMEVL  EQU     #4C64
CHRGTR  EQU     #4666
VALTYP  EQU     #F663
USR     EQU     #F7F8
PROCNM	EQU	#FD89

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
	INC	HL		; Skip address
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

	DEFB	"UPRINT",0      ; Print upper case string
	DEFW	_UPRINT
	
	DEFB	"LPRINT",0      ; Print lower case string
	DEFW	_LPRINT

	DEFB	0               ; No more commands

;---------------------------
_UPRINT:
	CALL	EVALTXTPARAM	; Evaluate text parameter
	PUSH	HL
        CALL    GETSTRPNT
.LOOP
        LD      A,(HL)
        CALL    .UCASE
        CALL    CHPUT  ;Print
        INC     HL
        DJNZ    .LOOP

	POP	HL
	OR      A
	RET

.UCASE:
        CP      "a"
        RET     C
        CP      "z"+1
        RET     NC
        AND     %11011111
        RET
;---------------------------
_LPRINT:
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