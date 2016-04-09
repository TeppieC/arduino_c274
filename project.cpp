#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>
#include "lcd_image.h"


#define SD_CS    5  // Chip select line for SD card
#define TFT_CS  6  // Chip select line for TFT display
#define TFT_DC   7  // Data/command line for TFT
#define TFT_RST  8  // Reset line for TFT (or connect to +5V)


const int  JOYSTICK_VER = 0;
const int  JOYSTICK_HOR = 1;
const int push_Button = 4;


Sd2Card card; 

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// import the images from the SD card
lcd_image_t imgCat = {"cat.lcd", 11, 9};
lcd_image_t imgHero = {"hero.lcd", 11, 16};
lcd_image_t imgBomb = {"bomb.lcd", 10, 10};
lcd_image_t imgBackground = {"back.lcd", 128, 160};
lcd_image_t imgCash = {"cash.lcd", 11, 4};
lcd_image_t start = {"star.lcd", 128, 160};
lcd_image_t die = {"die.lcd", 128, 160};

// used in confining the moving space of the hero
const int BOUND_LOWER = 144;
const int BOUND_UPPER = 114;

// used to record and display scores
int score_display;
const int SCORE_ADD = 100;

// size of the TFT screen
const int screen_height = 160;
const int screen_width = 128;

const int hero_width = 11;
const int hero_height = 16;
const int hero_move_speed = 11;
const int hero_jump_speed = 16;

const int cash_width = 11;
const int cash_height = 4;
const int cash_speed = 4;

const int bomb_width = 10;
const int bomb_height = 10;
const int bomb_speed = 10;

const int cat_width = 11;
const int cat_height = 9;
const int cat_speed = 11;
// if cat appear from the left,0
// if cat appear from the right,1
int cat_direction = 0;

// To calibrate the joystick
int horIni;
int verIni;


// innitialize the hero to be at the center of the bottom of the screen
// hero_position_x, hero_position_y, width, height, existence, move_speed, jump_speed, during_jump_state, during_fall_state
int hero[] = {screen_width/2-5, 144, hero_width, hero_height, 1, hero_move_speed, hero_jump_speed, 0, 0};


// cat_position_x, cat_position_y, width, height, existence, speed, direction
int cat[] = {0, 151, cat_width, cat_height, 0, cat_speed, cat_direction};

// x, y, width, height, existence, speed
int bomb[] = {0, 0, bomb_width, bomb_height, 0, bomb_speed};


// Allows for 10 cashes in maximum
const int cash_Number = 10;
// for each cash: cash_position_x, cash_position_y, state
int cashes[cash_Number][3];


/*****************************PreDef the functions****************************/

void heroUpdate();
boolean collide(int *a, int a_Width, int a_Height, 
		int *b, int b_Width, int b_Height);
void display();
void process_Objects(int *obj, int width, int height, 
		     char *img, bool scoring, bool killHero, 
		     bool horizontal, int speed, int existence_index);



/**************************Functions******************************************/


// function to move the hero
void heroUpdate() {


  //if want move to the right
  if (analogRead(JOYSTICK_HOR) > horIni+3) {
    //clear the last position of the hero
    tft.fillRect(hero[0], hero[1], hero_width, hero_height, 0xFFFF);
  
    //reset the hero's position
    hero[0] = hero[0] - hero_move_speed;
    
    //check for the boundary
    //if touch the boundary, reset hero's position_x to 0
    if (hero[0] < 0) {
      hero[0] = 0;
    }
  } else if (analogRead(JOYSTICK_HOR) < horIni-3) {
    //clear the last position of the hero
    tft.fillRect(hero[0], hero[1], hero_width, hero_height, 0xFFFF);

    //reset the hero's position
    hero[0] = hero[0] + hero_move_speed;

    //check for the boundary
    if (hero[0] > screen_width - hero_width) {
      hero[0] = screen_width - hero_width;
    }
  }
  
  //if move joystick up and the hero was not in the falling phase
  if (analogRead(JOYSTICK_VER) > verIni+7  && !hero[8] ) {
    //set the hero to the jumping phase
    hero[7] = 1;
  } 
}



// giving a boolean return value to show whether the two objects encountered
// if collide(all conditions not satisfied), return 1
// otherwise(one of the conditions satisfied), return 0
boolean collide(int *a, int a_Width, int a_Height, 
  int *b, int b_Width, int b_Height) {
  //these are conditions where the a and b did not collide
  return !((b[0] > (a[0] + a_Width)) ||
    ((b[0] + b_Width) < a[0]) ||
    (b[1] > (a[1] + a_Height)) ||
    ((b[1] + b_Height) < a[1]));
}


/*
  This function will process the conditions of objects
  1) check if player scores or game is over
  2) reset the objects' positions
  3) draw the objects in their newest positions
  4) randomly(pseudo) generate new objects
*/
// This function will distinguish the kind of objects by the boolean arguments it takes in
void process_Object(int *obj, int width, int height, lcd_image_t *img, bool scoring, bool killHero, bool horizontal, int speed,int existence_index) {
  
  
  //check if the object is on the screen
  if (obj[existence_index] == 1) {
    
    //if hero collide with the object
    if (collide(hero, hero_width, hero_height, obj, width, height)) {
      
      //check if the object is cash
      if (scoring) {
	score_display = score_display + SCORE_ADD;
      }
      obj[existence_index] = 0;
  
      //check if the object is bomb or cat
      if (killHero) {
	hero[4] = 0;
	tft.fillRect(hero[0], hero[1], hero_width, hero_height, 0xFFFF);
	//game over now	    
      }        
    }
    
    
   
    //check if the obj is a cat
    if (horizontal) {
      //if the cat appear from the left
      if (cat[6]==0) {
	//reset its position
	obj[0] = obj[0] + speed;
	//if the cat is out of the screen
	if (obj[0] > screen_width - obj[2]) {
	  obj[existence_index] = 0;
	}
      }else{
	//if the cat appear from the right
	obj[0] = obj[0] - speed;
	//if the cat is out of the screen
	if (obj[0] < 0) {
	  obj[existence_index] = 0;
	}
      }
    } else {
      //if the obj is a bomb or cash
      obj[1] = obj[1] + speed;
      //if the obj dropped to the ground
      if (obj[1] > BOUND_LOWER + hero_width) {
	//erase it
	obj[existence_index] = 0;
      }
    }
    
    
   
  }else {
    //if the certain object does not exist on the screen, here gives a certain possibility to generate it
    if (random() % 50 == 1){

      obj[existence_index] = 1;
      //if we want generate a cat
      if (horizontal) {
	
	//let the cat appear from the left
	if(random() % 2 == 1){
	  obj[0] = 0;
	  obj[6] = 0;
	}else{
	  //from the right
	  obj[0] = 117;
	  obj[6] = 1;
	}
      } else {
	//if we want generate a cash or bomb
	obj[0] = random() % (screen_width-cash_width);
	//set it to fall from the top of the screen
	obj[1] = 0;
      }
    }
  }
  
  //if the object is still on the screen after the whole procedure, draw the object on its newest position
  if (obj[existence_index] == 1) {
    lcd_image_draw(img, &tft, 0, 0, obj[0], obj[1], width, height);
  }
}




//display the score in the screen and ask the player if want another try
void display(){
  tft.fillScreen(0);
  tft.setCursor(0,0);
  tft.setTextColor(0xFFFF);
  tft.setTextWrap(false);
  
  tft.print("$$$$$$$$$$$$$$$$$$$$$");
  tft.print("\n");
  tft.print("\n");
  tft.print("\n");
  tft.print("Dear player, ");
  tft.print("\n");
  tft.print("\n");
  tft.print("You got ");
  tft.print(score_display);
  tft.print(" dollars");
  tft.print("\n");
  tft.print("\n");

  tft.print("Congratulates!");
  tft.print("\n");
  tft.print("\n");
  tft.print("BUT SORRY");
  tft.print("\n");
  tft.print("\n");
  tft.print("you can't deposit them!!");
  tft.print("\n");
  tft.print("\n");
  tft.print("HAHAHA");
  tft.print("\n");
  tft.print("\n");


  tft.print("Want another try?");
  tft.print("\n");
  tft.print("\n");
  tft.print("Press the pushbutton!");
  tft.print("\n");
  tft.print("\n");
  tft.print("$$$$$$$$$$$$$$$$$$$$$");
}




void setup() {

  Serial.begin(9600);
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
 
  //initialize the pushbutton
  pinMode(push_Button, INPUT);
  digitalWrite(push_Button, HIGH); 


  Serial.print("Initializing SD card...");

  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
    return;
  }

  Serial.println("OK!");


  if (!card.init(SPI_HALF_SPEED, SD_CS)) {
    Serial.println("Raw SD Initialization has failed");
    while (1) {};  // Just wait, stuff exploded.
  }
  


  //calibrating the joystick
  horIni = analogRead(JOYSTICK_HOR);
  verIni = analogRead(JOYSTICK_VER);

  //start the game with score 0
  score_display = 0;

  //draw the starting cutscene to the screen
  lcd_image_draw(&start, &tft, 0, 0, 0, 0, 128, 160);
  //pause for a while
  delay(1500);


  tft.fillScreen(ST7735_WHITE);
  
}


void loop() {
 
  //detect the joystick's move and set the position of the hero
  heroUpdate();
 
  
  //if the hero is not exist(game is over)
  //display the score
  if (hero[4] == 0) {
    //game over cutscene
    lcd_image_draw(&die, &tft, 0, 0, 0, 0, 128, 160);
    delay(1500);
    //display the score
    display();

    //wait for the player to restart
    while(digitalRead(push_Button)==HIGH){}
    score_display = 0;
    //restart settings
    //set all the objects' and hero's existence index to 0
    for (int i = 0; i < cash_Number; ++i){
      cashes[i][2] = 0;
    }
    cat[4] = 0;
    bomb[4] = 0;
    hero[4] = 1;
    hero[0] = screen_width/2-5;
    hero[1] = 144;
    lcd_image_draw(&start, &tft, 0, 0, 0, 0, 128, 160);
    delay(1500);

    tft.fillScreen(ST7735_WHITE);
  }

  //if the hero is rising(jumping)
  if (hero[7]) {
    tft.fillRect(hero[0], hero[1], hero_width, hero_height, 0xFFFF);
  
    hero[1] = hero[1] - hero_jump_speed;
 
    //use BOUND_UPPER and BOUND_LOWER to confine the working space of hero   
    //if the hero is jumping enough high,
    //let him fall
    if (hero[1] < BOUND_UPPER) {
      //change its jumping state to falling state
      hero[8] = 1;
      hero[7] = 0;
    }

    //if the hero falls to the ground
  } else if (hero[8]) {
    tft.fillRect(hero[0], hero[1], hero_width, hero_height, 0xFFFF);
  
    hero[1] = hero[1] + hero_jump_speed;
    if (hero[1] > BOUND_LOWER) {
      //change its falling state to jumping state
      hero[8] = 0;
      hero[1] = BOUND_LOWER;
    }
  }

  //redraw the hero
  lcd_image_draw(&imgHero, &tft, 0, 0, hero[0], hero[1], hero_width, hero_height);
 
 
  /************************Check for the hero and the cashes****************/
  for (int i = 0; i < cash_Number; ++i) {
    //if cash is on the screen
    if(cashes[i][2]){
      //cover its original position
      tft.fillRect(cashes[i][0], cashes[i][1], cash_width, cash_height, 0xFFFF);
    }
    //move it and draw it to the newest position
    process_Object(cashes[i], cash_width, cash_height, &imgCash, 1, 0, 0, cash_speed, 2);
   
  }


  //if the cat is on the screen
  if(cat[4]){
    //cover its original position
    tft.fillRect(cat[0], cat[1], cat[2], cat[3], 0xFFFF);
  }
  //move and redraw
  process_Object(cat, cat[2], cat[3], &imgCat, 0, 1, 1, cat_speed, 4);
  
  //if the bomb is on the screen
  if(bomb[4]){
    //cover its original position in order to redraw it in the new position
    tft.fillRect(bomb[0], bomb[1], bomb[2], bomb[3], 0xFFFF);
  }
  //move and redraw it to the newest position
  process_Object(bomb, bomb[2], bomb[3], &imgBomb, 0, 1, 0, bomb_speed, 4);
  
  //could run faster if change this setting
  delay(100);
}
