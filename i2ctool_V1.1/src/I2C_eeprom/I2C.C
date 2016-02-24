#include <stdio.h>
#include <conio.h>
#include <graph.h>


//#include <i86.h>

// only define one of the following
//#define USEMEMCPY


/* This program reads/writes registers from I2C and is independent of BIOS */
/* taken from Ray's code originally and modified to read any register on I2C */

#define TRUE 1
#define FALSE 0

typedef unsigned char BYTE;

#define BIT0    0x01
#define BIT1    0x02
#define BIT2    0x04
#define BIT3    0x08
#define BIT4    0x10
#define BIT5    0x20
#define BIT6    0x40
#define BIT7    0x80

// Serial Port Enable
#define S3_SPC_ENABLE   BIT4
#define S3_SCL_ENABLE   BIT4
#define S3_SDA_ENABLE   BIT4
#define S3_SCL_READ     BIT2
#define S3_SDA_READ     BIT3
#define S3_SCL_WRITE    BIT0
#define S3_SDA_WRITE    BIT1

#define UMA_SPC_ENABLE  BIT0
#define UMA_SCL_ENABLE  BIT3
#define UMA_SDA_ENABLE  BIT2
#define UMA_SCL_READ    BIT3
#define UMA_SDA_READ    BIT2
#define UMA_SCL_WRITE   BIT5
#define UMA_SDA_WRITE   BIT4

#define UMA_GSCL_ENABLE BIT7
#define UMA_GSDA_ENABLE BIT6
#define UMA_GSCL_READ   BIT3
#define UMA_GSDA_READ   BIT2
#define UMA_GSCL_WRITE  BIT5
#define UMA_GSDA_WRITE  BIT4
#define UMA_GSW_ENABLE  BIT1

void delay(unsigned i);
void i2c_start(void);
void i2c_stop(void);
void i2c_write (unsigned addr);
int ack_read(void);
int i2c_read(void);

BYTE SCL_WL = 0;    
BYTE SCL_WH = 0;    
BYTE SDA_WL = 0;    
BYTE SDA_WH = 0; 
BYTE SDA_BIT = 0;
BYTE SDA_SHIFT = 0;
BYTE SCL_BIT = 0;
BYTE SCL_SHIFT = 0;
BYTE SDA_R = 0;
unsigned addr;
unsigned data;
unsigned temp;
unsigned subaddr;
unsigned serial;
unsigned bUsingGPIO;
BYTE isS3=FALSE;

void crWrite(unsigned index, unsigned value)
{
  outp(0x3d4,index);
  outp(0x3d5,value);
}

unsigned crRead(unsigned index)
{
  outp(0x3d4,index); 
  return(inp(0x3d5));
}

void srWrite(unsigned index, unsigned value)
{
  outp(0x3c4,index); 
  outp(0x3c5,value);
}

unsigned srRead(unsigned index)
{
  outp(0x3c4,index);
  return(inp(0x3c5));
}


void CLE_chipUnlock(void)
{
  srWrite(0x10, 0x01);
}


int read_data(unsigned addr)
{
    int data;
    //printf("reading addr: %x\n", addr);
    i2c_start();        // start bit

    i2c_write(subaddr);    // slave address & bit 0 = 0 is for write mode
    if(ack_read()==FALSE) return FALSE;   // wait for ack
                                  //debug
    
    i2c_write(addr);    // write addr
    if(ack_read()==FALSE) return FALSE;   // wait for ack

    i2c_start();        // start bit
    i2c_write(subaddr+1);    // slave address & bit 0 = 1 is for read
//    bit_write(1);       // read mode for the data
    if(ack_read()==FALSE) return FALSE;   // wait for ack
//    bit_write(1); //testing only
  
    data = i2c_read();

    nack_write();
    //if(ack_read()==FALSE) printf("Master Involved\n");   // wait for ack
    i2c_stop();
    return data;

}

int write_data(unsigned addr, unsigned data)
{
    i2c_start();
    i2c_write(subaddr);
    if(ack_read()==FALSE) return FALSE;
    i2c_write(addr);
    if(ack_read()==FALSE) return FALSE;
    i2c_write(data);
    if(ack_read()==FALSE) return FALSE;
    i2c_stop();
    return data;
}

void i2c_stop()
{
    if (!isS3)
    {
        srWrite(serial, SCL_WL|SDA_WL); //SCL_L_SDA_L); //0x01);
        delay(1);
        srWrite(serial, SCL_WH|SDA_WL); //SCL_H_SDA_L); //0x21);
        delay(1);
        srWrite(serial, SCL_WH|SDA_WH); //0x31);
        delay(1);
    }
    else
    {
        crWrite(serial, SCL_WL|SDA_WL); //SCL_L_SDA_L); //0x01);
        delay(1);
        crWrite(serial, SCL_WH|SDA_WL); //SCL_H_SDA_L); //0x21);
        delay(1);
        crWrite(serial, SCL_WH|SDA_WH); //0x31);
        delay(1);
    }
}

int i2c_read()
{
    int i, data=0;
    for (i=0; i<8; i++)
    {
        if (!isS3)
        {
            srWrite (serial, SCL_WL|SDA_R); // 0x80);
            delay(1);
            srWrite (serial, SCL_WH|SDA_R); // 0xA0);
            delay(1);
            data = (data<<1) + (0x1 & (srRead(serial)>>SDA_SHIFT));
            srWrite (serial, SCL_WL|SDA_R); //0x80);
            delay(1);
        }
        else
        {
            crWrite (serial, SCL_WL|SDA_R); // 0x80);
            delay(1);
            crWrite (serial, SCL_WH|SDA_R); // 0xA0);
            delay(1);
            data = (data<<1) + (0x1 & (crRead(serial)>>SDA_SHIFT));
            crWrite (serial, SCL_WL|SDA_R); //0x80);
            delay(1);
        }
    }
    return data;
}

int ack_read()
{
    int ack;

    if (!isS3)
    {
        srWrite(serial, SCL_WL|SDA_R); // 0x11);
        delay(1);
        srWrite(serial, SCL_WH|SDA_R); // 0x31);
        delay(1);
     
        ack = (SDA_BIT & (srRead(serial))); //(0x4 & (srRead(serial)));
        //debug printf("\nAck = %x", ack);
        srWrite(serial, SCL_WL|SDA_R); //0x11);
        delay(4);
    }
    else
    {
        crWrite(serial, SCL_WL|SDA_R); // 0x11);
        delay(1);
        crWrite(serial, SCL_WH|SDA_R); // 0x31);
        delay(1);
     
        ack = (SDA_BIT & (crRead(serial))); //(0x4 & (srRead(serial)));
        //debug printf("\nAck = %x", ack);
        crWrite(serial, SCL_WL|SDA_R); //0x11);
        delay(4);
    }

    if (ack==0) 
        return TRUE;
    else 
        return FALSE;
}

int nack_write()
{
    int ack;

    if (!isS3)
    {
        srWrite(serial, SCL_WL|SDA_R); // 0x11);
        delay(1);
        srWrite(serial, SCL_WH|SDA_R); // 0x31);
        delay(1);
     
        //debug printf("\nAck = %x", ack);
        srWrite(serial, SCL_WL|SDA_R); //0x11);
        delay(4);
    }
    else
    {
        crWrite(serial, SCL_WL|SDA_R); // 0x11);
        delay(1);
        crWrite(serial, SCL_WH|SDA_R); // 0x31);
        delay(1);
             
        crWrite(serial, SCL_WL|SDA_R); //0x11);
        delay(4);
    }

    if (ack==0) 
        return TRUE;
    else 
        return FALSE;
}


void i2c_start()
{
    if (!isS3)
    {
        srWrite(serial, SCL_WH|SDA_WH); //0x31);
        delay(1);
        srWrite(serial, SCL_WH|SDA_WL); //0x21);
        delay(1);
        srWrite(serial, SCL_WL|SDA_WL); //0x01);
        delay(1);
    }
    else
    {   
        crWrite(serial, SCL_WH|SDA_WH); //0x31);
        delay(1);
        crWrite(serial, SCL_WH|SDA_WL); //0x21);
        delay(1);
        crWrite(serial, SCL_WL|SDA_WL); //0x01);
        delay(1);
    }
}

void delay(unsigned i)
{

    i = i * 200;
    for (i; i>0 ;i--)
    {
        inp(0x100);
    }
}
        

void i2c_write (unsigned addr)
{
    int i, value;
    if (!isS3)
    {
        for (i=7; i>=0;i--)
        {
            value= (0x1 & (addr>>i));

            //   printf("value=%x, i=%x, addr=%x\n", value, i, addr);            //debug
            if (value ==0)
            {
                srWrite(serial, SCL_WL|SDA_WL); // 0x01);
                delay(1);
                srWrite(serial, SCL_WH|SDA_WL); // 0x21);
                delay(1);
                srWrite(serial, SCL_WL|SDA_WL); // 0x01);
                delay(1);
            }
            else 
            {
                srWrite(serial, SCL_WL|SDA_WH); // 0x11);
                delay(1);
                srWrite(serial, SCL_WH|SDA_WH); // 0x31);
                delay(1);
                srWrite(serial, SCL_WL|SDA_WH); // 0x11);
                delay(1);
            }
        }
    }
    else
    {
        for (i=7; i>=0;i--)
        {
            value= (0x1 & (addr>>i));

            //   printf("value=%x, i=%x, addr=%x\n", value, i, addr);            //debug
            if (value ==0)
            {
                crWrite(serial, SCL_WL|SDA_WL); // 0x01);
                delay(1);
                crWrite(serial, SCL_WH|SDA_WL); // 0x21);
                delay(1);
                crWrite(serial, SCL_WL|SDA_WL); // 0x01);
                delay(1);
            }
            else 
            {
                crWrite(serial, SCL_WL|SDA_WH); // 0x11);
                delay(1);
                crWrite(serial, SCL_WH|SDA_WH); // 0x31);
                delay(1);
                crWrite(serial, SCL_WL|SDA_WH); // 0x11);
                delay(1);
            }
        }
    }
}
void main(int argc, char *argv[])
{
    int i, j;
    int count=0;
    char sum=0x0;
    char sum1=0x0;
    unsigned char ch;
    bUsingGPIO = FALSE;

    

    if (argc >= 2)
    {
        //sscanf(argv[1], "%x", &serial);
        serial=0x31;
        switch ( serial )
        {
            //GPIO port registers
            case 0x25:
            case 0x2c:
            case 0x3d:
                 bUsingGPIO = TRUE;
                 isS3 = FALSE;
                break;
            //I2C port registers
            case 0x26:
            case 0x31:
                 bUsingGPIO = FALSE;
                 isS3 = FALSE;
                break;
            // for S3 chip
            case 0xA0:
            case 0xB1:
            case 0xAA:
                 bUsingGPIO = FALSE;
                 isS3 = TRUE;
                 break;
            default:
                 bUsingGPIO = FALSE;
                 isS3 = FALSE;
                 break;
        }

        if (isS3)
        {
            // S3 Chip
            SCL_WL      = S3_SCL_ENABLE|0;
            SCL_WH      = S3_SCL_ENABLE|S3_SCL_WRITE;
            SDA_WL      = S3_SDA_ENABLE|0;
            SDA_WH      = S3_SDA_ENABLE|S3_SDA_WRITE;
            SDA_R       = S3_SDA_ENABLE|S3_SDA_WRITE;
            SDA_BIT     = S3_SDA_READ;
            SDA_SHIFT   = 3;
            SCL_BIT     = S3_SCL_READ;
            SCL_SHIFT   = 2;
        }
        else
        {
            // UMA Chip
            if(bUsingGPIO)
            {
                SCL_WL      = UMA_GSCL_ENABLE|UMA_GSW_ENABLE|0;
                SCL_WH      = UMA_GSCL_ENABLE|UMA_GSW_ENABLE|UMA_GSCL_WRITE;
                SDA_WL      = UMA_GSDA_ENABLE|UMA_GSW_ENABLE|0;
                SDA_WH      = UMA_GSDA_ENABLE|UMA_GSW_ENABLE|UMA_GSDA_WRITE;
                SDA_R       = 0;
                SDA_BIT     = UMA_GSDA_READ;
                SDA_SHIFT   = 2;
                SCL_BIT     = UMA_GSCL_READ;
                SCL_SHIFT   = 3;
            }
            else
            {
                SCL_WL      = UMA_SPC_ENABLE|0;    
                SCL_WH      = UMA_SPC_ENABLE|UMA_SCL_WRITE;    
                SDA_WL      = UMA_SPC_ENABLE|0;    
                SDA_WH      = UMA_SPC_ENABLE|UMA_SDA_WRITE; 
                SDA_R       = UMA_SPC_ENABLE|UMA_SDA_WRITE;
                SDA_BIT     = UMA_SDA_READ;
                SDA_SHIFT   = 2;
                SCL_BIT     = UMA_SCL_READ;
                SCL_SHIFT   = 3;
            }
        }
    }

    // debug config value
#if 0    
    printf("S3 Chip? %02x\n", isS3);
    printf("SCL_WL %02x\n", SCL_WL);
    printf("SCL_WH %02x\n", SCL_WH);
    printf("SDA_WL %02x\n", SDA_WL);
    printf("SDA_WH %02x\n", SDA_WH);
    printf("SDA_R  %02x\n", SDA_R);
    printf("SDA_BIT %02x\n", SDA_BIT);
    printf("SDA_SHIFT %02x\n", SDA_SHIFT);
    printf("SCL_BIT %02x\n", SCL_BIT);
    printf("SCL_SHIFT %02x\n", SCL_SHIFT);
#endif

  if (argc == 3)
  {
        
        FILE *file;
        char buf[256];
        i=0x00;
        
        serial=0x31; //DVPSPD port0x31h need to modify for different board
        sscanf(argv[1], "%x", &subaddr); //slave address
        //subaddr=0xa8;  //Slave address
        
        if(argc != 3)
        { 
        printf("command: i2ctool slaveaddr <filename>\n");
        return;
        } 

        file = fopen(argv[2], "rb");
        if(!file)
        { 
        printf("command: i2ctool slaveaddr <filename>\n");
        return; 
        }

        printf("=======================================================\n");
        printf("Start to Write data to EEPROM...\n");
      
            while(1)
            {
                fread(&ch, sizeof(char), 1, file);
                if(feof(file)) break;
        
                buf[i] = ch;
                sum1 += buf[i];
                
                write_data(addr+i, buf[i]);
                delay(100);
        
                if(count > 15)
                {  
                   
                    count = 0; 
                } 
                
                count++; 
                i++;
            } 

        fclose(file);
        printf("Writing success!!!\n");     

        // show all register
        printf("Start to Read data from EEPROM...\n");
        printf("I2C 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
        printf("---------------------------------------------------\n");
        for (j=0; j<0x10; j++)
        {
            printf("%02x ",j);
            for(i=0; i<0x10; i++)
                {
                    addr = j*0x10 + i;
                    temp = read_data(addr);
                    //if (j <= 7)
                    sum += temp;    // do 128 bytes check sum
                    printf(" %02x", temp);
                }
            printf("\n");
        }

        printf("Check Sum form file   = %02x\n", sum1);
        printf("Check Sum from eeprom = %02x\n", sum);
        
        if(sum1 == sum )
        {
            printf("Check Sum PASSed!!!\n");
            return;
        }
        
        printf("Check Sum failed...\n");
        return;            
         
  }
  
  else if (argc > 4 || argc <3)
  {
    printf("I2C ver 1.1\n");
    printf("This program writes binart file from I2C independently of BIOS\n\n");
    printf("command: i2ctool slaveaddr <filename>\n");
    printf("         slaveaddr      = slaveaddr \n");
    printf("         <filename>     = binary.bin \n");
    printf("examples:\n");
    printf(" i2ctool 0xa8 file.bin -- Write file.bin to eeporm\n");
    return;
  }
  
  sscanf(argv[1], "%x", &serial);
  sscanf(argv[2], "%x", &subaddr);
  sscanf(argv[3], "%x", &addr);
  sscanf(argv[4], "%x", &data);

}
