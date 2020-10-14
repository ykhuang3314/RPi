/* SPI Pratice */

#include <string.h>
#include <bcm2835.h>
#include <stdio.h>
#include <stdbool.h>

// Config Instructions
#define	CMD_RST_EN			0x66
#define	CMD_RST_MEM			0x99
#define	CMD_READ_STATE_REG		0x05
#define	CMD_WRITE_STATE_REG		0x01
#define	CMD_WRITE_COFIG_REG		0x35

// Read instructions 
#define	CMD_READ_MEM			0x03

// Identification
#define	CMD_JEDEC_ID			0x9F

// Write instructions
#define CMD_WRITE_EN			0x06
#define CMD_WRITE_DISABLE		0x04
#define CMD_ERASE_FULL_ARRARY		0xC7
#define CMD_Sector_Erase		0x20
#define CMD_Chip_Erase			0xC7
#define CMD_Page_Program		0x02

// Protection
#define CMD_LBPR			0x8D
#define CMD_ULBPR 			0x98

void SST26VF016B_Read_JEDECID (uint8_t* d);
uint8_t SST26VF016B_Read_STATE_REG (void);
void SST26VF016B_Read_MEM(uint32_t Address, uint8_t* buf, uint16_t n);
void print_data ( uint8_t* data, uint16_t n);
void SST26VF016B_WRITE_ENABLE (void);
void SST26VF016B_WRITE_DISABLE (void);
bool SST26VF016B_IsBusy(void);
void SST26VF016B_4KByte_Sector_Erase (uint16_t sec_no, bool flagwait);
void SST26VF016B_Page_Program(uint16_t sec_no, uint16_t start_addr, uint8_t* wdata, uint16_t n);
void SST26VF016B_Unlock_Protection(void);
void SST26VF016B_Lock_Protection(void);

int main(int argc, char **argv)
{
    //Variabels
    uint16_t n = 128, wn = 26;
    uint8_t buf[n];
    uint8_t jedecid[3];
    uint8_t wdata[wn];
    uint16_t wsec_no, init_address;
    int i;
    
    // SPI initial & Settings
    if (!bcm2835_init())
    {
	printf("bcm2835_init failed.\n");
	return 1;
    }
    else
	printf("bcm2835_init succeeded.\n");
     
    
    if (!bcm2835_spi_begin())
    {
	printf("bcm2835_spi_begin failed.\n");
	return 1;
    }
    else
	printf("bcm2835_spi_begin succeeded.\n");
    
    bcm2835_spi_set_speed_hz(10000);
    //bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_16); // Set SPI Clock: 15.625MHz
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0); // Set SPI Mode: CPOL=0, CPHA=0;
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST); // Set Bit Order
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW); // Set CS Polarity
    
    printf("\n---------------------------------------------------------------\n");
    //Read JEDEC-ID
    SST26VF016B_Read_JEDECID (jedecid);
    printf("JEDEC-ID: ");
    for (i=0; i<3; i++){
	printf("%02x ", jedecid[i]);
    }
    printf("\n---------------------------------------------------------------\n");
    
    //Read Status
    buf[0] = SST26VF016B_Read_STATE_REG();
    printf ("Status Register: %02x ",buf[0]);
    printf("\n---------------------------------------------------------------\n");
    
    //Global block protection unlock
    printf("Unlock global block protection\n");
    //buf[0] = SST26VF016B_Read_STATE_REG();
    //printf("Enable writing...\n");
    //printf ("Status Register: %02x ",buf[0]);
    //printf("\n---------------------------------------------------------------\n");
    SST26VF016B_Unlock_Protection();
    buf[0] = SST26VF016B_Read_STATE_REG();
    printf ("Check status Register after unlock: %02x ",buf[0]);
    printf("\n---------------------------------------------------------------\n");
 
    // Erase sector before writing data
    wsec_no = 0;
    printf("Erase sector => Sector Number: %d\n",wsec_no);
    SST26VF016B_4KByte_Sector_Erase (0,true);
    
    
    //Read Data
    printf("Read memory after erasing one sector\n");
    memset(buf,0,n);
    SST26VF016B_Read_MEM(0, buf, n);
    print_data(buf, n);
    
    // data to be written
    for (i=0; i<wn; i++){
	wdata[i] = 'A'+i;
    }
    printf("data value to be written \n");
    print_data(wdata,wn);
    init_address = 0;
    printf("\nPage Program => Sector Number: %d, Initial Address: %05x, Data size: %d bytes\n", wsec_no, init_address, wn);
    SST26VF016B_Page_Program(wsec_no, init_address, wdata, wn);
    //Read Data after writing
    while(SST26VF016B_IsBusy());
    printf("Read memory after writing\n");
    memset(buf,0,n);
    SST26VF016B_Read_MEM(0, buf, n);
    print_data(buf, n);
    
    while(SST26VF016B_IsBusy());
    /*
    printf ("Execute WRDI(WRiteDIsable) instructions and lock block-protection REG\n");
    //Write Disable
    SST26VF016B_WRITE_DISABLE();
    while(SST26VF016B_IsBusy());
    */
    //Read Status
    buf[0] = SST26VF016B_Read_STATE_REG();
    printf ("Status Register: %02x \n",buf[0]);
    printf("---------------------------------------------------------------\n");
    bcm2835_spi_end();
    bcm2835_close();
    
    return 0;
    
}

void SST26VF016B_Read_JEDECID (uint8_t* d){
	char data[4];
	memset(data,0,sizeof(data));
	data[0] = CMD_JEDEC_ID;
	bcm2835_spi_transfern(data,sizeof(data));
	memcpy(d,&data[1],3);
} 

uint8_t SST26VF016B_Read_STATE_REG (void){
	char data[2];
	data[0] = CMD_READ_STATE_REG;
	bcm2835_spi_transfern(data,sizeof(data));
	return data[1];
}

void SST26VF016B_Read_MEM(uint32_t Address, uint8_t* buf, uint16_t n){
	char *data;
	data = (char*)malloc(n+4);
	data[0] = CMD_READ_MEM;
	data[1] = (Address >> 16) & 0xFF;
	data[2] = (Address >> 8) & 0xFF;
	data[3] = Address & 0xFF;
	bcm2835_spi_transfern(data,n+4);	
	memcpy(buf,&data[4],n);
	free(data);
}


void print_data ( uint8_t* data, uint16_t n){
	uint8_t colum = 0;
	uint32_t start_add = 0, end_add = n-1;
	printf("Size: %d, Strat Address:%05x, End Address: %05x\n", n, start_add, end_add);
	printf("---------------------------------------------------------------\n");
	for (uint8_t address = start_add; address <= end_add ; address++){
		if (colum ==0){
		    printf("%05x  ", address);
		}
		printf("%02x ", data[address]);
		colum +=1;
		if (colum == 16){
		    printf("\n");
		    colum = 0;
		}
	}
	printf("\n---------------------------------------------------------------\n");
}

void SST26VF016B_WRITE_ENABLE (void){
	char data[1];
	data[0] = CMD_WRITE_EN;
	bcm2835_spi_transfern(data,sizeof(data));
}

void SST26VF016B_WRITE_DISABLE (void){
	char data;
	data = CMD_WRITE_DISABLE;
	bcm2835_spi_transfer(data);
}

bool SST26VF016B_IsBusy(void){
	char data[2];
	data[0] = CMD_READ_STATE_REG;
	bcm2835_spi_transfern(data,sizeof(data));
	uint8_t status_REG = data[1];
	if (status_REG & 0x01) return true;
	return false;
}

void SST26VF016B_4KByte_Sector_Erase (uint16_t sec_no, bool flagwait){
	char data[4];
	uint8_t tmp;
	uint16_t Address = sec_no; 
	Address = Address << 12; //4Kbyte sector
	data[0] = CMD_Sector_Erase;
	data[1] = (Address >> 16) & 0xFF;
	data[2] = (Address >> 8) & 0xFF;
	data[3] = Address & 0xFF;
	//unlock protection
	//SST26VF016B_Unlock_Protection();
	// Write enable
	SST26VF016B_WRITE_ENABLE();
	printf ("Check status REG...\n");
	//Read Status
	tmp = SST26VF016B_Read_STATE_REG();
	printf ("Status Register: %02x \n",tmp);
	bcm2835_spi_transfern(data,sizeof(data));
	while(SST26VF016B_IsBusy() & flagwait){
	    delay(10);
	}
}

void SST26VF016B_Page_Program(uint16_t sec_no, uint16_t start_addr, uint8_t* wdata, uint16_t n){
	char *data;
	uint8_t tmp;
	data = (char*)malloc(n+4);
	uint16_t Address = sec_no; 
	Address <<= 12; //4Kbyte sector
	Address += start_addr;
	data[0] = CMD_Page_Program;
	data[1] = (Address >> 16) & 0xFF;
	data[2] = (Address >> 8) & 0xFF;
	data[3] = Address & 0xFF;
	memcpy(&data[4], wdata, n);
	//unlock protection
	//SST26VF016B_Unlock_Protection();
	// Write enable
	SST26VF016B_WRITE_ENABLE();
	printf ("Check the status REG...\n");
	//Read Status
	tmp = SST26VF016B_Read_STATE_REG();
	printf ("Status Register: %02x \n",tmp);
	if(tmp & 0x02){
		printf ("Device is write-enabled and start writing data.\n");
		bcm2835_spi_transfern(data,n+4);
	}
	else{
	    printf("Device is not write-enabled. Fail to writed data\n");
	}
	while(SST26VF016B_IsBusy());
	free(data);
	
} 

void SST26VF016B_Unlock_Protection(void){
    	char data[1];
	data[0] = CMD_ULBPR;
	SST26VF016B_WRITE_ENABLE();
	bcm2835_spi_transfern(data,sizeof(data));
	while(SST26VF016B_IsBusy());
}

void SST26VF016B_Lock_Protection(void){
    	char data[1];
	data[0] = CMD_LBPR;
	SST26VF016B_WRITE_ENABLE();
	bcm2835_spi_transfern(data,sizeof(data));
	while(SST26VF016B_IsBusy());
}
