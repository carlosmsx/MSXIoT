#define PROCNM 0xFD89

void Init();
void CallHandler();

void _crt_start (void)
{
	//ROM_HEADER CODE_START:4000
	__asm
		.dw 0x4241, _Init, _CallHandler, 0, 0, 0, 0, 0
	__endasm;
}

void Init()
{
	__asm
		LD		A,#65
		CALL	#0xA2
		LD		A,#13
		CALL	#0xA2
		LD		A,#10
		CALL	#0xA2
	__endasm;
}

int strcmp(char *a, char*b)
{
	char *pa, *pb;
	for (pa = a, pb = b; *pa==*pb && *pa!='\0'; pa++, pb++);
	return *pa-*pb;
}

void out(unsigned char port, unsigned char data) __naked
{
	port;
	data;
	__asm

	ld iy,#2
    add iy,sp ;Bypass the return address of the function 

	push bc
    ld c,(iy)   ;port
    ld a,1(iy)  ;data
	out (c), a
	pop bc

    ret	
	__endasm;
}

unsigned char inp(unsigned char port) __naked
{
	port;
	__asm
	ld iy,#2
    add iy,sp ;Bypass the return address of the function 

    ld c,(iy)   ;port
    in l,(c)

    ret	
	__endasm;
}

void ClearCarryFlag()
{
	__asm
	or a
	__endasm;
}

void SetCarryFlag()
{
	__asm
	scf
	__endasm;
}

void print(char *s) __naked
{
	s;
	 __asm
		ld iy,#2
		add iy,sp
		ld l,(iy)
		ld h,1(iy)
	00001$:
		ld a,(hl)
		call #0xA2
		inc hl
		and a
		jr nz,00001$
		ret
	__endasm;
}

void putc(char c) __naked
{
	c;
	 __asm
		ld iy,#2
		add iy,sp
		ld a,(iy)
		call #0xA2
		ret
	__endasm;
}

char HEX[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

void CallWifiList()
{
	out(1, 0x10);
	unsigned char c=inp(1);
	putc('+');
	//putc(HEX[(c>>4)]);
	//putc(HEX[c&0xf]);
	//unsigned int timeOut=20000000;
	while (inp(1)==0x80) 	putc('?');
 // && --timeOut);
	putc('+');
	putc(HEX[65>>4]);
	putc(HEX[65&0xf]);
	putc('+');
	//if (timeOut==0)
	//{
	//	print("MSX IoT timeout\n");
	//}
	//else
	//{
		while (inp(1)==0x40)
		{
			unsigned char c = inp(0);
			putc(c);
			putc('.');
		}
	//}
	ClearCarryFlag();
}

void CallX()
{
	while (inp(1)==0x40)
	{
		unsigned char c = inp(0);
		putc(c);
		putc('.');
	}
	ClearCarryFlag();
}

void CallHandler()
{
	__asm
		push hl
	__endasm;
	if (!strcmp((char*)PROCNM, "TEST"))
	{
		print((char*)PROCNM);
		ClearCarryFlag();
	}
	else if (!strcmp((char*)PROCNM, "WIFILIST"))
	{
		print("procesando...1\r\n");
		CallWifiList();
	}
	else if (!strcmp((char*)PROCNM, "PPP"))
	{
		CallX();
	}
	else 
	{
		SetCarryFlag();
	}
	__asm
		POP	HL
	__endasm;
}