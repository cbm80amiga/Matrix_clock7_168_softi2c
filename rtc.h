//#define BCD2DEC(x) ((x)>>4)*10+((x)&0xf)
//#define DEC2BCD(x) ((x)/10)<<4+((x)%10)

int BCD2DEC(int x) { return ((x)>>4)*10+((x)&0xf); }
int DEC2BCD(int x) { return (((x)/10)<<4)+((x)%10); }

#if USEHW==1
#define I2CStart(x)   Wire.beginTransmission(x)
#define I2CStop()     Wire.endTransmission()
#define I2CWrite(x)   Wire.write(x)
#define I2CRead()     Wire.read()
#define I2CReadLast() Wire.read()
#define I2CReq(x,y)   Wire.requestFrom(x,y)
#define I2CReady      while(!Wire.available()) {};
#else
#define I2CStart(x)   i2c_start((x<<1) | I2C_WRITE)
#define I2CStop()     i2c_stop()
#define I2CWrite(x)   i2c_write(x)
#define I2CRead()     i2c_read(false)
#define I2CReadLast() i2c_read(true)
#define I2CReq(x,y)   i2c_rep_start((x<<1) | I2C_READ)
#define I2CReady
#endif

#define DS1307_I2C_ADDRESS 0x68
#define DS1307_TIME    0x00
#define DS1307_DOW     0x03
#define DS1307_DATE    0x04
#define DS1307_MEM     0x08

void setRTCDateTime()
{
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_TIME);
  I2CWrite(DEC2BCD(second));
  I2CWrite(DEC2BCD(minute));
  I2CWrite(DEC2BCD(hour));
  I2CWrite(DEC2BCD(dayOfWeek));
  I2CWrite(DEC2BCD(day));
  I2CWrite(DEC2BCD(month));
  I2CWrite(DEC2BCD(year-2000));
  I2CStop();
}

void setRTCTime()
{
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_TIME);
  I2CWrite(DEC2BCD(second));
  I2CWrite(DEC2BCD(minute));
  I2CWrite(DEC2BCD(hour));
  I2CStop();
#if DEBUG_RTC==1
  Serial.print("SetTime: ");Serial.print(hour);Serial.print(":");Serial.print(minute);Serial.print(":");Serial.println(second);
#endif
}

void setRTCDate()
{
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_DATE);
  I2CWrite(DEC2BCD(day));
  I2CWrite(DEC2BCD(month));
  I2CWrite(DEC2BCD(year-2000));
  I2CStop();
#if DEBUG_RTC==1
  Serial.print("SetDate: ");Serial.print(day);Serial.print("-");Serial.print(month);Serial.print("-");Serial.println(year);
#endif
}

void setRTCDoW()
{
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_DOW);
  I2CWrite(DEC2BCD(dayOfWeek));
  I2CStop();
#if DEBUG_RTC==1
  Serial.print("SetDoW: ");Serial.println(dayOfWeek);
#endif
}

void getRTCDateTime(void)
{
  int v;
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_TIME);
  I2CStop();

  I2CReq(DS1307_I2C_ADDRESS, 7);
  I2CReady;

  v = I2CRead() & 0x7f;
  second = BCD2DEC(v);
  v = I2CRead() & 0x7f;
  minute = BCD2DEC(v);
  v = I2CRead() & 0x3f;
  hour = BCD2DEC(v);
  v = I2CRead() & 0x07;
  dayOfWeek = BCD2DEC(v);
  v = I2CRead() & 0x3f;
  day = BCD2DEC(v);
  v = I2CRead() & 0x3f;
  month = BCD2DEC(v);
  v = I2CReadLast() & 0xff;
  year = BCD2DEC(v) + 2000;

  I2CStop();
#if DEBUG_RTC==1
  //Serial.print(hour); Serial.print(":"); Serial.print(minute); Serial.print(":"); Serial.println(second);
  //Serial.print(day); Serial.print("-"); Serial.print(month); Serial.print("-"); Serial.println(year);
  //Serial.println(dayOfWeek);
#endif
}

void writeRTCMem(byte addr, byte val)
{
  if(addr>56) return;
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_MEM + addr);
  I2CWrite(val);
  I2CStop();
}

byte readRTCMem(byte addr)
{
  if(addr>56) return 0;
  I2CStart(DS1307_I2C_ADDRESS);
  I2CWrite(DS1307_MEM + addr);
  I2CStop();
  I2CReq(DS1307_I2C_ADDRESS, 1);
  I2CReady;
  byte v = I2CReadLast();
  I2CStop();
  return v;
}

