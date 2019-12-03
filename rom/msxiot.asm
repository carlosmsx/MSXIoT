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

CMD_SENDSTR	EQU	#01
CMD_WLIST	EQU	#10
CMD_WCLIST	EQU	#11
CMD_WNET	EQU	#12
CMD_WPASS	EQU	#13
CMD_WCONN	EQU	#14
CMD_WDISCON	EQU	#15
CMD_WSTAT	EQU	#16
CMD_WLOAD	EQU	#17
CMD_WBLOAD	EQU	#18

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

	DEFB	"WLIST",0
	DEFW	_WLIST
	DEFB	"WCLIST",0
	DEFW	_WCLIST
	DEFB	"WNET",0
	DEFW	_WNET
	DEFB	"WPASS",0
	DEFW	_WPASS
	DEFB	"WCONNECT",0
	DEFW	_WCONNECT
	DEFB	"WDISCONNECT",0
	DEFW	_WDISCONNECT
	DEFB	"WSTATUS",0
	DEFW	_WSTATUS
	DEFB	"WBLOAD",0
	DEFW	_WBLOAD
	DEFB	"WLOAD",0
	DEFW	_WLOAD
	DEFB	0               ; No more commands

;---------------------------------
_WCONNECT:
	LD		A,CMD_WCONN
	JR		GETLOG
;---------------------------------
_WDISCONNECT:
	LD		A,CMD_WDISCON
	JR		GETLOG
;---------------------------------
_WSTATUS:
	LD		A,CMD_WSTAT
	JR		GETLOG
;---------------------------
_WCLIST:
	LD		A,CMD_WCLIST
	JR		GETLOG
;---------------------------
_WLIST:
	LD		A,CMD_WLIST

GETLOG:
	PUSH	HL
	OUT		(CMDPORT),A
	CALL	DEMORA
.LOOP1
	IN		A,(CMDPORT)
	CALL	DEMORA
	AND		#80
	JR		NZ,.LOOP1
.LOOP2
	IN		A,(CMDPORT)
	CALL	DEMORA
	AND		#40
	JR		Z,.END
	IN		A,(DATPORT)
	CALL	DEMORA
	CALL	CHPUT
	JR		.LOOP2
.END	
	POP		HL
	OR      A
	RET
	
SENDSTR:
	LD		A,CMD_SENDSTR
	OUT		(CMDPORT),A
	CALL	DEMORA
.LOOP
	LD		A,(HL)
	OUT		(DATPORT),A
	CALL	DEMORA
	INC		HL
	DJNZ	.LOOP
	RET
	
DEMORA:
	PUSH	AF
	LD		A,10
.LOOP
	NOP
	NOP
	NOP
	NOP
	NOP
	DEC		A
	JR		NZ,.LOOP
	POP		AF
	RET

;---------------------------------
_WPASS:
	LD		A,CMD_WPASS
	JR		_STRCMD
;---------------------------------
_WBLOAD:
	LD		A,CMD_WBLOAD
	JR		_STRCMD
;---------------------------------
_WLOAD:
	LD		A,CMD_WLOAD
	JR		_STRCMD
;---------------------------
_WNET:
	LD		A,CMD_WNET

_STRCMD:
	PUSH	AF
	CALL	EVALTXTPARAM	; Evaluate text parameter
	PUSH	HL
    CALL    GETSTRPNT
	CALL	SENDSTR
	POP		HL
	POP		AF
	OUT		(CMDPORT),A
	CALL	DEMORA
	OR      A
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