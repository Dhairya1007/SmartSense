#include <SDHCI.h>
#include <Audio.h>

SDClass theSD;
AudioClass *theAudio;

File myFile;               

int trigPin = 11;    // Trigger
int echoPin = 12;    // Echo
long duration, cm;
String result; 
bool ErrEnd = false;

static void audio_attention_cb(const ErrorAttentionParam *atprm)
{
  puts("Attention!");
  
  if (atprm->error_code >= AS_ATTENTION_CODE_WARNING)
    {
      ErrEnd = true;
   }
}

void setup() 
{
  //Serial Port begin
  Serial.begin (115200); 
  sleep(1);
  //Define inputs and outputs  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  // start audio system
  theAudio = AudioClass::getInstance();
  theAudio->begin(audio_attention_cb);
  puts("initialization Audio Library");
  /* Set clock mode to normal */
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
    Serial.println(cm);
    result = "No Obstacles or pits detected";
    Serial.println(result);
    myFile = theSD.open("Voice2.mp3");
    err = theAudio->writeFrames(AudioClass::Player0, myFile);
    theAudio->setVolume(-160);
    theAudio->startPlayer(AudioClass::Player0);
    theAudio->stopPlayer(AudioClass::Player0);
    myFile.close();   
  }
  else if((cm>=30)&&(cm<=60))
  {
    Serial.println(cm);
    result = "Caution! Nearby Obstacles Detected";
    Serial.println(result);
    myFile = theSD.open("Voice3.mp3");
    err = theAudio->writeFrames(AudioClass::Player0, myFile);
    theAudio->setVolume(-160);
    theAudio->startPlayer(AudioClass::Player0);
    theAudio->stopPlayer(AudioClass::Player0);
    myFile.close();    
  }
  else
  {
    Serial.println(cm);
    result = "Danger! Mind your step";
    Serial.println(result);
    myFile = theSD.open("Voice4.mp3");
    err = theAudio->writeFrames(AudioClass::Player0, myFile);
    theAudio->setVolume(-200);
    theAudio->startPlayer(AudioClass::Player0);
    theAudio->stopPlayer(AudioClass::Player0);
    myFile.close();
  }
  delay(4000);
}
