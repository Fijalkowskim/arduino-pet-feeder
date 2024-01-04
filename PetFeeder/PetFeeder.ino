#include <LiquidCrystal.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#define FEEDING_SLOTS 4
#define MENU_SCREENS 5
#define REFRESH_RATE 100
#define MAX_PORTIONS 8
#define FEEDING_MINUTES_INCREMENT 15

struct FeedData{
  short  hour;
  short  minute;
  short  portions;
  bool isActive;
  bool tomorrow; 

  FeedData(short _hour, short _minute,short _portions,bool _isActive) : hour(_hour), minute(_minute), portions(_portions), isActive(_isActive), tomorrow(false) {}
  FeedData() : hour(12), minute(0), portions(1), isActive(false), tomorrow(false) {}

  String AsString(){
    String s = "";
    if(hour < 10) s+="0";
    s+= String(hour);
    s+=":";
    if(minute < 10) s+="0";
    s+= String(minute);
    return s;
    }
};

class MenuPage{
  public:
  virtual void displayLoop() = 0;
  virtual void selectBtnClick() = 0;
  virtual void leftBtnClick() = 0;
  virtual void rightBtnClick() = 0;
};
//############################################################################################################################
//---------Compontents
ThreeWire rtcWire(11,12,13);
RtcDS1302<ThreeWire> Rtc(rtcWire);
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);
const int noButtons = 3;
int buttons[noButtons] = {14,15,16};
bool btnClicked[noButtons];
//---------Variables

//Feeding
FeedData feedData[FEEDING_SLOTS];
const byte editBlinkingTime = 5;
bool allFeedingsUnactive = true;
FeedData* nextFeedingData;
bool feedingNow = false;
int hourOfFeedingSet = 0;
const byte feedingPortionTime;

//Display
int screenIndex = 0;
bool editMode = false;
MenuPage* menuScreens[MENU_SCREENS];
MenuPage* currentScreen = nullptr;

//############################################################################################################################

//SetUp//############################################################################################################################
void setup(){
  Serial.begin(9600); 
  setUpButtons();
  setUpLcd();
  setUpRtc();
  setUpFeedTimes();
  setUpDisplays();
}
void setUpButtons(){
for(int i=0; i<noButtons;i++){
    pinMode(buttons[i],INPUT_PULLUP);
  }
}
void setUpLcd(){
lcd.begin(16, 2);
}
void setUpRtc(){   
    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    Serial.println();
    Serial.println("Current time:");
    printDateTime(compiled);
    Serial.println();

    if (!Rtc.IsDateTimeValid()) 
    {
        Serial.println("RTC lost confidence in the DateTime!");
        Rtc.SetDateTime(compiled);
    }

    if (Rtc.GetIsWriteProtected())
    {
        Serial.println("RTC was write protected, enabling writing now");
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }

    
}
void setUpFeedTimes(){
  feedData[0] = FeedData();
  feedData[1] = FeedData();
  feedData[2] = FeedData();
  feedData[3] = FeedData();
  allFeedingsUnactive = true;
  findNextFeedingTime();
}
//SetUp//############################################################################################################################
void loop() {
  handleButtons();
  handleFeeding();
  handleDisplay();
  delay(REFRESH_RATE);
}
//Clock//############################################################################################################################
void displayClock(){
  RtcDateTime now = Rtc.GetDateTime();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(getCurrentTime(now));
  lcd.setCursor(0, 1);
  lcd.print("Feeding: ");
  if(allFeedingsUnactive){
    lcd.print("NONE");
  }
  else if(nextFeedingData != nullptr){  
    lcd.print(nextFeedingData->AsString());
  }
    

}
#define countof(a) (sizeof(a) / sizeof(a[0]))

String getCurrentTime(const RtcDateTime& dt){
  char timestring[26];
  snprintf_P(timestring, 
            countof(timestring),
            PSTR("%02u:%02u:%02u"),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
  return timestring;
}
const char* printDateTime(const RtcDateTime& dt)
{
    char datestring[26];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}

//Clock//############################################################################################################################

//Feeding//############################################################################################################################

void handleFeeding(){
  if(nextFeedingData == nullptr)
  return;
  RtcDateTime now = Rtc.GetDateTime();
  int hour = now.Hour();
  if(nextFeedingData->tomorrow && hour < hourOfFeedingSet){
    nextFeedingData->tomorrow = false;
  }
  if(!nextFeedingData->tomorrow){
     int nowAsMinutes = now.Hour() * 60 + now.Minute();
     int feedingTimeAsMinutes = nextFeedingData->hour * 60 + nextFeedingData->minute;
     if (nowAsMinutes >= feedingTimeAsMinutes){
      feed();
     }
  }
}
void feed(){
  feedingNow = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Feeding...");
  //Activate servo
  delay(feedingPortionTime * nextFeedingData->portions);
  //Disactivate servo
  feedingNow = false;
  findNextFeedingTime();
}
void findNextFeedingTime(){
  if(allFeedingsUnactive){
    nextFeedingData = nullptr;
    return;
  } 
  RtcDateTime now = Rtc.GetDateTime();
  int nowAsMinutes = now.Hour() * 60 + now.Minute();
  int currentMinAsMinutes = 1440;
  int currentMinIndex = -1;
  for(byte i=0; i < FEEDING_SLOTS; i++){
    if(!feedData[i].isActive )
      continue;
    int feedingAsMinutes = feedData[i].hour * 60 + feedData[i].minute;
    if(feedingAsMinutes < nowAsMinutes)
      continue;
    if(feedingAsMinutes < currentMinAsMinutes){
      currentMinAsMinutes = feedingAsMinutes;
      currentMinIndex = i;
      }
    }
    if(currentMinIndex == -1){ //Feeding time is tomorrow
      for(byte i=0; i < FEEDING_SLOTS; i++){
        if(!feedData[i].isActive )
        continue;
        int feedingAsMinutes = feedData[i].hour * 60 + feedData[i].minute;
        if(feedingAsMinutes < currentMinAsMinutes){
        currentMinAsMinutes = feedingAsMinutes;
        currentMinIndex = i;
        }
      }
      nextFeedingData = &feedData[currentMinIndex];
      nextFeedingData->tomorrow = true;
      hourOfFeedingSet = now.Hour();
      return;
    }

    nextFeedingData = &feedData[currentMinIndex];
    nextFeedingData->tomorrow = false;
}


class HomePage: public MenuPage{
  public:
  virtual void displayLoop(){
    displayClock();
  }
  virtual void selectBtnClick(){

  }
  virtual void leftBtnClick(){

  }
  virtual void rightBtnClick(){

  }
};
//Feeding//############################################################################################################################

//Buttons//############################################################################################################################
void buttonClicked(int btnIdx){
  switch(btnIdx){
    case 0: //Select
      if(currentScreen != nullptr)
        currentScreen->selectBtnClick();
        
    break;
    case 1: //Left
      if(editMode && currentScreen != nullptr){
        currentScreen->leftBtnClick();
      }
      else{
      screenIndex = screenIndex - 1 < 0 ? MENU_SCREENS - 1 : screenIndex - 1;
      currentScreen = menuScreens[screenIndex];
      }
      
    break;
    case 2: //Right
    if(editMode && currentScreen != nullptr){
        currentScreen->rightBtnClick();
    }
    else{
      screenIndex = screenIndex + 1 >= MENU_SCREENS ? 0 : screenIndex + 1;
      currentScreen = menuScreens[screenIndex];
    }
      
    break;
    default:
    break;
  }
  
}
void handleButtons(){
  for(int i=0; i<noButtons;i++){
    if (!btnClicked[i] && digitalRead(buttons[i]) == LOW) { 
      btnClicked[i] = true;
      buttonClicked(i);
    } else if(btnClicked[i] && digitalRead(buttons[i]) == HIGH) { 
      btnClicked[i] = false;
      delay(100);
    }
  }
}
//Buttons//############################################################################################################################

//Display//############################################################################################################################
void handleDisplay(){
  if(currentScreen == nullptr)
  return;
  currentScreen->displayLoop();
}

//Display//############################################################################################################################





//Feed DISPLAY//############################################################################################################################

enum FeedEditState{
  NONE, ACTIVE, HOUR, MINUTE, PORTIONS
};

class FeedingMenuPage: public MenuPage{
  private: 
  byte index = 0;
  FeedEditState editState = NONE;
  bool blink = false;
  byte blinkIndex = 0;
  public:
  virtual void displayLoop(){
    if(feedingNow){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("     Feeding");
      return;
    }
    if(editState == ACTIVE){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("     Active:");
      lcd.setCursor(0,1);
      if(feedData[index].isActive)
        lcd.print("       YES");
      else
        lcd.print("       NO");
    }
    else{
      displayFeedingData();
      if(editState != NONE){
        blinkIndex++;
        if(blinkIndex >= editBlinkingTime){
         blink = !blink;
         blinkIndex = true;
        }
      }  
    }
     
  }
  void StopEditing(){
    editState = NONE;
    editMode = false;
    findNextFeedingTime();
  }
  virtual void selectBtnClick(){
    switch(editState){
      case NONE:
      editMode = true;
      editState = ACTIVE;  
      break;
      case ACTIVE:   
      if(!feedData[index].isActive){ //Disactivate feeding
        StopEditing();
        if(!allFeedingsUnactive){
          for(byte i=0;i<FEEDING_SLOTS;i++){
            if(feedData[i].isActive) return;
          }
          allFeedingsUnactive = true;
        }
      }      
      else { //Activate feeding
        blink = true;
        blinkIndex = 0;
        editState = HOUR;
        if(allFeedingsUnactive){
          allFeedingsUnactive = false;
        }    
      }
         
      break;
      case HOUR:
      editState = MINUTE;
      blink = true;
      blinkIndex = 0;
      break;
      case MINUTE:
      editState = PORTIONS;
      blink = true;
      blinkIndex = 0;
      break;
      case PORTIONS:
      StopEditing();
      break;
    }

  }
  virtual void leftBtnClick(){
  
    switch(editState){
      case NONE:
      break;
      case ACTIVE:
      feedData[index].isActive = !feedData[index].isActive;
      break;
      case HOUR:
      feedData[index].hour = feedData[index].hour - 1 < 0 ? 23 : feedData[index].hour - 1;
      break;
      case MINUTE:
      feedData[index].minute = feedData[index].minute - FEEDING_MINUTES_INCREMENT < 0 ? 60 - FEEDING_MINUTES_INCREMENT : feedData[index].minute - FEEDING_MINUTES_INCREMENT;
      break;
      case PORTIONS:
      feedData[index].portions = feedData[index].portions - 1 < 1 ? MAX_PORTIONS : feedData[index].portions - 1 ;
      break;
    }

  }
  virtual void rightBtnClick(){
    switch(editState){
      case NONE:
      break;
      case ACTIVE:
      feedData[index].isActive = !feedData[index].isActive;
      break;
      case HOUR:
      feedData[index].hour = feedData[index].hour + 1 > 23 ? 0 : feedData[index].hour + 1;
      ResetBlink();
      break;
      case MINUTE:
      feedData[index].minute = feedData[index].minute + FEEDING_MINUTES_INCREMENT > 59 ? 0 : feedData[index].minute + FEEDING_MINUTES_INCREMENT;
      ResetBlink();
      break;
      case PORTIONS:
      feedData[index].portions = feedData[index].portions + 1 > MAX_PORTIONS ? 1 : feedData[index].portions + 1 ;
      ResetBlink();
      break;
    }
  }
  FeedingMenuPage(int _index):index(_index){}
  FeedingMenuPage(){}

  void ResetBlink(){
    blink = false;
    blinkIndex = 0;
  }

  void displayFeedingData(){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("    Feeding ");
    lcd.print(index+1);
    lcd.setCursor(0,1);
    if(!feedData[index].isActive)
    {    
      lcd.print("   Not Active");
      return;
    }
    
    if(blink && editState == HOUR)
      lcd.print("  ");
    else{
      if(feedData[index].hour < 10) lcd.print("0");
      lcd.print(feedData[index].hour);
    }
      
    lcd.print(":");

    
    if(blink && editState == MINUTE)
      lcd.print("  ");
    else{
      if(feedData[index].minute < 10) lcd.print("0");
      lcd.print(feedData[index].minute);
    }    
    
    lcd.print(" ");
    if(blink && editState == PORTIONS)
      lcd.print(" ");
    else
      lcd.print(feedData[index].portions);

    lcd.print(" PORTIONS");

  
  }

};
//Feed DISPLAY//############################################################################################################################

void setUpDisplays(){
    menuScreens[0] = new HomePage();
    menuScreens[1] = new FeedingMenuPage(0);
    menuScreens[2] = new FeedingMenuPage(1);
    menuScreens[3] = new FeedingMenuPage(2);
    menuScreens[4] = new FeedingMenuPage(3);
    currentScreen = menuScreens[0];
}

