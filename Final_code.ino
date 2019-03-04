#include <SDHCI.h>
#include <Audio.h>
#include <GNSS.h>
#define STRING_BUFFER_SIZE  128       /**< %Buffer size */

#define RESTART_CYCLE       (60 * 5)  /**< positioning test term */
static SpGnss Gnss; 
SDClass theSD;
AudioClass *theAudio;

File myFile;               

int trigPin = 11;    // Trigger
int echoPin = 12;    // Echo
long duration, cm;
String result; 
bool ErrEnd = false;

enum ParamSat {
  eSatGps,            /**< GPS                     World wide coverage  */
  eSatGlonass,        /**< GLONASS                 World wide coverage  */
  eSatGpsSbas,        /**< GPS+SBAS                North America        */
  eSatGpsGlonass,     /**< GPS+Glonass             World wide coverage  */
  eSatGpsQz1c,        /**< GPS+QZSS_L1CA           East Asia & Oceania  */
  eSatGpsGlonassQz1c, /**< GPS+Glonass+QZSS_L1CA   East Asia & Oceania  */
  eSatGpsQz1cQz1S,    /**< GPS+QZSS_L1CA+QZSS_L1S  Japan                */
};

/* Set this parameter depending on your current region. */
static enum ParamSat satType =  eSatGps;

static void audio_attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");
  
  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING)
    {
      ErrEnd = true;
   }
}


static void print_pos(SpNavData *pNavData)
{
  char StringBuffer[STRING_BUFFER_SIZE];

  /* print time */
  snprintf(StringBuffer, STRING_BUFFER_SIZE, "%04d/%02d/%02d ", pNavData->time.year, pNavData->time.month, pNavData->time.day);
  Serial1.print(StringBuffer);

  snprintf(StringBuffer, STRING_BUFFER_SIZE, "%02d:%02d:%02d.%06d, ", pNavData->time.hour, pNavData->time.minute, pNavData->time.sec, pNavData->time.usec);
  Serial1.print(StringBuffer);

  /* print satellites count */
  snprintf(StringBuffer, STRING_BUFFER_SIZE, "numSat:%2d, ", pNavData->numSatellites);
  Serial1.print(StringBuffer);

  /* print position data */
  if (pNavData->posFixMode == FixInvalid)
  {
    Serial1.print("No-Fix, ");
  }
  else
  {
    Serial1.print("Fix, ");
  }
  if (pNavData->posDataExist == 0)
  {
    Serial1.print("No Position");
  }
  else
  {
    Serial1.print("Lat=");
    Serial.print(pNavData->latitude, 6);
    Serial1.print(", Lon=");
    Serial.print(pNavData->longitude, 6);
  }

  Serial1.println("");
}

static void print_condition(SpNavData *pNavData)
{
  char StringBuffer[STRING_BUFFER_SIZE];
  unsigned long cnt;

  /* Print satellite count. */
  snprintf(StringBuffer, STRING_BUFFER_SIZE, "numSatellites:%2d\n", pNavData->numSatellites);
  Serial1.print(StringBuffer);

  for (cnt = 0; cnt < pNavData->numSatellites; cnt++)
  {
    const char *pType = "---";
    SpSatelliteType sattype = pNavData->getSatelliteType(cnt);

    /* Get satellite type. */
    /* Keep it to three letters. */
    switch (sattype)
    {
      case GPS:
        pType = "GPS";
        break;

      case GLONASS:
        pType = "GLN";
        break;

      case QZ_L1CA:
        pType = "QCA";
        break;

      case SBAS:
        pType = "SBA";
        break;

      case QZ_L1S:
        pType = "Q1S";
        break;

      default:
        pType = "UKN";
        break;
    }

    /* Get print conditions. */
    unsigned long Id  = pNavData->getSatelliteId(cnt);
    unsigned long Elv = pNavData->getSatelliteElevation(cnt);
    unsigned long Azm = pNavData->getSatelliteAzimuth(cnt);
    float sigLevel = pNavData->getSatelliteSignalLevel(cnt);

    /* Print satellite condition. */
    snprintf(StringBuffer, STRING_BUFFER_SIZE, "[%2d] Type:%s, Id:%2d, Elv:%2d, Azm:%3d, CN0:", cnt, pType, Id, Elv, Azm );
    Serial1.print(StringBuffer);
    Serial1.println(sigLevel, 6);
  }
}

/*------------------------------------------------------------------------------------------------------------------------------*/
void setup() {
  //Serial Port begin
  Serial.begin (115200);   //Serial Port for photon communication
  Serial1.begin(9600);  //Serial port for USB
  
  sleep(1);
  //Define inputs and outputs
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  // start audio system
  theAudio = AudioClass::getInstance();

  theAudio->begin(audio_attention_cb);

  puts("initialization Audio Library");

  theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP, AS_SP_DRV_MODE_LINEOUT);

  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);

  /* Verify player initialize */
  if (err != AUDIOLIB_ECODE_OK)
    {
      printf("Player0 initialize error\n");
      exit(1);
    }
}
 
void loop() 
{
  int result1;
  int error_flag = 0;
  static int LoopCount = 0;
  static int LastPrintMin = 0;
  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/sd0/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
 
 
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
 
  // Convert the time into a distance
  cm = (duration/2) / 29.1;     // Divide by 29.1 or multiply by 0.0343
  

  if(cm >=60)
  {
    result = "No Obstacles or pits detected";
    Serial1.println(result);
    myFile = theSD.open("Voice2.mp3");
    err = theAudio->writeFrames(AudioClass::Player0, myFile);
    theAudio->setVolume(-160);
    theAudio->startPlayer(AudioClass::Player0);
    theAudio->stopPlayer(AudioClass::Player0);
    myFile.close();
    sleep(2);
    Gnss.begin();
    Gnss.select(GPS);
    result1 = Gnss.start(COLD_START);
    if (result1 != 0)
    {
      Serial1.println("Gnss start error!!");
      error_flag = 1;
    }
    else
    {
      Serial1.println("Gnss setup OK");
    }
    
    if (Gnss.waitUpdate(-1))
    {
    /* Get NaviData. */
    SpNavData NavData;
    Gnss.getNavData(&NavData);
    while(NavData.posDataExist == 0)
    {
    if (NavData.time.minute != LastPrintMin)
    {
      print_condition(&NavData);
      LastPrintMin = NavData.time.minute;
    }

    /* Print position information. */
    print_pos(&NavData);
    /* Check loop count. */
  }
  }
    Gnss.stop();
    sleep(1);
  
  }
  else if((cm>=30)&&(cm<=60))
  {
    result = "Caution! Nearby Obstacles Detected";
    Serial1.println(result);
    myFile = theSD.open("Voice3.mp3");
    err = theAudio->writeFrames(AudioClass::Player0, myFile);
    theAudio->setVolume(-160);
    theAudio->startPlayer(AudioClass::Player0);
    theAudio->stopPlayer(AudioClass::Player0);
    myFile.close();
    sleep(2);
    Gnss.begin();
    Gnss.select(GPS);
    result1 = Gnss.start(COLD_START);
    if (result1 != 0)
    {
      Serial1.println("Gnss start error!!");
      error_flag = 1;
    }
    else
    {
      Serial1.println("Gnss setup OK");
    }
   
    if (Gnss.waitUpdate(-1))
    {
      /* Get NaviData. */
       SpNavData NavData;
    Gnss.getNavData(&NavData);
    while((NavData.posDataExist && (NavData.posFixMode != FixInvalid)))
    {
      if (NavData.time.minute != LastPrintMin)
    {
      print_condition(&NavData);
      LastPrintMin = NavData.time.minute;
    }

    /* Print position information. */
    print_pos(&NavData);
    }
  }
  else
  {
    /* Not update. */
    Serial1.println("data not update");
  }
    Gnss.stop();
    sleep(1);
  
  
  }
  else
  {
    result = "Danger! Mind your step";
    Serial1.println(result);
    myFile = theSD.open("Voice4.mp3");
    err = theAudio->writeFrames(AudioClass::Player0, myFile);
    theAudio->setVolume(-160);
    theAudio->startPlayer(AudioClass::Player0);
    theAudio->stopPlayer(AudioClass::Player0);
    myFile.close();
    sleep(2);
    Gnss.begin();
    Gnss.select(GPS);
    result1 = Gnss.start(COLD_START);
    if (result1 != 0)
    {
      Serial1.println("Gnss start error!!");
      error_flag = 1;
    }
    else
    {
      Serial1.println("Gnss setup OK");
    }
    /* Get NaviData. */
      
     if (Gnss.waitUpdate(-1))
    {
      SpNavData NavData;
      Gnss.getNavData(&NavData);
      while((NavData.posDataExist && (NavData.posFixMode != FixInvalid)))
      {
      if (NavData.time.minute != LastPrintMin)
    {
      print_condition(&NavData);
      LastPrintMin = NavData.time.minute;
    }

    /* Print position information. */
    print_pos(&NavData);
      }
  }
  else
  {
    /* Not update. */
    Serial1.println("data not update");
  }
    Gnss.stop();
    sleep(1);
  
}

  delay(2000);
}
