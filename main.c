#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = OFF
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#include <xc.h>
#define _XTAL_FREQ 20000000

#define LCD_RS PORTDbits.RD0
#define LCD_RW PORTDbits.RD1
#define LCD_EN PORTDbits.RD7
#define LCD_D4 PORTDbits.RD3
#define LCD_D5 PORTDbits.RD4
#define LCD_D6 PORTDbits.RD5
#define LCD_D7 PORTDbits.RD6

#define DHT_DATA PORTBbits.RB4
#define DHT_TRIS TRISBbits.TRISB4

#define DHT_INPUT()  do{ DHT_TRIS = 1; __delay_us(10); }while(0)
#define DHT_OUTPUT() do{ DHT_TRIS = 0; __delay_us(20); }while(0)
#define DHT_LOW()    do{ DHT_DATA = 0; __delay_us(5); }while(0)
#define DHT_HIGH()   do{ DHT_DATA = 1; __delay_us(5); }while(0)

void LCD_Command(unsigned char c){
    LCD_RS = 0; LCD_RW = 0;
    LCD_D7 = (c & 0x80) ? 1 : 0;
    LCD_D6 = (c & 0x40) ? 1 : 0;
    LCD_D5 = (c & 0x20) ? 1 : 0;
    LCD_D4 = (c & 0x10) ? 1 : 0;
    LCD_EN = 1; __delay_us(50); LCD_EN = 0; __delay_ms(5);
    LCD_D7 = (c & 0x08) ? 1 : 0;
    LCD_D6 = (c & 0x04) ? 1 : 0;
    LCD_D5 = (c & 0x02) ? 1 : 0;
    LCD_D4 = (c & 0x01) ? 1 : 0;
    LCD_EN = 1; __delay_us(50); LCD_EN = 0; __delay_ms(5);
}

void LCD_Write(unsigned char data){
    LCD_RS = 1; LCD_RW = 0;
    LCD_D7 = (data & 0x80) ? 1 : 0;
    LCD_D6 = (data & 0x40) ? 1 : 0;
    LCD_D5 = (data & 0x20) ? 1 : 0;
    LCD_D4 = (data & 0x10) ? 1 : 0;
    LCD_EN = 1; __delay_us(50); LCD_EN = 0; __delay_ms(2);
    LCD_D7 = (data & 0x08) ? 1 : 0;
    LCD_D6 = (data & 0x04) ? 1 : 0;
    LCD_D5 = (data & 0x02) ? 1 : 0;
    LCD_D4 = (data & 0x01) ? 1 : 0;
    LCD_EN = 1; __delay_us(50); LCD_EN = 0; __delay_ms(2);
}

void LCD_Print(const char *txt){
    while(*txt){
        LCD_Write(*txt++);
        __delay_us(50);
    }
}

void LCD_Initialize(){
    __delay_ms(100);
    LCD_Command(0x02); __delay_ms(10);
    LCD_Command(0x28); __delay_ms(10);
    LCD_Command(0x0C); __delay_ms(10);
    LCD_Command(0x06); __delay_ms(10);
    LCD_Command(0x01); __delay_ms(20);
    LCD_Command(0x80); __delay_ms(10);
}
    
void LCD_Goto(unsigned char row, unsigned char col){
    unsigned char pos = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_Command(pos); __delay_ms(5);
}

void DHT_Start(){
    DHT_OUTPUT(); // Makes the DHT pin ready to send data to the sensor
    DHT_LOW(); // Pulls the DHT pin to 0 volts for signaling
    __delay_ms(20);  
    DHT_HIGH();      // Pulls the DHT pin to high voltage (logic 1) for signaling
    __delay_us(30);  
    DHT_INPUT(); // Makes the DHT pin ready to read data from the sensor
    __delay_us(40);
}

unsigned char DHT_Response(){
    unsigned int t = 0;
    // Wait for sensor to pull line low (increased timeout to 300)
    while(DHT_DATA == 1){ 
        __delay_us(1); 
        if(++t > 100) return 0;  // Increased from 100 to 300
    }
    
    t = 0;
    // Wait for sensor to release line (increased timeout to 300)
    while(DHT_DATA == 0){ 
        __delay_us(1); 
        if(++t > 100) return 0;  // Increased from 100 to 300
    }
    
    t = 0;
    // Wait for sensor to pull line low again (increased timeout to 300)
    while(DHT_DATA == 1){ 
        __delay_us(1); 
        if(++t > 300) return 0;  // Increased from 100 to 300
    }
    
    return 1;
}

unsigned char DHT_ReadByte(){
    unsigned char value = 0;
    for(unsigned char i = 0; i < 8; i++){
        // Wait for sensor to release line
        while(DHT_DATA == 0);
        
        // Wait 30us and check line state
        __delay_us(30);
        
        // If line is high, bit is 1
        if(DHT_DATA == 1) 
            value |= (1 << (7 - i));
        
        // Wait for sensor to pull line low (end of bit)
        while(DHT_DATA == 1);
    }
    return value;
}

void ByteToStr(unsigned char val, char* buffer){
    buffer[0] = (val / 10) + '0';
    buffer[1] = (val % 10) + '0';
    buffer[2] = '\0';
}

void main(void){
    unsigned char hum, temp, h_dec, t_dec, chk;
    char hum_txt[3], temp_txt[3];

    // Configure ports
    ADCON1 = 0x0F;      // All digital I/O
    TRISA = 0xFF;       // PORTA as input
    TRISB4 = 1;         // RB4 as input for DHT
    TRISCbits.TRISC0 = 0;  // RC0 as output
    TRISCbits.TRISC1 = 0;  // RC1 as output
    TRISD = 0x00;       // PORTD as output for LCD
    PORTC = 0x00;       // Clear PORTC

    // Initialize LCD
    LCD_Initialize();
    
    // Display startup message
    LCD_Goto(0, 0);
    LCD_Print("Wlcm Doctor");
    LCD_Goto(1, 0);
    LCD_Print("Mustafa");
    
    // Wait for DHT sensor to stabilize (IMPORTANT!)
    __delay_ms(2000);  // Added 2 second delay for sensor stabilization

    while(1){
        // Clear display before reading
        LCD_Command(0x01); 
        __delay_ms(5);
        
        // Start DHT communication
        DHT_Start();
        
        // Check if sensor responds
        if(DHT_Response()){
            // Read all 5 bytes from sensor
            hum = DHT_ReadByte();     // Humidity integer
            h_dec = DHT_ReadByte();   // Humidity decimal
            temp = DHT_ReadByte();    // Temperature integer
            t_dec = DHT_ReadByte();   // Temperature decimal
            chk = DHT_ReadByte();     // Checksum

            // Verify checksum
            if((hum + h_dec + temp + t_dec) == chk){
                // Display temperature
                LCD_Goto(0, 0);
                LCD_Print("Temp: ");
                ByteToStr(temp, temp_txt);
                LCD_Print(temp_txt);
                LCD_Print(" C");
                
                // Display humidity
                LCD_Goto(1, 0);
                LCD_Print("Hum: ");
                ByteToStr(hum, hum_txt);
                LCD_Print(hum_txt);
                LCD_Print(" %");

                // Control outputs based on temperature
                if(temp >= 24){
                    PORTCbits.RC0 = 1;  // High temp indicator
                    PORTCbits.RC1 = 0;
                } else {
                    PORTCbits.RC0 = 0;
                    PORTCbits.RC1 = 1;  // Normal temp indicator
                }
            } else {
                // Checksum error
                LCD_Goto(0, 0);
                LCD_Print("Checksum Error");
                LCD_Goto(1, 0);
                LCD_Print("Check wiring");
                PORTC = 0x00;
            }
        } else {
            // No response from sensor
            LCD_Goto(0, 0);
            LCD_Print("No Response");
            LCD_Goto(1, 0);
            LCD_Print("Check sensor");
            PORTC = 0x00;
        }

        // Ensure data line is input before next reading
        DHT_INPUT();
        
        // Wait before next reading (DHT needs minimum 2 seconds between readings)
        __delay_ms(3000);  // 3 second delay
    }
}