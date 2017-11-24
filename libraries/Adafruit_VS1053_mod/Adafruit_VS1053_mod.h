/*************************************************** 
  This is a library for the Adafruit VS1053 Codec Breakout

  Designed specifically to work with the Adafruit VS1053 Codec Breakout 
  ----> https://www.adafruit.com/products/1381

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/
#ifndef ADAFRUIT_VS1053_H
#define ADAFRUIT_VS1053_H

#if (ARDUINO >= 100)
 #include <Arduino.h>
#else
 #include <WProgram.h>
 #include <pins_arduino.h>
#endif

#if !defined(ARDUINO_STM32_FEATHER)
#include "pins_arduino.h"
#include "wiring_private.h"
#endif

#include <SPI.h> 
#include <SdFat.h>


// define here the size of a register!
#if defined(ARDUINO_STM32_FEATHER)
  typedef volatile uint32 RwReg;
  typedef uint32_t PortMask;
#elif defined (__AVR__)
  typedef volatile uint8_t RwReg;
  typedef uint8_t PortMask;
#elif defined (__arm__)
  #if defined(TEENSYDUINO)
  typedef volatile uint8_t RwReg;
  typedef uint8_t PortMask;
  #else
  typedef volatile uint32_t RwReg;
  typedef uint32_t PortMask;
  #endif
#elif defined (ESP8266) || defined (ESP32)
  typedef volatile uint32_t RwReg;
  typedef uint32_t PortMask;
#elif defined (__ARDUINO_ARC__)
  typedef volatile uint32_t RwReg;
  typedef uint32_t PortMask;
#else
  typedef volatile uint8_t RwReg;
  typedef uint8_t PortMask;
#endif

typedef volatile RwReg PortReg;

#define VS1053_FILEPLAYER_TIMER0_INT 255 // allows useInterrupt to accept pins 0 to 254
#define VS1053_FILEPLAYER_PIN_INT 5

#define VS1053_SCI_READ 0x03
#define VS1053_SCI_WRITE 0x02

#define VS1053_REG_MODE  0x00
#define VS1053_REG_STATUS 0x01
#define VS1053_REG_BASS 0x02
#define VS1053_REG_CLOCKF 0x03
#define VS1053_REG_DECODETIME 0x04
#define VS1053_REG_AUDATA 0x05
#define VS1053_REG_WRAM 0x06
#define VS1053_REG_WRAMADDR 0x07
#define VS1053_REG_HDAT0 0x08
#define VS1053_REG_HDAT1 0x09
#define VS1053_REG_VOLUME 0x0B

#define VS1053_GPIO_DDR 0xC017
#define VS1053_GPIO_IDATA 0xC018
#define VS1053_GPIO_ODATA 0xC019

#define VS1053_INT_ENABLE  0xC01A

#define VS1053_MODE_SM_DIFF 0x0001
#define VS1053_MODE_SM_LAYER12 0x0002
#define VS1053_MODE_SM_RESET 0x0004
#define VS1053_MODE_SM_CANCEL 0x0008
#define VS1053_MODE_SM_EARSPKLO 0x0010
#define VS1053_MODE_SM_TESTS 0x0020
#define VS1053_MODE_SM_STREAM 0x0040
#define VS1053_MODE_SM_SDINEW 0x0800
#define VS1053_MODE_SM_ADPCM 0x1000
#define VS1053_MODE_SM_LINE1 0x4000
#define VS1053_MODE_SM_CLKRANGE 0x8000


#define VS1053_SCI_AIADDR 0x0A
#define VS1053_SCI_AICTRL0 0x0C
#define VS1053_SCI_AICTRL1 0x0D
#define VS1053_SCI_AICTRL2 0x0E
#define VS1053_SCI_AICTRL3 0x0F

#define VS1053_DATABUFFERLEN 32


#define para_chipID_0       0x1E00

/**
 * \brief A macro of the WRAM para_chipID_1 register's address (R/W)
 *
 * para_chipID_1 is a Read/Write Extra Parameter in X memory, accessed indirectly
 * with the SCI_WRAMADDR and SCI_WRAM.
 *
 * The fuse-programmed ID is read at startup and copied into the chipID field.
 * If not available the value will be all zeros.
 */
#define para_chipID_1       0x1E01

/**
 * \brief A macro of the WRAM para_version register's address (R/W)
 *
 * para_version is a Read/Write Extra Parameter in X memory, accessed indirectly
 * with the SCI_WRAMADDR and SCI_WRAM.
 *
 *
 */
#define para_version        0x1E02

/**
 * \brief A macro of the WRAM para_config1 register's address (R/W)
 *
 * para_config1 is a Read/Write Extra Parameter in X memory, accessed indirectly
 * with the SCI_WRAMADDR and SCI_WRAM.
 *
 * The version field can be used to determine the layout of the rest
 * of the structure. The version number is changed when the structure is changed. For VS1053b
 * the structure version is 3.
 */
#define para_config1        0x1E03

/**
 * \brief A macro of the WRAM para_playSpeed register's address (R/W)
 *
 * para_playSpeed is a Read/Write Extra Parameter in X memory, accessed indirectly
 * with the SCI_WRAMADDR and SCI_WRAM.
 *
 * config1 controls MIDI Reverb and AACs SBR and PS settings.
 */
#define para_playSpeed      0x1E04

/**
 * \brief A macro of the WRAM para_byteRate register's address (R/W)
 *
 * para_byteRate is a Read/Write Extra Parameter in X memory, accessed indirectly
 * with the SCI_WRAMADDR and SCI_WRAM.
 *
 * playSpeed makes it possible to fast forward songs. Decoding of the bitstream is performed,
 * but only each playSpeed frames are played. For example by writing 4 to playSpeed will play
 * the song four times as fast as normal, if you are able to feed the data with that speed. Write 0
 * or 1 to return to normal speed. SCI_DECODE_TIME will also count faster. All current codecs
 * support the playSpeed configuration.
 */
#define para_byteRate       0x1E05

/**
 * \brief A macro of the WRAM para_endFillByte register's address (R/W)
 *
 * para_endFillByte is a Read/Write Extra Parameter in X memory, accessed indirectly
 * with the SCI_WRAMADDR and SCI_WRAM.
 *
 * The endFillByte indicates what byte value to send as to properly flush the
 * streams playback buffer, before SM_CANCEL is set, as to gracefully end the
 * current stream.
 *
 * \warning Omitting the endFillByte requirement may prevent subsequent streams from
 * properly resyncing.
 */
#define para_endFillByte    0x1E06

/**
 * \brief A macro of the WRAM para_MonoOutput register's address (R/W)
 *
 * para_MonoOutput is a Read/Write Extra Parameter in X memory, accessed indirectly
 * with the SCI_WRAMADDR and SCI_WRAM.
 *
 * Analog output of the VS10XX may be configuured to be either either
 * Stereo(default) or Mono. When correspondingly set to 0 or 1.
 *
 * \warning This feature is only available when composite patch 1.7 or higher
 * is loaded into the VSdsp.
 */
#define para_MonoOutput    0x1E09

/**
 * \brief A macro of the WRAM para_positionMsec_0 register's address (R/W)
 *
 * para_positionMsec_0 is a Read/Write Extra Parameter in X memory, accessed indirectly
 * with the SCI_WRAMADDR and SCI_WRAM. Corresponding to the high 16 bit value of positionMsec
 *
 * positionMsec is a field that gives the current play position in a file in milliseconds, regardless
 * of rewind and fast forward operations. The value is only available in codecs that can determine
 * the play position from the stream itself. Currently WMA and Ogg Vorbis provide this information.
 * If the position is unknown, this field contains -1.
 */

#define para_positionMsec_0 0x1E27

/**
 * \brief A macro of the WRAM para_positionMsec_1 register's address (R/W)
 *
 * para_positionMsec_1 is a Read/Write Extra Parameter in X memory, accessed indirectly
 * with the SCI_WRAMADDR and SCI_WRAM. Corresponding to the high 16 bit value of positionMsec
 *
 * positionMsec is a field that gives the current play position in a file in milliseconds, regardless
 * of rewind and fast forward operations. The value is only available in codecs that can determine
 * the play position from the stream itself. Currently WMA and Ogg Vorbis provide this information.
 * If the position is unknown, this field contains -1.
 */
#define para_positionMsec_1 0x1E28

/**
 * \brief A macro of the WRAM para_resync register's address (R/W)
 *
 * para_resync is a Read/Write Extra Parameter in X memory, accessed indirectly
 * with the SCI_WRAMADDR and SCI_WRAM.
 *
 * The resync field is set to 32767 after a reset to make resynchronization the default action, but
 * it can be cleared after reset to restore the old action. When resync is set, every file decode
 * should always end as described in Chapter 9.5.1.
 */
#define para_resync         0x1E29

/** End Extra_Parameter_Group
 *  /@}
 */

#define TRUE                     1
#define FALSE                    0

//------------------------------------------------------------------------------
/** \name ID3_Tag_Group
 *  ID3 Tag location offsets
 *  /@{
 */

/**
 * \brief A macro of the offset for the track's Title
 *
 * The offset from the begining of the ID3 tag for the location containing the track's Title of the
 * mp3 file being read from the SdCard.
 *
 * \warning This may not be available on all source music files.
 */
#define TRACK_TITLE              3
/**
 * \brief A macro of the offset for the track's Artist
 *
 * The offset from the begining of the ID3 tag for the location containing the track's Artist of the
 * mp3 file being read from the SdCard.
 *
 * \warning This may not be available on all source music files.
 */
#define TRACK_ARTIST            33
/**
 * \brief A macro of the offset for the track's Album
 *
 * The offset from the begining of the ID3 tag for the location containing the track's Artist of the
 * mp3 file being read from the SdCard.
 *
 * \warning This may not be available on all source music files.
 */
#define TRACK_ALBUM             63

enum flush_m {

/** \brief Flush After
 *
 * This will cause vs1053::flush_cancel(flush_m) to flush with endfillbyte
 * after issuing the cancel
 */
  post,

/** \brief Flush First
 *
 * This will cause vs1053::flush_cancel(flush_m) to flush with endfillbyte
 * before issuing the cancel
 */
  pre,

/** \brief Flush both First and After
 *
 * This will cause vs1053::flush_cancel(flush_m) to flush with endfillbyte
 * before and after issuing the cancel
 */
  both,

/** \brief Don't Flush
 *
 * This will cause vs1053::flush_cancel(flush_m) NOT to flush with endfillbyte
 * when issuing the cancel
 */
  none
  }; //enum flush_m


//extern SdFat SD;

class Adafruit_VS1053 {
 public:
  Adafruit_VS1053(int8_t mosi, int8_t miso, int8_t clk, 
		  int8_t rst, int8_t cs, int8_t dcs, int8_t dreq);
  Adafruit_VS1053(int8_t rst, int8_t cs, int8_t dcs, int8_t dreq);
  uint8_t begin(void);
  void reset(void);
  void softReset(void);
  uint16_t sciRead(uint8_t addr);
  void sciWrite(uint8_t addr, uint16_t data);
  void sineTest(uint8_t n, uint16_t ms);
  void spiwrite(uint8_t d);
  void spiwrite(uint8_t *c, uint16_t num); 
  uint8_t spiread(void);
  void spiInit(void);

  uint16_t decodeTime(void);
  void setVolume(uint8_t left, uint8_t right);
  uint16_t getVolume(void);
  void dumpRegs(void);

  void playData(uint8_t *buffer, uint8_t buffsiz);
  boolean readyForData(void);
  void applyPatch(const uint16_t *patch, uint16_t patchsize);
  uint16_t loadPlugin(char *fn);

  void GPIO_digitalWrite(uint8_t i, uint8_t val);
  void GPIO_digitalWrite(uint8_t i);
  uint16_t GPIO_digitalRead(void);
  boolean GPIO_digitalRead(uint8_t i);
  void GPIO_pinMode(uint8_t i, uint8_t dir);
 
  boolean prepareRecordOgg(char *plugin);
  void startRecordOgg(boolean mic);
  void stopRecordOgg(void);
  uint16_t recordedWordsWaiting(void);
  uint16_t recordedReadWord(void);

  uint8_t mp3buffer[VS1053_DATABUFFERLEN];

  void cs_high(void);
  void cs_low(void);
  void dcs_high(void);
  void dcs_low(void);
  void refill(void);
  void flush_cancel(flush_m);
  uint16_t Mp3ReadWRAM(uint16_t);
  void Mp3WriteWRAM(uint16_t, uint16_t);

  static SdFat SD_CARD;
  static SdFile track;
  uint8_t _cardCS;
#ifdef ARDUINO_ARCH_SAMD
 protected:
  int32_t _dreq;
 
private:
  int32_t _mosi, _miso, _clk, _reset, _cs, _dcs;
  


  boolean useHardwareSPI;

#else
 protected:
  int8_t _dreq;
 private:
  int8_t _mosi, _miso, _clk, _reset, _cs, _dcs;
  boolean useHardwareSPI;
#endif
};


class Adafruit_VS1053_FilePlayer : public Adafruit_VS1053 {
 public:
  Adafruit_VS1053_FilePlayer (int8_t mosi, int8_t miso, int8_t clk, 
			      int8_t rst, int8_t cs, int8_t dcs, int8_t dreq,
			      int8_t cardCS);
  Adafruit_VS1053_FilePlayer (int8_t rst, int8_t cs, int8_t dcs, int8_t dreq,
			      int8_t cardCS);
  Adafruit_VS1053_FilePlayer (int8_t cs, int8_t dcs, int8_t dreq,
			      int8_t cardCS);

  boolean begin(void);
  boolean useInterrupt(uint8_t type);
  File currentTrack;
  volatile boolean playingMusic;
  void feedBuffer(void);
  boolean startPlayingFile(const char *trackname);
  boolean playFullFile(const char *trackname);
  void stopPlaying(void);
  boolean paused(void);
  boolean stopped(void);
  void pausePlaying(boolean pause);
  uint8_t skip(int32_t);
  
 private:
  void feedBuffer_noLock(void);

};

#endif // ADAFRUIT_VS1053_H
