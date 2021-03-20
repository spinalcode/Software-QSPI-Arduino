#define D1 5
#define D2 4
#define D3 0
#define D5 14
#define SK 10
#define S3 9

#define SIO0 D1 // MOSI
#define SIO1 D2 // MISO
#define SIO2 D3
#define SIO3 D5
#define CLOCK SK
#define CS S3

#define BYTEMODE 0
#define SEQMODE  0x40
#define PAGEMODE 0x80
#define DUALMODE 0x3B
#define QUADMODE 0x38
#define SPIMODE  0xFF

#define CLR_PIN(x) GPOC = (1 << x)
#define SET_PIN(x) GPOS = (1 << x)
#define GET_PIN(x) (((GPI) >> (x))&1)
#define FLIP_PIN(x) if(GET_PIN(x)){CLR_PIN(x);}else{SET_PIN(x);}
//#define FLIP_PIN(x) ((GET_PIN(x))?(CLR_PIN(x);):(SET_PIN(x);))

void setReadMode(){
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D5, INPUT);
}

void setWriteMode(){
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D5, OUTPUT);
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

  //GPIO_REG_WRITE(GPIO_OUT_ADDRESS, 0xF0F0);
  
  if((value>>4)&1)SET_PIN(SIO0); else CLR_PIN(SIO0);
  if((value>>5)&1)SET_PIN(SIO1); else CLR_PIN(SIO1);
  if((value>>6)&1)SET_PIN(SIO2); else CLR_PIN(SIO2);
  if((value>>7)&1)SET_PIN(SIO3); else CLR_PIN(SIO3);
  //digitalWrite(SIO0,(value >> 4) &1);
  //digitalWrite(SIO1,(value >> 5) &1);
  //digitalWrite(SIO2,(value >> 6) &1);
  //digitalWrite(SIO3,(value >> 7) &1);
  FLIP_PIN(CLOCK);;
  FLIP_PIN(CLOCK);;
  if((value)&1)SET_PIN(SIO0); else CLR_PIN(SIO0);
  if((value>>1)&1)SET_PIN(SIO1); else CLR_PIN(SIO1);
  if((value>>2)&1)SET_PIN(SIO2); else CLR_PIN(SIO2);
  if((value>>3)&1)SET_PIN(SIO3); else CLR_PIN(SIO3);
  //digitalWrite(SIO0,value &1);
  //digitalWrite(SIO1,(value >> 1) &1);
  //digitalWrite(SIO2,(value >> 2) &1);
  //digitalWrite(SIO3,(value >> 3) &1);
  FLIP_PIN(CLOCK);;
  FLIP_PIN(CLOCK);;
}

void readQuad(uint8_t* buffer, uint32_t number){
  unsigned int temp=0;
  int t= -number;
  buffer+=number;
  for(; t;){
    //FLIP_PIN(CLOCK);;
    FLIP_PIN(CLOCK)
    temp = (GET_PIN(SIO0) << 4) | (GET_PIN(SIO1) << 5) | (GET_PIN(SIO2) << 6) | (GET_PIN(SIO3) << 7);
    //FLIP_PIN(CLOCK);;
    //FLIP_PIN(CLOCK);;
    FLIP_PIN(CLOCK)
    FLIP_PIN(CLOCK)
    temp |= ((GET_PIN(SIO3) << 3) | (GET_PIN(SIO2) << 2) | (GET_PIN(SIO1) << 1) | GET_PIN(SIO0));
    FLIP_PIN(CLOCK);;
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
  FLIP_PIN(CLOCK)
  FLIP_PIN(CLOCK)
  FLIP_PIN(CLOCK)
  FLIP_PIN(CLOCK)

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
//  setMode(SEQMODE);
//  setProtocol(QUADMODE);
}

void clearQuad(){
  setWriteMode();
  CLR_PIN(CS);
  writeQuad(0x02); // write command
  writeQuad(0); // First byte of address
  writeQuad(0); // Second byte of address
  writeQuad(0); // Third byte of address
  for(int t = 0x1FFFF; t; --t){
    writeQuad(15);
  }
  SET_PIN(CS);
}

void setup() {

  Serial.begin(9600);
  Serial.println("Wait 5 seconds...");  
  delay(2000); // wait a while
  Serial.println("Starting...");  
  Serial.println("Init RAM...");  
  initRAM();

  int howMany = 40;
  uint8_t myArray[howMany];
  uint8_t myArray2[howMany];
  Serial.println("Filling array with dummy data...");
  for(int t=0; t<howMany; t++){
    myArray[t] = random(255);
    Serial.print(myArray[t]);
    Serial.print(",");
  }
  Serial.println("Done.");

  setMode(SEQMODE); // default anyway?
  
  Serial.println("Setting QUAD mode");
  setProtocol(QUADMODE);
  
  Serial.print("Sending - ");
  for(int t=0; t<howMany; t++){
    Serial.print(myArray[t]);
    Serial.print(",");
  }
 
  Serial.println("");
  writeToAddressQuad(0, &myArray[0], howMany);
  readFromAddressQuad(0, myArray2, howMany);

  Serial.print("Reading - ");
  for(int t=0; t<howMany; t++){
    Serial.print(myArray2[t]);
    Serial.print(",");
  }
  Serial.println("");
  Serial.println("OK?");

  setWriteMode();
}

void loop() {
// Holding pattern for easy viewing
  SET_PIN(SIO0); CLR_PIN(SIO0);
  SET_PIN(SIO1); CLR_PIN(SIO1);
  SET_PIN(SIO2); CLR_PIN(SIO2);
  SET_PIN(SIO3); CLR_PIN(SIO3);
}
