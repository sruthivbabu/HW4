#include<xc.h>           // processor SFR definitions
#include<math.h>         //include math functions
#include<sys/attribs.h>  // __ISR macro

// DEVCFG0
#pragma config DEBUG = OFF // no debugging
#pragma config JTAGEN = OFF // no jtag
#pragma config ICESEL =  ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // no write protect
#pragma config BWP = OFF // no boot write protect
#pragma config CP = OFF // no code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF // turn off secondary oscillator
#pragma config IESO = OFF // no switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // disable secondary osc
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // do not enable clock switch
#pragma config WDTPS = PS1048576 // use slowest wdt
#pragma config WINDIS = OFF // wdt no window mode
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz
#pragma config UPLLIDIV = DIV_2 // divider for the 8MHz input clock, then multiplied by 12 to get 48MHz for USB
#pragma config UPLLEN = ON // USB clock on

// DEVCFG3
#pragma config USERID = 00000000 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations
#pragma config FUSBIDIO = ON // USB pins controlled by USB module
#pragma config FVBUSONIO = ON // USB BUSON controlled by USB module

#define CS LATAbits.LATA0       // chip select pin

// send a byte via spi and return the response
unsigned char spi_io(unsigned char o) {
  SPI1BUF = o;
  while(!SPI1STATbits.SPIRBF) { // wait to receive the byte
    ;
  }
  return SPI1BUF;
}

void setVoltage(char a, int v) {
	unsigned short t;
	t= a << 15; //a is at the very end of the data transfer
	t = t | 0b0111000000000000; //setting the values for the configuration
	t = t | ((v&0b111111111111)); 
	
	CS = 0;
	spi_io(t>>8);
	spi_io(t&0xFF);
    CS=1;
	
}
void initSPI1(){
// the chip select pin is used by the sram to indicate when a command is beginning (clear CS to low) and when it is ending (set CS high)
  TRISAbits.TRISA0 = 0; // set up the chip select pin as an output
  CS = 1; //no communication
  // setup spi1
  RPA1Rbits.RPA1R = 0b0011; //set pin A1 as output SDO1
  SPI1CON = 0;              // turn off the spi module and reset it
  SPI1BUF;                  // clear the rx buffer by reading from it
  SPI1BRG = 03;             // baud rate to 10 MHz [SPI1BRG = (80000000/(2*desired))-1]
  SPI1STATbits.SPIROV = 0;  // clear the overflow bit
  SPI1CONbits.CKE = 1;      // data changes when clock goes from hi to lo (since CKP is 0)
  SPI1CONbits.MSTEN = 1;    // master operation
  SPI1CONbits.ON = 1;       // turn on spi 1
}
int main() {

    __builtin_disable_interrupts();

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    //set input and output pins
    TRISBbits.TRISB4=1; //pin B4 is an input
    TRISAbits.TRISA4=0; //pin A4 is an output
    LATAbits.LATA4=1; //set the initial output of pin A4 to high (3.3V))

    __builtin_enable_interrupts();
    initSPI1();    
    _CP0_SET_COUNT(0);
    
        double a=0;
        double x=0;
        double dx = 2*10*(3.1412/1000);
        float b=0;
        float db=40.95;
        
    while(1) {
	
        setVoltage(0,a); //channel A set to 1.65V
        
        setVoltage(1,b); //channel B set to 0.825V     
        
        a = 4096/2 + 4096/2*sin(x);
        x=x+dx;
        b=b+db;
        if (b>=4095){//when reached top of triangle start going down
            db=-db;
        }
        if (b<=0){ //when reached bottom of triangle start going back up
            db=-db;
        }
        
        _CP0_SET_COUNT(0);
         while(_CP0_GET_COUNT()< 24000)
         {};
    }
    }