class LED_Matrix {   
  private:
    int cs;
    int clk;
    int data;
    void sendByte(unsigned char value) {
      int i;
      for (i=7;i>=0;i--) {   //從最高位元開始送
        digitalWrite(data,bitRead(value,i));   //讀出第ibit，從dataPin送出去
        digitalWrite(clk,HIGH);    //設定clock bit為1, 暫停10us, 再設為0，產生一個clock來將送出去的資料寫入位移暫存器
        delayMicroseconds(10);
        digitalWrite(clk,LOW);
      }
    }
    void writeData() {
      digitalWrite(cs,HIGH);
      delay(1);
      digitalWrite(cs,LOW);
    }
  public:
    LED_Matrix(int csPin,int clkPin,int dataPin) {
      cs=csPin;
      clk=clkPin;
      data=dataPin;
      pinMode(cs,OUTPUT);
      pinMode(clk,OUTPUT);
      pinMode(data,OUTPUT);
    }
    void init(unsigned char dm, unsigned char intensity, unsigned char scan) {
      digitalWrite(cs,LOW);
      sendByte(0x09);  //address of decode mode register 
      sendByte(dm);    
      writeData();   
      sendByte(0x0A);  //address of intensity register  
      sendByte(intensity);  // intensity =  0x00 (dark) ~ 0x0F (light)
      writeData();
      sendByte(0x0B);  //address of scan limit register 
      sendByte(scan);  //scan=0x00 ~ 0x07, how many digits are displayed, from 1 to 8
      writeData();  
      sendByte(0x0C);  //address of shutdown register 
      sendByte(0x01);  //set normal display (not shutdown mode)
      writeData();
    }
    void sendData(unsigned char address, unsigned char data) {
      if (address<1 || address >8)
        return;
      sendByte(address);
      sendByte(data);
      writeData();
    }
}; 
#include "pitches.h"
#include <LiquidCrystal.h>          // 引入 LCD 函式庫
LiquidCrystal lcd(12, 13, 3, 2, 10, 9); // 初始化 LCD 模組
boolean one_times;
LED_Matrix led8x8(6,7,5);  //cs=6, clk=7, data=5
int score;
int bouns;
int max_score;
int px,py;  //紅點位置
int dx,dy;  //紅點移動方向
int mx,my;  //怪獸位置
int t;      //當t=1時移動怪獸
bool over;  //結束遊戲
const int Speeker = 8;

int FrequencyTable[] = {
  NOTE_E4, NOTE_E4, NOTE_E4, NOTE_C4, NOTE_E4, NOTE_G4, NOTE_G3, 
};
int FrequencyTable2[] = {
  NOTE_B4, NOTE_F5, NOTE_F5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5,
};

int DurationTable[] = {
  8,4,4,8,4,2,2
};
int DurationTable2[] = {
  8,4,8,8,4,4,2
};
unsigned char ending[7][8]={{B11111111,B10000001,B10000001,B10000001,B10000001,B10000001,B10000001,B11111111},  //結束畫面
                           {B00000000,B01111110,B01000010,B01000010,B01000010,B01000010,B01111110,B00000000},
                           {B00000000,B00000000,B00111100,B00100100,B00100100,B00111100,B00000000,B00000000},
                           {B00000000,B00000000,B00000000,B00011000,B00011000,B00000000,B00000000,B00000000},
                           {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000},
                           {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000},
                           {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000}
                           };
                           
unsigned char start[7][8]={{B00111000,B01000100,B00000100,B00011000,B00000100,B00000100,B01000100,B00111000},  //開始畫面
                           {B00111000,B01000100,B00000100,B00000100,B00001000,B00010000,B00100000,B01111100},
                           {B00010000,B00110000,B00010000,B00010000,B00010000,B00010000,B00010000,B00111000},
                           {B00000000,B00000000,B00000000,B00011000,B00011000,B00000000,B00000000,B00000000},
                           {B00000000,B00000000,B00111100,B00100100,B00100100,B00111100,B00000000,B00000000},
                           {B00000000,B01111110,B01000010,B01000010,B01000010,B01000010,B01111110,B00000000},
                           {B11111111,B10000001,B10000001,B10000001,B10000001,B10000001,B10000001,B11111111},
                           };


void showPoint(unsigned char x,unsigned char y,unsigned char mx,unsigned char my) {  //在LED矩陣上的(x,y)位置顯示紅點，(mx,my)位置顯示怪獸(也是一個紅點)
  int i;
  unsigned char ydata;
  unsigned char mydata;
  ydata=0x01 << (y-1);    //設定y座標
  mydata=0x01 << (my-1);  //設定怪獸的my座標
  if (x==mx)              //假如兩個的x座標相同，則將兩者的y座標合併在ydata
    ydata=ydata | mydata;
  for (i=1;i<9;i++) {  //分別將x，mx座標指定的位址填入y，my座標設定的資料，其餘的位址都關閉led
    if (i==x)  //假如是x的位址，則填入y座標的資料，如果同時也是mx的位址，則ydata已包含mydata的資料      
      led8x8.sendData(i,ydata);
    else if (i==mx) 
      led8x8.sendData(i,mydata);
    else
      led8x8.sendData(i,0x00);
  }
}

void showEnding() {   //顯示遊戲結束畫面
  int i,j;
  for (i=0;i<7;i++) {
    for (j=0;j<8;j++) {
      led8x8.sendData(j+1,ending[i][j]);
    }
    
    //delay(800);  //每秀一個畫面，暫停200ms，再秀下一個畫面
    int duration = 1000/DurationTable2[i]; 
    tone(Speeker, FrequencyTable2[i],duration); 
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    noTone(Speeker);
    one_times=true;
    
    //delay(200);  //每秀一個畫面，暫停200ms，再秀下一個畫面
  }
}

void showstart() {   //顯示遊戲開始畫面
 int i,j;
  for (i=0;i<7;i++) {
    for (j=0;j<8;j++) {
      led8x8.sendData(j+1,start[i][j]);
    }
    //delay(800);  //每秀一個畫面，暫停200ms，再秀下一個畫面
    int duration = 1000/DurationTable[i]; 
    tone(Speeker, FrequencyTable[i],duration); 
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    noTone(Speeker);
    }
}

int dir(int value) {  //由於搖桿讀到的值介於0~1023之間，將這個區間分成三等份，分別表示減1，維持不變，以及加1
  if (value <341)
    return -1;
  if (value <682)
    return 0;
  return 1;  
}


void reset() {   //遊戲結束，重新設定
  Serial.begin(9600);
  showEnding();  //先秀出結束畫面
  lcd.clear();
  if(score>max_score)
  {
    lcd.setCursor(0,0);
    lcd.print("New Highest!!");
    lcd.setCursor(0,1);
    lcd.print("Fin="); 
    lcd.print(score);
    max_score=score;
  }else
  {
    lcd.setCursor(0,0);
    lcd.print("Game Over");
    lcd.setCursor(0,1);
    lcd.print("Fin="); 
    lcd.print(score);
    lcd.print("    H="); 
    lcd.print(max_score);
  } 
  delay(3000);
  score=0;
  bouns=0;
  px=5;          //設定變數初值，紅點在中央
  py=5;
  dx=0;          //紅點一開始不移動
  dy=0;
  mx=1;          //怪獸在左上角
  my=1;
  lcd.clear();
  over=false;
}

void moveM() {      //移動怪獸
  if (random(5)==0) {   //  1/5的機率隨機移動，否則朝著紅點移動
    mx=mx+random(3)-1;
    my=my+random(3)-1;
    tone(Speeker, 200,200); 
    if(px-mx<=2 && py-my<=2)
    {
      bouns=2;
      tone(Speeker, 1000,300);
    }
    else{
      bouns=1;
       tone(Speeker, 200,200);
    }
       
        
    return;
  }
  int deltax,deltay;      
  deltax=abs(px-mx);   //怪獸與紅點位置的x差值
  deltay=abs(py-my);   //怪獸與紅點位置的y差值
  
  Serial.print(deltax);
  if (deltax>deltay) { //如果x差值比y差值大，則先沿著x方向朝紅點移動，否則沿著y方向朝紅點移動 
    if (px>mx){
      mx++;
      if(deltax<=2){
        bouns=2;
        tone(Speeker, 1000,300);
      } 
      else
      {
        bouns=1;
        tone(Speeker, 200,200);
      }
    }
    else{
      mx--;
     if(deltax<=2){
        bouns=2;
        tone(Speeker, 1000,300);
      } 
      else
      {
        bouns=1;
        tone(Speeker, 200,200);
      }
    }
  }
  else {
    if (py>my)
    {
      my++;
      if(deltay<=2){
        bouns=2;
        tone(Speeker, 1000,300);
      } 
      else
      {
        bouns=1;
        tone(Speeker, 200,200);
      }
    }
    else
    {
       my--;
       if(deltay<=2){
        bouns=2;
        tone(Speeker, 1000,300);
      } 
      else
      {
        bouns=1;
        tone(Speeker, 200,200);
      }
    } 
  }
}


void setup() {
  delay(1000);
  lcd.begin(16, 2);
  one_times=true;
  led8x8.init(0x00,0x05,0x07); //0x00=> 都不要使用BCD解碼, 0x00=> 亮度設最暗, 0x07=>8組LED都要使用
  px=5;
  py=5;
  mx=1;
  my=1;
  max_score=0;
}
void loop() {
  if(one_times)
  {
    showstart();

  }
  one_times=false;
  lcd.setCursor(0,0);
  lcd.print("Game start");
  lcd.setCursor(0,1);
  lcd.print("score=");  
  lcd.print(score);
  int xval,yval;
  xval=analogRead(0);  //搖桿的VRx接到Analog In 第0腳位
  yval=analogRead(1);  //搖桿的VRy接到Analog In 第1腳位
  px=px+dir(xval);     //根據搖桿的值判斷x座標要加、減1或維持不變
  py=py+dir(yval);     //根據搖桿的值判斷y座標要加、減1或維持不變
  if(dir(xval) || dir(yval))
    score=score+bouns ;
px=constrain(px,1,8);  //將座標值限制在1-8之間
py=constrain(py,1,8);
       

  if (px==0 || px==9 || py==0 || py==9)  //如果座標等於0或9，表示撞牆，則遊戲結束
    over=true;
  else if (px==mx && py==my)             //如果更新後的座標是怪獸的位置，則遊戲結束
    over=true;
  t=1-t;       //讓t的值在0與1之間變動，即紅點移動兩次，怪獸移動一次
  if (t)       //t=1時移動怪獸
    moveM();
  mx=constrain(mx,1,8);    //將座標值限制在1-8之間 
  my=constrain(my,1,8);
  if (px==mx && py==my)    //如果移動後的位置與紅點相同，則遊戲結束
    over=true;
  if (over)                //如果結束，則顯示結束畫面，並做初始設定，開始下一次遊戲
    reset();
  else                     //否則顯示紅點及怪獸的位置
    showPoint(px,py,mx,my);
  delay(300);        
}
