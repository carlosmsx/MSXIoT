#ifndef _MSXIOT_FIRMWARE_H
#define _MSXIOT_FIRMWARE_H

//Wiring
//DATA BUS: PB0, PB1, PD2, PD3, PD4, PD5, PD6, PD7 
//CONTROL: PC0, PC1, PC2, PC3

#define MSX_CS_PIN A0 
#define MSX_A0_PIN A1
#define MSX_RD_PIN A2
#define MSX_EN_PIN A3
#define CS 10

#define CMD_DEBUG     0xD0
#define CMD_SENDSTR   0xE0
#define CMD_FSAVE     0xE1
#define CMD_WRITE     0xF0
#define CMD_READ      0xF1
#define CMD_INIHRD    0xF2
#define CMD_INIENV    0xF3
#define CMD_DRIVES    0xF4
#define CMD_DSKCHG    0xF5
#define CMD_CHOICE    0xF6
#define CMD_DSKFMT    0xF7
#define CMD_OEMSTAT   0xF8
#define CMD_MTOFF     0xF9
#define CMD_GETDPB    0xFA

#define CMD_PARAM__DRIVE_NUMBER   0
#define CMD_PARAM__N_SECTORS      1
#define CMD_PARAM__MEDIA          2
#define CMD_PARAM__SECTOR_H       3
#define CMD_PARAM__SECTOR_L       4
#define CMD_PARAM__ADDR_H         5
#define CMD_PARAM__ADDR_L         6
#define CMD_ST__READING_SEC       10
#define CMD_ST__READ_CRC          11
#define CMD_ST__WRITING_SEC       20

#endif
