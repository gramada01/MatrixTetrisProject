// Tetris Game on 2 LED matrices
// Still a work in progress when it comes to the menu and buzzer

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include "LedControl.h" 

// Hardware Pins
const byte PIN_RS = 9;
const byte PIN_EN = 8;
const byte PIN_D4 = 7;
const byte PIN_D5 = 6;
const byte PIN_D6 = 5;
const byte PIN_D7 = 4;
const byte PIN_DIN = 11;
const byte PIN_CLK = 13;
const byte PIN_LOAD = 12;
const byte PIN_JOY_X = A0;
const byte PIN_JOY_Y = A1;
const byte PIN_BTN = 2;
 
const int MSG_DISPLAY_MS = 3000; 

const int BTN_DEBOUNCE_MS = 300;
const int JOY_DEBOUNCE_MS = 200;

const byte PHYSICS_STEP_MS = 50;

// Game States
enum GameState { 
  STATE_MENU, 
  STATE_PLAYING, 
  STATE_HIGHSCORE, 
  STATE_GAME_FINISHED,
  STATE_HTP,
  STATE_CREDITS,
  STATE_ADJUST_LCD
};


enum PieceType {
  I,
  J,
  L,
  O,
  S,
  T,
  Z
};

//Piece type is used to identify what piece is being used
//The digits used decribe the shape of the piece
/*
I = #
    #
    #
    #

J = #
    #
   ##

L = #
    #
    ##

O = ##
    ##

S =  ##
    ##

T = ###
     #

Z = ##
     ##
*/

const byte NUMBER_OF_PIECES = 7;
const byte NUMBER_OF_ROTATIONS = 4;

const byte I_PIECE_INDEX  = 0;
const byte J_PIECE_INDEX  = 1;
const byte L_PIECE_INDEX  = 2;
const byte O_PIECE_INDEX  = 3;
const byte S_PIECE_INDEX  = 4;
const byte T_PIECE_INDEX  = 5;
const byte Z_PIECE_INDEX  = 6;


const uint16_t pieces[NUMBER_OF_PIECES][NUMBER_OF_ROTATIONS] = {

    // I piece
    {
        0b0000111100000000, 
        /*  . . . .
            # # # #
            . . . .
            . . . . */

        0b0010001000100010, 
        /*  . . # .
            . . # .
            . . # .
            . . # . */

        0b0000000011110000, 
        /*  . . . .
            . . . .
            # # # #
            . . . . */

        0b0100010001000100  
        /*  . # . .
            . # . .
            . # . .
            . # . . */
    },

    // J piece
    {
        0b1000111000000000, 
        /*  # . . .
            # # # .
            . . . .
            . . . . */

        0b0110010001000000, 
        /*  . # # .
            . # . .
            . # . .
            . . . . */

        0b0000111000100000, 
        /*  . . . .
            # # # .
            . . # .
            . . . . */

        0b0010001001100000  
        /* . . # .
            . . # .
            . # # .
            . . . . */
    },

    // L piece
    {
        0b0010111000000000, 
        /*  . . # .
            # # # .
            . . . .
            . . . . */

        0b0100010001100000, 
        /*  . # . .
            . # . .
            . # # .
            . . . . */

        0b0000111010000000, 
        /*  . . . .
            # # # .
            # . . .
            . . . . */

        0b1100010001000000  
        /*  # # . .
            . # . .
            . # . .
            . . . . */
    },

    // O piece (Square) - No Rotation change
    {
        0b0110011000000000, 
        /*  . # # .
            . # # .
            . . . .
            . . . . */
            
        0b0110011000000000,
        0b0110011000000000,
        0b0110011000000000
    },

    // S piece
    {
        // Rotation 0
        0b0110110000000000, 
        /*  . # # .
            # # . .
            . . . .
            . . . . */

        0b0100011000100000, 
        /*  . # . .
            . # # .
            . . # .
            . . . . */

        0b0000011011000000, 
        /*  . . . .
            . # # .
            # # . .
            . . . . */

        0b1000110001000000  
        /*  # . . .
            # # . .
            . # . .
            . . . . */
    },

    // T piece
    {
        0b0100111000000000, 
        /*  . # . .
            # # # .
            . . . .
            . . . . */

        0b0100011001000000, 
        /*  . # . .
            . # # .
            . # . .
            . . . . */

        0b0000111001000000, 
        /*  . . . .
            # # # .
            . # . .
            . . . . */

        0b0100110001000000  
        /*  . # . .
            # # . .
            . # . .
            . . . . */
    },

    // Z piece
    {
        0b1100011000000000, 
        /*  # # . .
            . # # .
            . . . .
            . . . . */

        0b0010011001000000, 
        /*  . . # .
            . # # .
            . # . .
            . . . . */

        0b0000110001100000, 
        /*  . . . .
            # # . .
            . # # .
            . . . . */

        0b0100110010000000  
        /*  . # . .
            # # . .
            # . . .
            . . . . */
    }
};

struct HighScoreEntry {
  int score;
  int completionTime;
};

enum Buzz {
  NO_BUZZ,
  CANT_MOVE_BUZZ,
  SCORE_BUZZ
};

// Handles all game logic, physics, and state management
class GameModel {
public:
  long int lastPieceBlink;

  int score = 0;

  Buzz buzz = NO_BUZZ;

  const byte PIECE_WINDOW_SIZE = 4;
  const byte PIECE_STORE_SIZE = 16;
  static const byte WINDOW_NUMBER_OF_ROWS = 16;
  static const byte WINDOW_NUMBER_OF_COLUMNS = 8;

  byte pieceSpawnPoint[2] = {0, 2}; //LED matrix coordinates for piece spawning
  int pieceWindowAnchor[2]; //coordinates for the top right corner of the window that holds the piece
  byte currentPieceIndex = 0;
  byte currentRotation = 0;
  byte nextPieceIndex = 0;
  byte savedPieceIndex = 10;
 
  byte rowBlockCounter[WINDOW_NUMBER_OF_ROWS];
  byte gameWindow[WINDOW_NUMBER_OF_ROWS][WINDOW_NUMBER_OF_COLUMNS];

  unsigned long lastAutoMoveTime; //the last time the piece moved down on its own
  int autoMoveWaitPeriod = 1000;
  bool pieceShouldBePlaced = false;

  GameState currentState;
  unsigned long gameStartTime;
  unsigned long stateStartTime; 
  int elapsedTime;
  
  int menuCursor = 0;
  static const byte numberOfMenuOptions = 5;
  

  int hsRankViewer = 0;
  HighScoreEntry highScores[3];

  GameModel() {
    currentState = STATE_MENU;
    stateStartTime = 0;
    
    loadHighScores();
  }

  void initGame() {
    for(int i = 0; i < WINDOW_NUMBER_OF_ROWS; ++i){
      for(int j = 0; j < WINDOW_NUMBER_OF_COLUMNS; ++j)
        gameWindow[i][j] = 0;
      rowBlockCounter[i] = 0;
    }
    
    randomSeed(analogRead(A0));
    currentPieceIndex = random(0, NUMBER_OF_PIECES);
    nextPieceIndex = random(0, NUMBER_OF_PIECES);

    score = 0;

    gameStartTime = millis();
    elapsedTime = 0;
    currentState = STATE_PLAYING;

    lastPieceBlink = millis();

    lastAutoMoveTime = millis();
  }

  void clearRows(int rowToClear) {
    for (int i = rowToClear; i > 0; i--) {
      for (int col = 0; col < WINDOW_NUMBER_OF_COLUMNS; col++) {
        gameWindow[i][col] = gameWindow[i - 1][col];
      }
      rowBlockCounter[i] = rowBlockCounter[i - 1];
    }

    for (int col = 0; col < WINDOW_NUMBER_OF_COLUMNS; col++) {
      gameWindow[0][col] = 0;
    }
    rowBlockCounter[0] = 0;
  }
  
  void checkRows(){
    byte counter = 0;
    for(int i = WINDOW_NUMBER_OF_ROWS - 1; i >= 0; i--){
      
      if(rowBlockCounter[i] == WINDOW_NUMBER_OF_COLUMNS){
        
        score += 100;
        counter++;
        buzz = SCORE_BUZZ;
        
        clearRows(i);
        i++; 
      }
    }

    if(score > counter)
      score += (10*counter)*(10*counter);
  }

  void setupNextPiece(){
    currentRotation = 0;
    currentPieceIndex = nextPieceIndex;
    pieceWindowAnchor[0] = pieceSpawnPoint[0];
    pieceWindowAnchor[1] = pieceSpawnPoint[1];
    nextPieceIndex = random(0, NUMBER_OF_PIECES);
    if(colisionCheck(pieces[currentPieceIndex][currentRotation], pieceWindowAnchor[0], pieceWindowAnchor[1])){
      gameFinished();
      return;
    }
    lastPieceBlink = millis();
    lastAutoMoveTime = millis();
  }

  void placePiece(){
  uint16_t pieceShape = pieces[currentPieceIndex][currentRotation];
  for (int i = 0; i < PIECE_STORE_SIZE; i++) {
    if (pieceShape & (1 << (15 - i))) {
      
      int col = i % PIECE_WINDOW_SIZE;
      int row = i / PIECE_WINDOW_SIZE;

      int targetRow = row + pieceWindowAnchor[0];
      int targetCol = col + pieceWindowAnchor[1];

      if (targetRow >= 0 && targetRow < WINDOW_NUMBER_OF_ROWS && 
          targetCol >= 0 && targetCol < WINDOW_NUMBER_OF_COLUMNS) {
            
          rowBlockCounter[targetRow]++;
          gameWindow[targetRow][targetCol] = 1;
      }
    }
  }

  checkRows();
  setupNextPiece();
}

  bool colisionCheck(uint16_t pieceToCheck, int y, int x){
    for (int i = 0; i < WINDOW_NUMBER_OF_ROWS; i++) {
      if (pieceToCheck & (1 << (15 - i))) {
        int col = i % PIECE_WINDOW_SIZE;
        int row = i / PIECE_WINDOW_SIZE;
        
        int worldX = x + col;
        int worldY = y + row;

        if (worldX < 0) return true;

        if (worldX >= WINDOW_NUMBER_OF_COLUMNS) return true;

        if (worldY >= WINDOW_NUMBER_OF_ROWS) return true;

        if (worldY >= 0) {
            if (gameWindow[worldY][worldX] != 0) {
                return true;
            }
        }
      }
    }
    
    return false;
  }


  void rotatePiece(){
    byte nextRotIndex = (currentRotation + 1) % NUMBER_OF_ROTATIONS;
    uint16_t nextShape = pieces[currentPieceIndex][nextRotIndex];

    const byte NUMBER_OF_TESTS = 4;
    const byte NUMBER_OF_COORDINATES = 2;

    int testOffsets[NUMBER_OF_TESTS][NUMBER_OF_COORDINATES] = {
      {0, 0},   // Attempt 1: Standard Rotation
      {1, 0},   // Attempt 2: Wall Kick Right (Shift +1)
      {-1, 0},  // Attempt 3: Wall Kick Left (Shift -1)
      {0, -1}   // Attempt 4: Floor Kick (Shift Up)
    };

    for (byte i = 0; i < NUMBER_OF_TESTS; i++) {
      
      int kickX = testOffsets[i][0];
      int kickY = testOffsets[i][1];

      if (colisionCheck(nextShape, pieceWindowAnchor[0] + kickY, pieceWindowAnchor[1] + kickX) == false) { 
        currentRotation = nextRotIndex;
        pieceWindowAnchor[0] += kickY;
        pieceWindowAnchor[1] += kickX;
        
        return;
      }
    }
  }

  void moveLeft(){
    if(!colisionCheck(pieces[currentPieceIndex][currentRotation], pieceWindowAnchor[0], pieceWindowAnchor[1] - 1)){
      pieceWindowAnchor[1] -= 1;
    }
  }

  void moveRight(){
    if(!colisionCheck(pieces[currentPieceIndex][currentRotation], pieceWindowAnchor[0], pieceWindowAnchor[1] + 1)){
      pieceWindowAnchor[1] += 1;
    }
  }

  void moveDown(){
    if(!colisionCheck(pieces[currentPieceIndex][currentRotation], pieceWindowAnchor[0] + 1, pieceWindowAnchor[1])){
      pieceWindowAnchor[0] += 1;
    }
  }

  void gameLogic() {
    if (currentState != STATE_PLAYING) return;

    elapsedTime = (millis() - gameStartTime) / 1000;

    if(millis() - lastAutoMoveTime > autoMoveWaitPeriod){
      if(!colisionCheck(pieces[currentPieceIndex][currentRotation], pieceWindowAnchor[0] + 1, pieceWindowAnchor[1])){
        pieceWindowAnchor[0] += 1;
        pieceShouldBePlaced = false;
      }
      else{
        if(pieceShouldBePlaced){
          placePiece();
          pieceShouldBePlaced = false;
        }
        pieceShouldBePlaced = true;
      }
      lastAutoMoveTime = millis();
    }
  }

  void gameFinished() {
    updateHighScores(score, elapsedTime);
    currentState = STATE_GAME_FINISHED;
    stateStartTime = millis(); 
  }

  void loadHighScores() {
    for (int i = 0; i < 3; i++) {
      EEPROM.get(i * sizeof(HighScoreEntry), highScores[i]);
      if (highScores[i].score < 0) highScores[i] = {0, 0};
    }
  }

  void updateHighScores(int score, int time) {
    for (int i = 0; i < 3; i++) {
      if (score > highScores[i].score) {
        for (int j = 2; j > i; j--) highScores[j] = highScores[j-1];
        highScores[i] = {score, time};
        for (int k = 0; k < 3; k++) EEPROM.put(k * sizeof(HighScoreEntry), highScores[k]);
        break;
      }
    }
  }

  const char* menuOptions[numberOfMenuOptions] = {
    "PLAY",
    "HIGHSCORES",
    "HOW TO PLAY",
    "CREDITS",
    "ADJUST LCD"
  };
};

// Interface for rendering to decouple logic from display hardware
class IRenderer {
public:
  virtual void init() = 0;
  virtual void render(GameModel& model) = 0;
};

class GameRenderer : public IRenderer {
private:
  LiquidCrystal lcd;
  int cameraX;
  GameState lastState;
  byte numberOfMatrix = 2;
  LedControl lc; 
  const byte LCD_COLS = 16;
  const byte LCD_ROWS = 2;
  byte lastMenu = 0;
  byte blinkValue = 0;
  byte pieceBLinkTime = 200;
  

  byte matrixBrightness = 1;

public:
  GameRenderer() : lcd(PIN_RS, PIN_EN, PIN_D4, PIN_D5, PIN_D6, PIN_D7), lc(PIN_DIN, PIN_CLK, PIN_LOAD, numberOfMatrix) {
    cameraX = 0;
    lastState = STATE_MENU;
  }

  void init() override {
    lcd.begin(LCD_COLS, LCD_ROWS);
    for(byte i = 0 ; i < numberOfMatrix; ++i) {
      lc.shutdown(i, false);
      
      lc.setIntensity(i, 1); 
      
      lc.clearDisplay(i);
    }
    lcd.clear();
  }

  void updateMatrix(GameModel& model) {
    

    if(millis() - model.lastPieceBlink > pieceBLinkTime){
      blinkValue = !blinkValue;
      model.lastPieceBlink = millis();
    }

    
    for (int row = 0; row < model.WINDOW_NUMBER_OF_ROWS; row++) {
      for (int col = 0; col < model.WINDOW_NUMBER_OF_COLUMNS; col++) {
        
        int address;
        int realRow;

        if (row < 8) {
          address = 0;
          realRow = row;
        } else {
          address = 1;
          realRow = row - 8; 
        }


        lc.setLed(address, realRow, col, model.gameWindow[row][col]); 
      }
    }

    uint16_t pieceWindow = pieces[model.currentPieceIndex][model.currentRotation];
    for (int i = 0; i < model.WINDOW_NUMBER_OF_ROWS; i++) {
      if (pieceWindow & (1 << (15 - i))) {
        int col = i % model.PIECE_WINDOW_SIZE;
        int row = i / model.PIECE_WINDOW_SIZE;
        
        int worldX = model.pieceWindowAnchor[1] + col;
        int worldY = model.pieceWindowAnchor[0] + row;

        int address;
        int realRow;

        if (worldY < 8) {
          address = 0;
          realRow = worldY;
        } else {
          address = 1;
          realRow = worldY - 8; 
        }

        lc.setLed(address, realRow, worldX, blinkValue);
      }
    }
  }

  void render(GameModel& model) override {
    if (model.currentState != lastState) {
      lcd.clear();  
      lastState = model.currentState;
    }

    if (model.currentState == STATE_MENU) {
      if(lastMenu != model.menuCursor)
        lcd.clear();

      lcd.setCursor(0, 0); lcd.print(model.menuOptions[model.menuCursor]);
      lastMenu = model.menuCursor;
      return;
    }
    
    if (model.currentState == STATE_HIGHSCORE) {
      lcd.setCursor(0, 0); lcd.print("Rank #"); lcd.print(model.hsRankViewer + 1);
      lcd.setCursor(0, 1); lcd.print("S:"); lcd.print(model.highScores[model.hsRankViewer].score);
      lcd.print(" T:"); lcd.print(model.highScores[model.hsRankViewer].completionTime);
      return;
    }
    
    if (model.currentState == STATE_GAME_FINISHED) {
      lcd.setCursor(0, 0); lcd.print("GAME FINISHED!");
      lcd.setCursor(0, 1); lcd.print("S:"); lcd.print(model.score);
      lcd.print(" T:"); lcd.print(model.elapsedTime); lcd.print("s");
      return;
    }


    // Render Gameplay

    updateMatrix(model);


  }
};

class GameController {
private:
  GameModel& model;
  IRenderer* renderer;
  unsigned long lastUpdate;
  unsigned long lastInputTime;
  const int MAX_THRESHOLD = 700;
  const int MIN_THRESHOLD = 300;

public:
  GameController(GameModel& m, IRenderer* r) : model(m), renderer(r) {
    lastUpdate = 0;
    lastInputTime = 0;
  }

  void setup() {
    renderer->init();
  }

  void loop() {
    if (millis() - lastUpdate > PHYSICS_STEP_MS) {
      handleInput();
      model.gameLogic();
      renderer->render(model); 
      lastUpdate = millis();
    }
  }

private:
  void handleInput() {
    int xVal = analogRead(PIN_JOY_X);
    int yVal = analogRead(PIN_JOY_Y);
    bool btn = (digitalRead(PIN_BTN) == LOW);
    
    if (model.currentState == STATE_MENU || model.currentState == STATE_HIGHSCORE) {
      if (millis() - lastInputTime < JOY_DEBOUNCE_MS) return;

      if (model.currentState == STATE_MENU) {
        if(xVal < MIN_THRESHOLD){
          if(model.menuCursor < model.numberOfMenuOptions - 1)
            model.menuCursor++;
          else
            model.menuCursor = 0;
        }
        if(xVal > MAX_THRESHOLD){
          if(model.menuCursor > 0)
            model.menuCursor--;
          else
            model.menuCursor = model.numberOfMenuOptions - 1;
        }
        
        if (btn && millis() - lastInputTime > BTN_DEBOUNCE_MS) {
          if (model.menuCursor == 0) model.initGame();
          else model.currentState = STATE_HIGHSCORE;
          lastInputTime = millis();
        }
      } else { 
        if (xVal > MAX_THRESHOLD && model.hsRankViewer > 0) model.hsRankViewer--;
        if (xVal < MIN_THRESHOLD && model.hsRankViewer < 2) model.hsRankViewer++;
        
        if (btn && millis() - lastInputTime > BTN_DEBOUNCE_MS) {
           model.currentState = STATE_MENU;
           lastInputTime = millis();
        }
      }
      if (xVal > MAX_THRESHOLD || xVal < MIN_THRESHOLD) lastInputTime = millis();
      return;
    }

    if(model.currentState == STATE_GAME_FINISHED){
      if(millis() - model.stateStartTime > MSG_DISPLAY_MS)
        model.currentState = STATE_MENU;
    }

    if(model.currentState == STATE_PLAYING){
      
      if (millis() - lastInputTime < JOY_DEBOUNCE_MS) return; 

      bool moved = false;

      if(xVal > MAX_THRESHOLD) {
        model.rotatePiece();
        moved = true;
      }

      if(xVal < MIN_THRESHOLD) {
        model.moveDown();
        moved = true;
      }

      if(yVal < MIN_THRESHOLD) {
        model.moveLeft();
        moved = true;
      }

      if(yVal > MAX_THRESHOLD) {
        model.moveRight();
        moved = true;
      }

      if(moved) {
        lastInputTime = millis();
      }
    }
  }
};

GameModel model;
GameRenderer gameRenderer;
GameController controller(model, &gameRenderer);

void setup() {
  pinMode(PIN_JOY_X, INPUT);
  pinMode(PIN_JOY_Y, INPUT);
  pinMode(PIN_BTN, INPUT_PULLUP);
  controller.setup();
}

void loop() {
  controller.loop();
}