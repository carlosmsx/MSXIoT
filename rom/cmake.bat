If not exist .\release md .\release

sdcc -mz80 --no-std-crt0 --no-peep --code-loc 0x4000 --data-loc 0x6000 msxiot.c 

hex2bin -e bin msxiot.ihx 
copy msxiot.bin msxiot.rom
fillfile msxiot.rom 16384 

@move *.ihx ./release > nul
@move *.noi ./release > nul
@move *.sym ./release > nul
@move *.lst ./release > nul
@move *.map ./release > nul
@move *.lk  ./release > nul
@move *.rel ./release > nul
@move *.asm ./release > nul
@move *.bin ./release > nul