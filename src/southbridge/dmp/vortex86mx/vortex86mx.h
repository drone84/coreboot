#ifndef SOUTHBRIDGE_DMP_VORTEX86_H
#define SOUTHBRIDGE_DMP_VORTEX86_H

#define	SB			PCI_DEV(0, 7, 0)
#define	SB_REG_VID			0x00
#define	SB_REG_DID			0x02
#define	SB_REG_CMD			0x04
#define	SB_REG_STS			0x06
#define	SB_REG_RID			0x08
#define	SB_REG_PI			0x09
#define	SB_REG_HDR			0x0E
#define	SB_REG_SVID			0x2C
#define	SB_REG_SID			0x2E
#define	SB_REG_BSR			0x40
#define	SB_REG_LPCCR		0x41
#define	SB_REG_FRCSCR		0x42
#define	SB_REG_WTC			0x44
#define	SB_REG_GPIO_CTRL	0x48
#define	SB_REG_SMM_BAR		0x4C
#define	SB_REG_SERIRQ_CTRL	0x50
#define	SB_REG_LPC_CTRL		0x51
#define	SB_REG_COMP_DEC		0x52
#define	SB_REG_INTUART_CTRL	0x53
#define	SB_REG_INTUART_IO	0x54
#define	SB_REG_PIRQ_X_ROUT	0x58
#define	SB_REG_BSCR			0x5C
#define	SB_REG_GPIO_DPBA_1	0x60
#define	SB_REG_GPIO_DPBA_2	0x62
#define	SB_REG_GPIO_DPBA_3	0x64
#define	SB_REG_GPIO_DPBA_4	0x66
#define	SB_REG_GPIO_DPBA_5	0x68
#define	SB_REG_GPIO_DBA		0x6A
#define	SB_REG_GPIO_CNTL	0x6C
#define	SB_REG_RSR			0x6D
#define	SB_REG_RCR			0x6E
#define	SB_REG_LADR			0x70
#define	SB_REG_LADBAR1		0x74
#define	SB_REG_LADBAM1		0x78
#define	SB_REG_LADBAR2		0x7C
#define	SB_REG_LADBAM2		0x80
#define	SB_REG_CS_BASE2		0x88
#define	SB_REG_CS_BASE_MASK2	0x8C
#define	SB_REG_CS_BASE0		0x90
#define	SB_REG_CS_BASE_MASK0	0x94
#define	SB_REG_CS_BASE1		0x98
#define	SB_REG_CS_BASE_MASK1	0x9C
#define	SB_REG_INTUART2_IO	0xA0
#define	SB_REG_INTUART3_IO	0xA4
#define	SB_REG_INTUART4_IO	0xA8
#define	SB_REG_INTUART9_IO	0xAC
#define	SB_REG_IPPCR		0xB0
#define	SB_REG_PIRQ_X_ROUT2	0xB4
#define	SB_REG_OCDCR		0xBC
#define	SB_REG_IPFCR		0xC0
#define	SB_REG_FRWPR		0xC4
#define	SB_REG_IPFCR2		0xC8
#define	SB_REG_IPFCR3		0xCC
#define	SB_REG_STRAP		0xCE
#define	SB_REG_PM_BASE		0xF8
#define	SB_REG_ACPI_CTRL	0xFC
#define	SB_REG_SMI_BASE		0x4C


#define	WDT1_REG_CTRL	    0x68
#define	WDT1_REG_SIGN_SEL	0x69
#define	WDT1_REG_COUNTER0	0x6A
#define	WDT1_REG_COUNTER1  	0x6B
#define	WDT1_REG_COUNTER2  	0x6C
#define	SYSTEM_CTL_PORT     0x92

#endif	/* SOUTHBRIDGE_DMP_VORTEX86_H */
