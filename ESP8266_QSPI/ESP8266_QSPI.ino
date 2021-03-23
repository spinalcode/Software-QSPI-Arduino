
#define SIO0 4  // MOSI in SPI mode
#define SIO1 5  // MISO in SPI mode
#define SIO2 9  // Needs to be HIGH in SPI mode
#define SIO3 10 // Needs to be HIGH in SPI mode
#define CLOCK 0
#define CS 12

#define BYTEMODE 0
#define SEQMODE  0x40
#define PAGEMODE 0x80
#define DUALMODE 0x3B
#define QUADMODE 0x38
#define SPIMODE  0xFF

#define SET_PIN(x) WRITE_PERI_REG( 0x60000304, (1<<x) )
#define CLR_PIN(x) WRITE_PERI_REG( 0x60000308, (1<<x) )
#define GET_PIN(x) (((*((volatile uint32_t *)(0x60000318))) >> (x))&1)
#define FLIP_PIN(x) (*((volatile uint32_t *)(0x60000300))) ^= (1<<x)

#define GET_TWO_PINS(x) (((*((volatile uint32_t *)(0x60000318))) >> (x))&3)

void hw_wdt_disable(){
  *((volatile uint32_t*) 0x60000900) &= ~(1); // Hardware WDT OFF
}

void hw_wdt_enable(){
  *((volatile uint32_t*) 0x60000900) |= 1; // Hardware WDT ON
}

void setReadMode(){
  pinMode(SIO0, INPUT);
  pinMode(SIO1, INPUT);
  pinMode(SIO2, INPUT);
  pinMode(SIO3, INPUT);
}

void setWriteMode(){
  pinMode(SIO0, OUTPUT);
  pinMode(SIO1, OUTPUT);
  pinMode(SIO2, OUTPUT);
  pinMode(SIO3, OUTPUT);
}

int spi_write(int value){
  uint8_t myByte = 0;
  for (int bit = 7; bit >= 0; --bit){
    if((value >> bit) & 0x01){ // Set MOSI Value
      SET_PIN(SIO0);
    }else{
      CLR_PIN(SIO0);
    }
    myByte |= (GET_PIN(SIO1) << bit); // Read MISO value
    FLIP_PIN(CLOCK);
    FLIP_PIN(CLOCK);
  }
  return myByte;
}

inline void writeQuad(uint8_t value){
  (*((volatile uint32_t *)(0x60000300))) &= 0b1111100111001111;
  (*((volatile uint32_t *)(0x60000300))) |= (((value&0b11000000)<<3)|(value&0b00110000));

  //if((value>>4)&1)SET_PIN(SIO0); else CLR_PIN(SIO0);
  //if((value>>5)&1)SET_PIN(SIO1); else CLR_PIN(SIO1);
  //if((value>>6)&1)SET_PIN(SIO2); else CLR_PIN(SIO2);
  //if((value>>7)&1)SET_PIN(SIO3); else CLR_PIN(SIO3);
  SET_PIN(CLOCK);
  CLR_PIN(CLOCK);

  (*((volatile uint32_t *)(0x60000300))) &= 0b1111100111001111;
  (*((volatile uint32_t *)(0x60000300))) |= (((value&0b00001100)<<7)|(value&0b00000011)<<4);
  
  //if((value)&1)SET_PIN(SIO0); else CLR_PIN(SIO0);
  //if((value>>1)&1)SET_PIN(SIO1); else CLR_PIN(SIO1);
  //if((value>>2)&1)SET_PIN(SIO2); else CLR_PIN(SIO2);
  //if((value>>3)&1)SET_PIN(SIO3); else CLR_PIN(SIO3);
  SET_PIN(CLOCK);
  CLR_PIN(CLOCK);
}
void readQuad(uint8_t* buffer, uint32_t number){
  unsigned int temp=0;
  int t= -number;
  buffer+=number;
  for(; t;){
    SET_PIN(CLOCK);
//    temp = (GET_PIN(SIO0) << 4) | (GET_PIN(SIO1) << 5) | (GET_PIN(SIO2) << 6) | (GET_PIN(SIO3) << 7);
    temp = (GET_TWO_PINS(SIO2) << 6) | (GET_TWO_PINS(SIO0) << 4);
    CLR_PIN(CLOCK);
    SET_PIN(CLOCK);
//    temp |= ((GET_PIN(SIO3) << 3) | (GET_PIN(SIO2) << 2) | (GET_PIN(SIO1) << 1) | GET_PIN(SIO0));
    temp |= (GET_TWO_PINS(SIO2) << 2) | GET_TWO_PINS(SIO0);
    CLR_PIN(CLOCK);
    buffer[t++] = temp;
  }
}

void writeToAddressQuad(uint32_t address, const uint8_t* buffer, uint32_t number){
  CLR_PIN(CS);
  uint8_t temp;
    
  // max address in 128k RAM is 131071 or 000000011111111111111111 or 1FFFF
  address &= 0x1FFFF;

  setWriteMode();
    
  // 1 byte for the command, sent in 2 clock ticks
  writeQuad(0x02); // write command

  temp = (address >> 16) & 255;
  writeQuad(temp);
  temp = (address >> 8) & 255;
  writeQuad(temp);
  temp = address & 255;
  writeQuad(temp);
    
  // data sent in number*2 clock ticks
  for(int t=0; t<number; t++){
    writeQuad(buffer[t]);
  }
    
  SET_PIN(CS);
}

void readFromAddressQuad(uint32_t address, uint8_t* buffer, uint32_t number){
  CLR_PIN(CLOCK); // just in case
  CLR_PIN(CS);
  uint8_t temp;
  setWriteMode();

  writeQuad(0x03); // read command

  temp = (address >> 16) & 255;
  writeQuad(temp);
  temp = (address >> 8) & 255;
  writeQuad(temp);
  temp = address & 255;
  writeQuad(temp);
  
  setReadMode();

  // pretend to read the first byte, its a dummy
  FLIP_PIN(CLOCK);
  FLIP_PIN(CLOCK);
  FLIP_PIN(CLOCK);
  FLIP_PIN(CLOCK);

  readQuad(&buffer[0], number);

  SET_PIN(CS);
}

void setProtocol(uint8_t prot){
  CLR_PIN(CS);
  spi_write(prot);
  SET_PIN(CS);
}

void setMode(uint8_t mode){
  switch(mode){
    case BYTEMODE:
    case PAGEMODE:
    case SEQMODE:
      CLR_PIN(CS);
      spi_write(0x01);
      spi_write(mode);
      SET_PIN(CS);
      break;
  }
}

void writeToAddress(uint32_t address, const uint8_t* buffer, uint32_t number){
  CLR_PIN(CS);
  spi_write(0x02);
  uint8_t temp = address >> 16;
  spi_write(temp);
  temp = address >> 8;
  spi_write(temp);
  temp = address & 255;
  spi_write(temp);
  for(int t=0; t<number; t++){
    spi_write(buffer[t]);
  }
  SET_PIN(CS);
}

void readFromAddress(uint32_t address, uint8_t* buffer, uint32_t number){
  CLR_PIN(CS);
  spi_write(0x03);
  uint8_t temp = address >> 16;
  spi_write(temp);
  temp = address >> 8;
  spi_write(temp);
  temp = address & 255;
  spi_write(temp);
  for(int t=0; t<number; t++){
    buffer[t] = spi_write(0x00); // sending dummy bytes will also read the MISO
  }
  SET_PIN(CS);
}

void initRAM(){
  // these pins need to be held high when using standard SPI mode
  SET_PIN(SIO2);
  SET_PIN(SIO3);
  // set clock pin to output
  pinMode(CLOCK, OUTPUT);
  // set mosi pin to output
  pinMode(SIO0, OUTPUT);
  // set msio pin to input
  pinMode(SIO1, INPUT);
  // set CS pin to output
  pinMode(CS, OUTPUT);
  // these use normal software spi
  setMode(SEQMODE);
  setProtocol(QUADMODE);
}

void clearQuad(){
  setWriteMode();
  CLR_PIN(CS);
  writeQuad(0x02); // write command
  writeQuad(0); // First byte of address
  writeQuad(0); // Second byte of address
  writeQuad(0); // Third byte of address
  
  writeQuad(0); // send 0 as first byte and set registers to send 0's
  // Then just toggle the clock pin untill full RAM written to.
  for(int t = 0x1FFFF; t; --t){
    SET_PIN(CLOCK);
    CLR_PIN(CLOCK);
  }
  SET_PIN(CS);
}

void setup() {
  hw_wdt_disable();

  Serial.begin(115200);
  delay(500); // For everything to catch up

  Serial.println(" ");
  Serial.println(" ");
  Serial.println(" _______  _______  _______ _________     _______  _______  _______     _________ _______  _______ _________");
  Serial.println("(  ___  )(  ____ \\(  ____ )\\__   __/    (  ____ )(  ___  )(       )    \\__   __/(  ____ \\(  ____ \\\\__   __/");
  Serial.println("| (   ) || (    \\/| (    )|   ) (       | (    )|| (   ) || () () |       ) (   | (    \\/| (    \\/   ) (   ");
  Serial.println("| |   | || (_____ | (____)|   | |       | (____)|| (___) || || || |       | |   | (__    | (_____    | |   ");
  Serial.println("| |   | |(_____  )|  _____)   | |       |     __)|  ___  || |(_)| |       | |   |  __)   (_____  )   | |   ");
  Serial.println("| | /\\| |      ) || (         | |       | (\\ (   | (   ) || |   | |       | |   | (            ) |   | |   ");
  Serial.println("| (_\\ \\ |/\\____) || )      ___) (___    | ) \\ \\__| )   ( || )   ( |       | |   | (____/\\/\\____) |   | |   ");
  Serial.println("(____\\/_)\\_______)|/       \\_______/    |/   \\__/|/     \\||/     \\|       )_(   (_______/\\_______)   )_(   ");
     
  Serial.println("Starting...");  
  Serial.println("Init RAM...");  
  initRAM();

  int howMany = 8;
  uint8_t myArray[howMany];
  uint8_t myArray2[howMany];
 

  Serial.print("Sending - ");
  for(int t=0; t<howMany; t++){
    myArray[t]=random(255);
    Serial.print(myArray[t]);
    Serial.print(",");
  }


  Serial.println("");
  int startTime = millis();
  
  writeToAddressQuad(0, &myArray[0], howMany);
  int writeTime = millis();
  readFromAddressQuad(0, myArray2, howMany);
  int readTime = millis();

  Serial.print("Reading - ");
  for(int t=0; t<howMany; t++){
    Serial.print(myArray2[t]);
    Serial.print(",");
  }
  Serial.println("");
  Serial.println("OK?");

  Serial.print("Reading - ");
  for(int t=0; t<howMany; t++){
  }
  Serial.println("");
  Serial.println("OK?");

  Serial.print("Start Time = ");
  Serial.println(startTime);
  Serial.print("Write Time = ");
  Serial.println(writeTime);
  Serial.print("Read Time = ");
  Serial.println(readTime);

  setWriteMode();
}

void loop() {
// Holding pattern for easy viewing
  SET_PIN(SIO0); CLR_PIN(SIO0);
  SET_PIN(SIO1); CLR_PIN(SIO1);
  SET_PIN(SIO2); CLR_PIN(SIO2);
  SET_PIN(SIO3); CLR_PIN(SIO3);

}
