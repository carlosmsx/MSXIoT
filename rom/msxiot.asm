;Author: Carlos Escobar 2019 Dec
;ROM para el cartucho MSX-IoT

        OUTPUT "MSXIOT.ROM"
        ORG     #4000

;---------------------------
; External variables & routines
CHPUT   	EQU     #A2
CALBAS		EQU		#159
ERRHAND 	EQU     #406F
FRMEVL  	EQU     #4C64
CHRGTR  	EQU     #4666
VALTYP  	EQU     #F663
USR     	EQU     #F7F8
PROCNM		EQU		#FD89
ENASLT  	EQU     0024H           ;enable slot
RSLREG  	EQU     0138H           ;read primary slot select register
RSLGREG 	EQU		0138H
BOTTOM		EQU		0FC48H
HIMEM		EQU		0FC4AH
SLTWRK		EQU		0FD09H
EXPTBL  	EQU     0FCC1H          ;slot is expanded or not
CALSLT		EQU		#001C

CMD_SENDSTR	EQU		#01
CMD_WLIST	EQU		#10
CMD_WCLIST	EQU		#11
CMD_WNET	EQU		#12
CMD_WPASS	EQU		#13
CMD_WCONN	EQU		#14
CMD_WDISCON	EQU		#15
CMD_WSTAT	EQU		#16
CMD_WLOAD	EQU		#17
CMD_WBLOAD	EQU		#18
CMD_TCPSVR	EQU		#19
CMD_TCPSEND	EQU		#1A
CMD_FNAME	EQU		#60
CMD_FSAVE	EQU		#61
CMD_FLOAD	EQU 	#62
CMD_FFILES	EQU 	#63
CMD_FROM	EQU		#64

CMDPORT		EQU		1
DATPORT		EQU		0
_TIMEOUT	EQU		#ffff
;---------------------------
; ROM Header

    DEFW    #4241,INIT,HANDLER,0,0,0,0,0

;---------------------------
;********************************************************
;  List 5.13  subroutines to support slot
;	       for ROM in 1 page
;********************************************************


;--------------------------------------------------------
;
;	GTSL1	Get slot number of designated page
;	Entry	None
;	Return	A	Slot address as follows
;	Modify	Flags
;
;	    FxxxSSPP
;	    |	||||
;	    |	||++-- primary slot # (0-3)
;	    |	++---- secondary slot # (0-3)
;	    |	       00 if not expanded
;	    +--------- 1 if secondary slot # specified
;
;	This value can later be used as an input parameter
;	for the RDSLT, WRSLT, CALSLT, ENASLT and 'RST 10H'
;
	;PUBLIC	GTSL10
GTSL10:
	PUSH	HL		;Save registers
	PUSH	DE

	CALL	RSLREG		;read primary slot #
	RRCA
	RRCA
	AND		11B		;[A]=000000PP
	LD		E,A
	LD		D,0		;[DE]=000000PP
	LD		HL,EXPTBL
	ADD		HL,DE		;[HL]=EXPTBL+000000PP
	LD		E,A		;[E]=000000PP
	LD		A,(HL)		;A=(EXPTBL+000000PP)
	AND		80H		;Use only MSB
	JR		Z,.GTSL1NOEXP
	OR		E		;[A]=F00000PP
	LD		E,A		;save primary slot number
	INC		HL		;point to SLTTBL entry
	INC		HL
	INC		HL
	INC		HL
	LD		A,(HL)		;get current expansion slot register
	RRCA
	RRCA
	AND		11B		;[A] = 000000SS
	RLCA
	RLCA			;[A] = 0000SS00
	OR		E		;[A] = F000SSPP
;
.GTSL1END:
	POP	DE
	POP	HL
	RET
.GTSL1NOEXP:
	LD	A,E		;[A] = 000000PP
	JR	.GTSL1END


;--------------------------------------------------------
;
;	ASLW1	Get address of slot work
;	Entry	None
;	Return	HL	address of slot work
;	Modify	None
;
	;PUBLIC	ASLW10
ASLW10:
	PUSH	DE
	PUSH	AF
	CALL	GTSL10		;[A] = F000SSPP, SS = 00 if not expanded
	AND		00001111B	;[A] = 0000SSPP
	LD		L,A		;[A] = 0000SSPP
	RLCA
	RLCA
	RLCA
	RLCA			;[A] = SSPP0000
	AND		00110000B	;[A] = 00PP0000
	OR		L		;[A] = 00PPSSPP
	AND		00111100B	;[A] = 00PPSS00
	OR		01B		;[A] = 00PPSSBB
;
;	Now, we have the sequence number for this cartridge
;	as follows.
;
;	00PPSSBB
;	  ||||||
;	  ||||++-- higher 2 bits of memory address (1)
;	  ||++---- seconday slot # (0..3)
;	  ++------ primary slot # (0..3)
;
	RLCA			;*=2
	LD		E,A
	LD		D,0		;[DE] = 0PPSSBB0
	LD		HL,SLTWRK
	ADD		HL,DE
	POP		AF
	POP		DE
	RET


;--------------------------------------------------------
;
;	RSLW1	Read slot work
;	Entry	None
;	Return	HL	Content of slot work
;	Modify	None
;
	;PUBLIC	RSLW10
RSLW10:
	PUSH	DE
	CALL	ASLW10		;[HL] = address of slot work
	LD		E,(HL)
	INC		HL
	LD		D,(HL)		;[DE] = (slot work)
	EX		DE,HL		;[HL] = (slot work)
	POP		DE
	RET


;--------------------------------------------------------
;
;	WSLW1	Write slot work
;	Entry	HL	Data to write
;	Return	None
;	Modify	None
;
	;PUBLIC	WSLW10
WSLW10:
	PUSH	DE
	EX		DE,HL		;[DE] = data to write
	CALL	ASLW10		;[HL] = address of slot work
	LD		(HL),E
	INC		HL
	LD		(HL),D
	EX		DE,HL		;[HL] = data tow write
	POP		DE
	RET


;--------------------------------------------------------
;
; How to allocate work area for cartridges
; If the work area is greater than 2 bytes, make the SLTWRK point
; to the system variable BOTTOM (0FC48H), then update it by the
; amount of memory required. BOTTOM is set up by the initizalization
; code to point to the bottom of equipped RAM.
;
;	   Ex, if the program is at 4000H..7FFFH.
;
;	WORKB	allocate work area from BOTTOM
;		(my slot work) <- (old BOTTOM)
;	Entry	HL	required memory size
;	Return	HL	start address of my work area = old BOTTOM
;			0 if cannot allocate
;	Modify	None
;
	;PUBLIC	WORKB0
WORKB0:
	PUSH	DE
	PUSH	BC
	PUSH	AF

	EX		DE,HL		;[DE] = Size
	LD		HL,(BOTTOM)	;Get current RAM bottom
	CALL	WSLW10		;Save BOTTOM to slot work
	PUSH	HL			;Save old BOTTOM
	ADD		HL,DE		;[HL] = (BOTTOM) + SIZE
	LD		A,H			;Beyond 0DFFFH?
	CP		0E0H
	JR		NC,NOROOM	;Yes, cannot allocate this much
	LD		(BOTTOM),HL	;Updtae (BOTTOM)
	POP		HL			;[HL] = old BOTTOM
WORKBEND:
	POP		AF
	POP		BC
	POP		DE
	RET
;
;	BOTTOM became greater than 0DFFFH, there is
;	no RAM to be allocated.
;
NOROOM:
	LD		HL,0
	CALL	WSLW10		;Clear slot work
	JR		WORKBEND	;Return 0 in [HL]


INIT:
	LD		HL,8
	CALL	WORKB0
	RET

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
	LD		C,0FFH
	XOR		A
	CPIR			; Skip to end of command name
	INC		HL
	INC		HL			; Skip address
	CP		(HL)
	JR		NZ,.CHKCMD	; Not end of table, go checking
	POP		HL
    SCF
	RET
	
.CALLDE:
	PUSH	DE
	RET

;---------------------------
CMDS:

; List of available commands (as ASCIIZ) and execute address (as word)

	DEFB	"FFILES",0
	DEFW	_FFILES
	DEFB	"FSAVE",0
	DEFW	_FSAVE
	DEFB	"FLOAD",0
	DEFW	_FLOAD
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
	DEFB	"TCPSVR",0
	DEFW	_TCPSVR
	DEFB	"TCPSEND",0
	DEFW	_TCPSEND
	DEFB	"LOADROM",0
	DEFW	_LOADROM
	DEFB	0               ; No more commands

;---------------------------------
_FFILES:
	LD		A,CMD_FFILES
	JR		GETLOG
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
	PUSH	BC
	OUT		(CMDPORT),A
	CALL	DEMORA
	LD		BC,_TIMEOUT
.LOOP1
	IN		A,(CMDPORT)
	CALL	DEMORA
	AND		#80
	JR		Z,.LOOP2
	;DJNZ	.LOOP1
	JR		.LOOP1
.TIMEOUT
	LD		HL,_TIMEOUTMSG
.LOOP_T
	LD		A,(HL)
	OR		A
	JR		Z,.END
	CALL	CHPUT
	INC		HL
	JR		.LOOP_T
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
	POP		BC
	POP		HL
	OR      A
	RET

_FSAVE:
	LD		A,CMD_FNAME
	CALL	_STRCMD
	PUSH	HL
	LD		HL,#8000
	LD		A,CMD_SENDSTR
	OUT		(CMDPORT),A		;envio cadena larga
	CALL	DEMORA
	LD		BC,(#F6C2)
	RES		7,B
	;LD		A,C
	;OUT		(DATPORT),A
	;CALL	DEMORA
	;LD		A,B
	;OUT		(DATPORT),A
	;CALL	DEMORA
.LOOP
	LD		A,(HL)
	OUT		(DATPORT),A
	CALL	DEMORA
	INC		HL
	DEC		BC
	LD		A,B
	OR		C
	JR		NZ,.LOOP

	LD		A,CMD_FSAVE
	OUT		(CMDPORT),A		;
	CALL	DEMORA
	
	POP		HL
	OR		A
	RET

_FLOAD:
	LD		A,CMD_FLOAD
	CALL	_STRCMD
	PUSH	HL

.LOOP1
	IN		A,(CMDPORT)
	CALL	DEMORA
	AND		#80
	JR		NZ,.LOOP1

	IN		A,(DATPORT)
	LD		C,A
	CALL	DEMORA
	IN		A,(DATPORT)
	LD		B,A
	SET		7,B
	LD		(#F6C2),BC
	LD		B,A
	CALL	DEMORA
	LD		HL,#8000
.LOOP
	IN		A,(DATPORT)
	LD		(HL),A
	CALL	DEMORA
	INC		HL
	DEC		BC
	LD		A,B
	OR		C
	JR		NZ,.LOOP
	POP		HL
	OR		A
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
_TCPSEND:
	LD		A,CMD_TCPSEND
	CALL	_STRCMD
	JP		GETLOG
;---------------------------------
_TCPSVR:
	LD		A,CMD_TCPSVR
	JR		_STRCMD
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
	LD		IX,FRMEVL
	CALL	CALBAS		; Evaluate expression
	LD      A,(VALTYP)
	CP      3               ; Text type?
	JP      NZ,TYPE_MISMATCH
	CALL	CHKCHAR
	DEFB	")"             ; Check for )
	RET


CHKCHAR:
	CALL	GETPREVCHAR	; Get previous basic char
	EX		(SP),HL
	CP		(HL) 	        ; Check if good char
	JR		NZ,SYNTAX_ERROR	; No, Syntax error
	INC		HL
	EX		(SP),HL
	INC		HL		; Get next basic char

GETPREVCHAR:
	DEC		HL
	LD		IX,CHRGTR
	JP      CALBAS


TYPE_MISMATCH:
	LD      E,13
	DB      1

SYNTAX_ERROR:
	LD      E,2
	LD		IX,ERRHAND	; Call the Basic error handler
	JP		CALBAS

_TIMEOUTMSG:
	DB		'Timeout',#13,#10,#0
	
_MEMTESTMSG:
	DB		' MemTest',#13,#10,#0
;---------------------------
PRINTHEX:
	PUSH	AF
	PUSH	AF
    CALL 	.Num1
	CALL	CHPUT
	POP		AF
	CALL 	.Num2
	CALL	CHPUT
	POP		AF
	RET  	; return with hex number in de
.Num1        
	RRA
	RRA
	RRA
	RRA
.Num2        
	OR		#F0
	DAA
	ADD 	A,#A0
	ADC 	A,#40 ; Ascii hex at this point (0 to F)   
	RET	

_LOADROM:
	LD		A,CMD_FROM
	CALL	_STRCMD
	PUSH	HL
	PUSH	BC
	PUSH	DE
	CALL	GTSL10
	CALL	PRINTHEX
	PUSH	AF
	POP		IY
	LD		IX,MXFER
	CALL	CALSLT
	LD		A,H
	CALL	PRINTHEX
	LD		A,L
	CALL	PRINTHEX
	PUSH	HL				;guardo INIT de la ROM
	CALL	MXFER0
	LD		A,64
	CALL	CHPUT
	LD		A,1
	PUSH	AF
	POP		IY
	POP		IX				;INIT de la ROM
	CALL	CALSLT

	LD		HL,_MEMTESTMSG
.LOOP_T
	LD		A,(HL)
	OR		A
	JR		Z,.END
	CALL	CHPUT
	INC		HL
	JR		.LOOP_T
.END
	POP		DE
	POP		BC
	POP		HL
	OR      A
	RET

MXFER0:
	IN		A,(#A8)		
	AND 	11001111b
	OR  	00010000b
	OUT		(#A8),A		;selecciono RAM de DPC200
	LD		HL,#8000
	LD		A,B
	CALL	PRINTHEX
	LD		A,C
	CALL	PRINTHEX

.LOOP2
	IN		A,(DATPORT)
	CALL	DEMORA
	LD		(HL),A
	INC		HL
	DJNZ	.LOOP2
	RET
	

	DS      #8000-$
	ORG		#8000
_TIMEOUTMSG2:
	DB		'Timeout(2)',#13,#10,#0

MXFER:
	;PUSH	HL
	;PUSH	DE

	IN		A,(#A8)		
	LD		E,A			;guardo el estado actual
	AND 	11110011b
	OR  	00000100b
	OUT		(#A8),A		;selecciono RAM de DPC200

	LD		A,CMD_FROM	;comando para cargar ROM
	OUT		(CMDPORT),A
	CALL	DELAY2
	LD		HL,#4000
	
.LOOP1
	IN		A,(CMDPORT)	;TODO: chequear flag de error
	CALL	DELAY2
	AND		#80
	JR		NZ,.LOOP1

	IN		A,(DATPORT)	;leo parte baja SIZE
	LD		C,A
	CALL	DELAY2
	IN		A,(DATPORT)	;leo parte alta SIZE
	LD		B,A
	CALL	DELAY2
	DEC		BC
	DEC		BC

.LOOP2
	IN		A,(DATPORT)
	CALL	DELAY2
	LD		(HL),A
	DEC		BC
	INC		HL
	LD		A,B
	OR		C
	JR		NZ,.CONTINUE
	;si se cargo el TOTAL, entonces la ROM está completa y salto al INIT
	LD		HL,(#4002)
	PUSH	HL	
	RET
	
.CONTINUE
	BIT		7,H			;chequeo que no haya alcanzado #8000, en tal caso debe retornar y continuar cargando
	JR		Z,.LOOP2

.END	
	LD		HL,(#4002)
	LD		A,E
	OUT		(#A8),A		;restauro paginas
	RET	
	
DELAY2:
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

;---------------------------
PRINTHEX2:
	PUSH	AF
	PUSH	AF
    CALL 	.Num1
	CALL	CHPUT
	POP		AF
	CALL 	.Num2
	CALL	CHPUT
	POP		AF
	RET  	; return with hex number in de
.Num1        
	RRA
	RRA
	RRA
	RRA
.Num2        
	OR		#F0
	DAA
	ADD 	A,#A0
	ADC 	A,#40 ; Ascii hex at this point (0 to F)   
	RET	

	DS      #0C000-$
