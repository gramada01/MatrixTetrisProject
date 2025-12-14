// Tetris Game on 2 LED matrices

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include "LedControl.h" 

// Hardware Pins
const uint8_t PIN_RS = 9;
const uint8_t PIN_EN = 8;
const uint8_t PIN_D4 = 7;
const uint8_t PIN_D5 = 6;
const uint8_t PIN_D6 = 5;
const uint8_t PIN_D7 = 4;
const uint8_t PIN_DIN = 11;
const uint8_t PIN_CLK = 13;
const uint8_t PIN_LOAD = 12;
const uint8_t PIN_JOY_X = A0;
const uint8_t PIN_JOY_Y = A1;
const uint8_t PIN_LDR = A5;
const uint8_t PIN_V0 = 10;
const uint8_t PIN_BTN = 2;
const uint8_t PIN_BUZZER = 3;
 
const int MSG_DISPLAY_MS = 3000; 
const int BTN_DEBOUNCE_MS = 300;
const int JOY_DEBOUNCE_MS = 200;
const uint8_t PHYSICS_STEP_MS = 50;

// Game States
enum GameState {
  STATE_MENU, 
  STATE_PLAYING, 
  STATE_HIGHSCORE, 
  STATE_GAME_FINISHED,
  STATE_HTP, // HTP = How To Play
  STATE_CREDITS,
  STATE_ADJUST_LCD,
  STATE_ADJUST_MATRIX,
  STATE_STARTUP,
  STATE_NAME_CHANGE
};

const   uint8_t NUMBER_OF_PIECES = 7;
const   uint8_t NUMBER_OF_ROTATIONS = 4;

// Piece type indices
const uint8_t I_PIECE_INDEX  = 0;
const   uint8_t J_PIECE_INDEX  = 1;
const   uint8_t L_PIECE_INDEX  = 2;
const   uint8_t O_PIECE_INDEX  = 3;
const   uint8_t S_PIECE_INDEX  = 4;
const   uint8_t T_PIECE_INDEX  = 5;
const   uint8_t Z_PIECE_INDEX  = 6;


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

// Arrow pattern for indicating in what direction you can use the joystick
const uint8_t arrow[8][8] = {
  {0, 0, 0, 1, 1, 0, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0},
  {1, 1, 1, 1, 1, 1, 1, 1},
  {0, 1, 1, 1, 1, 1, 1, 0},
  {0, 0, 1, 1, 1, 1, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0}
};

// High score data structure
struct HighScoreEntry {
  int score;
  int completionTime;
  char playerName[16];
};

// Buzzer sound types
enum Buzz {
  NO_BUZZ,
  MOVE_BUZZ,
  GAME_OVER_BUZZ,
  SCORE_BUZZ,
  DIFFICULTY_UP_BUZZ
};

// Handles all game logic, physics, and state management
class GameModel {
public:

  uint8_t numberOfPages = 10;
  uint8_t pageCursor = 0;
  uint8_t lastPage = 0;

  char* howToPlayText[10][2] = {
    {"OBJECTIVE:", "MOVE PIECES,"},
    {"FILL ROWS", "TO SCORE POINTS"},
    {"CONTROLS:", "1. JOYSTICK:"},
    {"UP = ROTATE", "PIECE"},
    {"RIGHT = MOVE", "PIECE TO RIGHT"},
    {"LEFT = MOVE", "PIECE TO LEFT"},
    {"DOWN = GOES", "DOWN FASTER"},
    {"2. LDR:", "COVER LDR TO"},
    {"SAVE PIECE", "FOR LATER"},
    {"COVER AGAIN", "TO GET IT BACK"}
  };

  unsigned long lastScrollTime;
  uint16_t scrollDelay = 600;

  uint8_t scrollIndex = 0;

  char* creditsText[2] = {
    "CREATOR: MATEI CHISCAN                                  ",
    "REPO: github.com/gramada01/MatrixTetrisProject          "
  };

  bool screenChange = false;

  bool startupOn = true;
  unsigned long startupTimeCheck;
  uint16_t startupDuration = 5000;

  char playerName[16] = "MATEI";
  char playerNameCopy[16];
  const uint8_t playerNameAddress = 100;

  uint8_t letterCursor = 0;

  const uint8_t numberOfNameChangeOptions = 16;
  unsigned long lastLetterBlink;
  const uint16_t letterBlinkDelay = 500;
  long int lastPieceBlink;
  bool letterBlinkOn = false;

  int score = 0;
  uint8_t scoreMultiplier = 1;
  uint8_t difficultyIncreaseDelay = 60;
  uint8_t currentDifficulty = 0;
  uint8_t maxDifficulty = 4;

  Buzz buzz = NO_BUZZ;

  const uint8_t PIECE_WINDOW_SIZE = 4;
  const uint8_t PIECE_STORE_SIZE = 16;
  static const   uint8_t WINDOW_NUMBER_OF_ROWS = 16;
  static const   uint8_t WINDOW_NUMBER_OF_COLUMNS = 8;

  uint8_t pieceSpawnPoint[2] = {0, 2}; //LED matrix coordinates for piece spawning
  int pieceWindowAnchor[2]; //coordinates for the top right corner of the window that holds the piece
  uint8_t currentPieceIndex = 0;
  uint8_t currentRotation = 0;
  uint8_t nextPieceIndex = 0;
  uint8_t savedPieceIndex = 10;

  bool justSavedPiece = false;

  uint8_t rowBlockCounter[WINDOW_NUMBER_OF_ROWS];
  uint8_t gameWindow[WINDOW_NUMBER_OF_ROWS][WINDOW_NUMBER_OF_COLUMNS];

  unsigned long lastAutoMoveTime; //the last time the piece moved down on its own
  int autoMoveWaitPeriod = 1000;
  bool pieceShouldBePlaced = false;

  GameState currentState = STATE_STARTUP;
  unsigned long gameStartTime;
  unsigned long stateStartTime; 
  int elapsedTime;
  
  int menuCursor = 0;
  static const uint8_t numberOfMenuOptions = 7;
  
  int hsRankViewer = 0;
  HighScoreEntry highScores[3];

  uint8_t matrixBrightness = 1;
  uint8_t maxMatrixBrightness = 15;
  uint8_t matrixBrightnessAddress = 200;

  int16_t LCDBrightness = 1023;
  uint16_t maxLCDBrightness = 1023;
  uint8_t LCDBrightnessAddress = 250;

  // EEPROM brightness management
  void loadLCDBrightness(){
    EEPROM.get(LCDBrightnessAddress, LCDBrightness);
  }

  void saveLCDBrightness(){
    EEPROM.put(LCDBrightnessAddress, LCDBrightness);
  }

  void loadMatrixBrightness(){
    EEPROM.get(matrixBrightnessAddress, matrixBrightness);
  }

  void saveMatrixBrightness(){
    EEPROM.put(matrixBrightnessAddress, matrixBrightness);
  }

  // Handle buzzer sounds based inputs and events
  void buzzHandle(){
    if (buzz == NO_BUZZ) return;

    switch (buzz) {
      case MOVE_BUZZ:
        tone(PIN_BUZZER, 700, 20); 
        break;

      case SCORE_BUZZ:
        tone(PIN_BUZZER, 1500, 300); 
        break;
      
      case DIFFICULTY_UP_BUZZ:
        tone(PIN_BUZZER, 2500, 500); 
        break;

      case GAME_OVER_BUZZ:
        tone(PIN_BUZZER, 200, 2000); 
        break;
    }

    buzz = NO_BUZZ;
  }

  GameModel() {
    currentState = STATE_STARTUP;
    stateStartTime = 0;
    
    loadHighScores();
  }

  // Initialize game state for a new game
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
    scoreMultiplier = 1;
    autoMoveWaitPeriod = 1000;
    currentDifficulty = 0;

    gameStartTime = millis();
    elapsedTime = 0;
    currentState = STATE_PLAYING;
    justSavedPiece = false;
    savedPieceIndex = 10;

    lastPieceBlink = millis();

    lastAutoMoveTime = millis();
  }

  // Clear a row and shift all rows above it down
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
  
  // Check for completed rows and clear them, update score
  void checkRows(){
    uint8_t counter = 0;
    for(int i = WINDOW_NUMBER_OF_ROWS - 1; i >= 0; i--){
      
      if(rowBlockCounter[i] == WINDOW_NUMBER_OF_COLUMNS){
        
        counter++;
        buzz = SCORE_BUZZ;
        
        clearRows(i);
        i++; 
      }
    }

    if(counter > 0)
      score += counter*counter*scoreMultiplier;
  }

  // Spawn the next piece at the top of the board
  void setupNextPiece(){
    currentRotation = 0;
    currentPieceIndex = nextPieceIndex;
    pieceWindowAnchor[0] = pieceSpawnPoint[0];
    pieceWindowAnchor[1] = pieceSpawnPoint[1];
    nextPieceIndex = random(0, NUMBER_OF_PIECES);
    if(colisionCheck(pieces[currentPieceIndex][currentRotation], pieceWindowAnchor[0], pieceWindowAnchor[1])){
      buzz = GAME_OVER_BUZZ;
      gameFinished();
      return;
    }
    justSavedPiece = false;
    screenChange = true;
    lastPieceBlink = millis();
    lastAutoMoveTime = millis();
  }

  // Save/swap current piece with saved piece (LDR trigger)
  void savePiece(){
    if(!justSavedPiece){
      uint8_t aux = currentPieceIndex;
      if(savedPieceIndex < NUMBER_OF_PIECES)
        currentPieceIndex = savedPieceIndex;
      else{
        currentPieceIndex = nextPieceIndex;
        nextPieceIndex = random(0, NUMBER_OF_PIECES);
      }
        
      savedPieceIndex = aux;
      currentRotation = 0;
      pieceWindowAnchor[0] = pieceSpawnPoint[0];
      pieceWindowAnchor[1] = pieceSpawnPoint[1];
      if(colisionCheck(pieces[currentPieceIndex][currentRotation], pieceWindowAnchor[0], pieceWindowAnchor[1])){
        buzz = GAME_OVER_BUZZ;
        gameFinished();
        return;
      }
      justSavedPiece = true;
      screenChange = true;
      lastPieceBlink = millis();
      lastAutoMoveTime = millis();
    }
  }

  // Place current piece into the game board and check for row clears
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

  // Check if piece would collide with walls or existing blocks
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

  // Rotate piece with wall kick system (tries multiple positions)
  void rotatePiece(){
    uint8_t nextRotIndex = (currentRotation + 1) % NUMBER_OF_ROTATIONS;
    uint16_t nextShape = pieces[currentPieceIndex][nextRotIndex];

    const   uint8_t NUMBER_OF_TESTS = 4;
    const   uint8_t NUMBER_OF_COORDINATES = 2;

    int testOffsets[NUMBER_OF_TESTS][NUMBER_OF_COORDINATES] = {
      {0, 0},   // Attempt 1: Standard Rotation
      {1, 0},   // Attempt 2: Wall Kick Right (Shift +1)
      {-1, 0},  // Attempt 3: Wall Kick Left (Shift -1)
      {0, -1}   // Attempt 4: Floor Kick (Shift Up)
    };

    for(uint8_t i = 0; i < NUMBER_OF_TESTS; i++) {
      
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

  // Movement functions - check collision before moving
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

  // Main game loop logic - handles piece falling, difficulty, timing
  void gameLogic() {
    if (currentState != STATE_PLAYING) return;

    elapsedTime = (millis() - gameStartTime) / 1000;
    
    if(currentDifficulty < maxDifficulty){
      if(elapsedTime > (currentDifficulty + 1) * difficultyIncreaseDelay){
        currentDifficulty++;
        scoreMultiplier++;
        autoMoveWaitPeriod -= 150;
        buzz = DIFFICULTY_UP_BUZZ;
      }
    }

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

  // End game and save high score
  void gameFinished(){
    saveHighScores(score, elapsedTime);
    currentState = STATE_GAME_FINISHED;
    stateStartTime = millis();
  }

  // Load high scores from EEPROM
  void loadHighScores(){
    for (int i = 0; i < 3; ++i) {
      EEPROM.get(i * sizeof(HighScoreEntry), highScores[i]);
      
      if (highScores[i].score < 0) {
        highScores[i].score = 0;
        highScores[i].completionTime = 0;

        for(uint8_t j = 0; j < 16; ++j){
          highScores[i].playerName[j] = '-';
        }
      }
    }
  }

  // Save new high score if it is in top 3
  void saveHighScores(int score, int time){
    for (uint8_t i = 0; i < 3; ++i) {
      if (score > highScores[i].score) {
        
        for (int j = 2; j > i; j--) {
            highScores[j] = highScores[j-1];
        }

        highScores[i].score = score;
        highScores[i].completionTime = time;

        for(uint8_t j = 0; j < 16; ++j){
          highScores[i].playerName[j] = playerName[j];
        }
        

        for (int k = 0; k < 3; ++k) {
            EEPROM.put(k * sizeof(HighScoreEntry), highScores[k]);
        }
        
        break;
      }
    }
  }

  // Player name management
  void loadName(){
    EEPROM.get(playerNameAddress, playerName);
  }

  void saveName(){
    EEPROM.put(playerNameAddress, playerName);
  }

  const char* menuOptions[numberOfMenuOptions] = {
    "PLAY",
    "HIGHSCORES",
    "HOW TO PLAY",
    "CREDITS",
    "ADJUST LCD",
    "ADJUST MATRIX",
    "SET PLAYER NAME"
  };
};

// Renderer interface
class IRenderer {
public:
  virtual void init() = 0;
  virtual void render(GameModel& model) = 0;
};

// Handles all display rendering (LCD and LED matrices)
class GameRenderer : public IRenderer {
private:
  GameState lastState;
  uint8_t numberOfMatrix = 2;
  const   uint8_t LCD_COLS = 16;
  const   uint8_t LCD_ROWS = 2;
  uint8_t lastMenu = 0;
  uint8_t blinkValue = 0;
  uint8_t pieceBLinkTime = 200;
public:
  LiquidCrystal lcd;
  LedControl lc;

  GameRenderer() : lcd(PIN_RS, PIN_EN, PIN_D4, PIN_D5, PIN_D6, PIN_D7), lc(PIN_DIN, PIN_CLK, PIN_LOAD, numberOfMatrix) {
    lastState = STATE_STARTUP;
  }

  void init() override{
    lcd.begin(LCD_COLS, LCD_ROWS);
    for(uint8_t i = 0 ; i < numberOfMatrix; ++i) {
      lc.shutdown(i, false);
      
      lc.setIntensity(i, 1); 
      
      lc.clearDisplay(i);
    }
    lcd.clear();
  }

  // Update LED matrices with game board and current piece
  void updateMatrix(GameModel& model){
    
    if(millis() - model.lastPieceBlink > pieceBLinkTime){
      blinkValue = !blinkValue;
      model.lastPieceBlink = millis();
    }

    for(int row = 0; row < model.WINDOW_NUMBER_OF_ROWS; row++){
      for(int col = 0; col < model.WINDOW_NUMBER_OF_COLUMNS; col++){
        
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
    for(int i = 0; i < model.WINDOW_NUMBER_OF_ROWS; i++){
      if(pieceWindow & (1 << (15 - i))){
        int col = i % model.PIECE_WINDOW_SIZE;
        int row = i / model.PIECE_WINDOW_SIZE;
        
        int worldX = model.pieceWindowAnchor[1] + col;
        int worldY = model.pieceWindowAnchor[0] + row;

        int address;
        int realRow;

        if(worldY < 8){
          address = 0;
          realRow = worldY;
        }else{
          address = 1;
          realRow = worldY - 8; 
        }

        lc.setLed(address, realRow, worldX, blinkValue);
      }
    }
  }

  // Main render function - handles all game states
  void render(GameModel& model) override{
    if(model.currentState != lastState){
      lcd.clear();  
      lastState = model.currentState;
    }

    if(model.currentState == STATE_STARTUP){
      lcd.setCursor(0, 0);
      lcd.print("TETRIS! PLAYER:");
      lcd.setCursor(0, 1);
      lcd.print(model.playerName);
      return;
    }

    if(model.currentState == STATE_MENU){
      if(lastMenu != model.menuCursor)
        lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print(model.menuOptions[model.menuCursor]);
      lastMenu = model.menuCursor;

      for(int8_t i = 7; i >= 0; --i){
        for(uint8_t j = 0; j < 8; ++j){
          lc.setLed(0, 7 - i, j, arrow[i][j]);
        }
      }

      for(uint8_t i = 0; i < 8; ++i){
        for(uint8_t j = 0; j < 8; ++j){
          lc.setLed(1, i, j, arrow[i][j]);
        }
      }

      return;
    }
    
    if(model.currentState == STATE_HIGHSCORE){
      if(model.screenChange){
        lcd.clear();
      }
      lcd.setCursor(0, 0);
      lcd.print("#"); lcd.print(model.hsRankViewer + 1);
      lcd.print(' ');
      lcd.print("S:"); lcd.print(model.highScores[model.hsRankViewer].score);
      lcd.print(" T:"); lcd.print(model.highScores[model.hsRankViewer].completionTime);
      lcd.setCursor(0, 1);
      lcd.print(model.highScores[model.hsRankViewer].playerName);

      if(model.hsRankViewer == 0){
        lc.clearDisplay(0);

        for(uint8_t i = 0; i < 8; ++i){
          for(uint8_t j = 0; j < 8; ++j){
            lc.setLed(1, i, j, arrow[i][j]);
          }
        }
      }
      else if(model.hsRankViewer == 2){
        for(int8_t i = 7; i >= 0; --i){
          for(uint8_t j = 0; j < 8; ++j){
            lc.setLed(0, 7 - i, j, arrow[i][j]);
          }
        }

        lc.clearDisplay(1);
      }
      else{
        for(int8_t i = 7; i >= 0; --i){
          for(uint8_t j = 0; j < 8; ++j){
            lc.setLed(0, 7 - i, j, arrow[i][j]);
          }
        }

        for(uint8_t i = 0; i < 8; ++i){
          for(uint8_t j = 0; j < 8; ++j){
            lc.setLed(1, i, j, arrow[i][j]);
          }
        }
      }
      return;
    }
    
    if(model.currentState == STATE_GAME_FINISHED) {
      lcd.setCursor(0, 0); lcd.print("GAME FINISHED!");
      lcd.setCursor(0, 1); lcd.print("S:"); lcd.print(model.score);
      lcd.print(" T:"); lcd.print(model.elapsedTime); lcd.print("s");
      return;
    }

    if(model.currentState == STATE_HTP){
      if(model.lastPage != model.pageCursor)
        lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print(model.howToPlayText[model.pageCursor][0]);

      lcd.setCursor(0, 1);
      lcd.print(model.howToPlayText[model.pageCursor][1]);

      if(model.pageCursor == 0){
        lc.clearDisplay(0);

        for(uint8_t i = 0; i < 8; ++i){
          for(uint8_t j = 0; j < 8; ++j){
            lc.setLed(1, i, j, arrow[i][j]);
          }
        }
      }
      else if(model.pageCursor == model.numberOfPages - 1){
        for(int8_t i = 7; i >= 0; --i){
          for(uint8_t j = 0; j < 8; ++j){
            lc.setLed(0, 7 - i, j, arrow[i][j]);
          }
        }

        lc.clearDisplay(1);
      }
      else{
        for(int8_t i = 7; i >= 0; --i){
          for(uint8_t j = 0; j < 8; ++j){
            lc.setLed(0, 7 - i, j, arrow[i][j]);
          }
        }

        for(uint8_t i = 0; i < 8; ++i){
          for(uint8_t j = 0; j < 8; ++j){
            lc.setLed(1, i, j, arrow[i][j]);
          }
        }
      }
    }

    if(model.currentState == STATE_CREDITS){
      for(uint8_t i = 0; i < 16; ++i){
        lcd.setCursor(i, 0);
        lcd.print(model.creditsText[0][(model.scrollIndex + i) % strlen(model.creditsText[0])]);
      }

      for(uint8_t i = 0; i < 16; ++i){
        lcd.setCursor(i, 1);
        lcd.print(model.creditsText[1][(model.scrollIndex + i) % strlen(model.creditsText[1])]);
      }
    }

    if(model.currentState == STATE_ADJUST_LCD){
      lcd.setCursor(0, 0);
      lcd.print("Brightness:");
      lcd.setCursor(0, 1);
      if(model.LCDBrightness == 0)
        lcd.print("MIN");
      else if(model.LCDBrightness == model.maxLCDBrightness)
        lcd.print("MAX");
      else
        lcd.print(model.LCDBrightness);
    }

    if(model.currentState == STATE_ADJUST_MATRIX){
      lcd.setCursor(0, 0);
      lcd.print("Brightness:");
      lcd.setCursor(0, 1);
      if(model.matrixBrightness == 1)
        lcd.print("MIN");
      else if(model.matrixBrightness == model.maxMatrixBrightness)
        lcd.print("MAX");
      else
        lcd.print(model.matrixBrightness);
    }

    if(model.currentState == STATE_NAME_CHANGE){
      if(millis() - model.lastLetterBlink > model.letterBlinkDelay){
        model.letterBlinkOn = !model.letterBlinkOn;
        model.lastLetterBlink = millis();
      }

      for(int i = 0; i < model.numberOfNameChangeOptions; ++i){
        lcd.setCursor(i, 0);
        if(i == model.letterCursor){
          if(model.letterBlinkOn){
            lcd.print(model.playerNameCopy[i]);
          }
          else
            lcd.print('_');
        }
        else
          lcd.print(model.playerNameCopy[i]);
      }

      lcd.setCursor(0, 1);
      
      if(model.letterCursor == model.numberOfNameChangeOptions){
        if(model.letterBlinkOn){
          lcd.print("    ");
        }
        else
          lcd.print("BACK");
      }
      else
        lcd.print("BACK");
      return;
    }

    // Game playing state rendering
    if(model.currentState == STATE_PLAYING){
      if(model.screenChange){
        lcd.clear();
      }

      lcd.setCursor(0, 0);

      lcd.print("N:");

      switch(model.nextPieceIndex){
        case I_PIECE_INDEX:
          lcd.setCursor(2, 0);
          lcd.print("####");
          break;

        case J_PIECE_INDEX:
          lcd.setCursor(2, 0);
          lcd.print("###");
          lcd.setCursor(4, 1);
          lcd.print('#');
          break;

        case L_PIECE_INDEX:
          lcd.setCursor(4, 0);
          lcd.print('#');
          lcd.setCursor(2, 1);
          lcd.print("###");
          break; 

        case O_PIECE_INDEX:
          lcd.setCursor(2, 0);
          lcd.print("##");
          lcd.setCursor(2, 1);
          lcd.print("##");
          break;
          
        case S_PIECE_INDEX:
          lcd.setCursor(2, 0);
          lcd.print(" ##");
          lcd.setCursor(2, 1);
          lcd.print("##");
          break;

        case T_PIECE_INDEX:
          lcd.setCursor(2, 0);
          lcd.print("###");
          lcd.setCursor(2, 1);
          lcd.print(" #");
          break;

        case Z_PIECE_INDEX:
          lcd.setCursor(2, 0);
          lcd.print("##");
          lcd.setCursor(2, 1);
          lcd.print(" ##");
          break;

        default:
          lcd.setCursor(2, 0);
          lcd.print("NULL");
          break;
      }

      lcd.setCursor(8, 0);

      lcd.print("S:");

      switch(model.savedPieceIndex){
        case I_PIECE_INDEX:
          lcd.setCursor(10, 0);
          lcd.print("####");
          break;

        case J_PIECE_INDEX:
          lcd.setCursor(10, 0);
          lcd.print("###");
          lcd.setCursor(12, 1);
          lcd.print('#');
          break;

        case L_PIECE_INDEX:
          lcd.setCursor(12, 0);
          lcd.print('#');
          lcd.setCursor(10, 1);
          lcd.print("###");
          break; 

        case O_PIECE_INDEX:
          lcd.setCursor(10, 0);
          lcd.print("##");
          lcd.setCursor(10, 1);
          lcd.print("##");
          break;
          
        case S_PIECE_INDEX:
          lcd.setCursor(10, 0);
          lcd.print(" ##");
          lcd.setCursor(10, 1);
          lcd.print("##");
          break;

        case T_PIECE_INDEX:
          lcd.setCursor(10, 0);
          lcd.print("###");
          lcd.setCursor(10, 1);
          lcd.print(" #");
          break;

        case Z_PIECE_INDEX:
          lcd.setCursor(10, 0);
          lcd.print("##");
          lcd.setCursor(10, 1);
          lcd.print(" ##");
          break;

        default:
          lcd.setCursor(10, 0);
          lcd.print("NULL");
          break;
      }

      updateMatrix(model);
    }
  }
};

// Handles input processing and game loop
class GameController {
private:
  GameModel& model;
  GameRenderer* renderer;
  unsigned long lastUpdate;
  unsigned long lastInputTime;
  const uint16_t MAX_THRESHOLD = 700;
  const uint16_t MIN_THRESHOLD = 300;
  const uint16_t LDR_THRESHOLD = 150; // Light sensor threshold for piece save

public:
  GameController(GameModel& m, GameRenderer* r) : model(m), renderer(r) {
    lastUpdate = 0;
    lastInputTime = 0;
  }

  void setup() {
    renderer->init();
  }

  void loop() {
    if(model.startupOn){
      model.startupTimeCheck = millis();
      model.startupOn = false;
      model.loadName();
      model.loadLCDBrightness();
      model.loadMatrixBrightness();
      analogWrite(PIN_V0, model.LCDBrightness);
      renderer->lc.setIntensity(0, model.matrixBrightness);
      renderer->lc.setIntensity(1, model.matrixBrightness);
    }

    if(model.currentState == STATE_STARTUP){
      
      if(millis() - model.startupTimeCheck > model.startupDuration){
        model.currentState = STATE_MENU;
      }
    }

    if(millis() - lastUpdate > PHYSICS_STEP_MS) {
      model.buzzHandle();
      handleInput();
      model.gameLogic();
      renderer->render(model); 
      lastUpdate = millis();
    }
  }

private:
  // Process all user input based on current game state
  void handleInput() {
    // Read joystick and button inputs
    int xVal = analogRead(PIN_JOY_X);
    int yVal = analogRead(PIN_JOY_Y);
    bool btn = (digitalRead(PIN_BTN) == LOW);
    
    // Input debouncing
    if(millis() - lastInputTime < JOY_DEBOUNCE_MS) return;

    if(model.currentState == STATE_MENU) {
      
      if(xVal < MIN_THRESHOLD){
        if(model.menuCursor < model.numberOfMenuOptions - 1)
          model.menuCursor++;
        else
          model.menuCursor = 0;
        model.buzz = MOVE_BUZZ;
        lastInputTime = millis();
      }
      if(xVal > MAX_THRESHOLD){
        if(model.menuCursor > 0)
          model.menuCursor--;
        else
          model.menuCursor = model.numberOfMenuOptions - 1;
        model.buzz = MOVE_BUZZ;
        lastInputTime = millis();
      }
      
      if(btn && millis() - lastInputTime > BTN_DEBOUNCE_MS) {
        switch(model.menuCursor){
          case 0:
            model.initGame();
            lastInputTime = millis();
            break;
          
          case 1:
            model.currentState = STATE_HIGHSCORE;
            lastInputTime = millis();
            break;

          case 2:
            model.currentState = STATE_HTP;
            lastInputTime = millis();
            break;

          case 3:
            model.currentState = STATE_CREDITS;
            lastInputTime = millis();
            model.lastScrollTime = millis();
            break;

          case 4:
            model.currentState = STATE_ADJUST_LCD;
            lastInputTime = millis();
            break;

          case 5:
            model.currentState = STATE_ADJUST_MATRIX;
            lastInputTime = millis();
            break;

          case 6:
            model.currentState = STATE_NAME_CHANGE;
            for(int i = 0; i < 16; ++i)
              model.playerNameCopy[i] = ' ';
            lastInputTime = millis();
            model.lastLetterBlink = millis();
            break;
        }
      }
    }
    else if(model.currentState == STATE_HIGHSCORE){
      if(model.screenChange)
        model.screenChange = false;

      if(xVal > MAX_THRESHOLD && model.hsRankViewer > 0){
        model.hsRankViewer--;
        model.screenChange = true;
        model.buzz = MOVE_BUZZ;
        lastInputTime = millis();
      }
      if(xVal < MIN_THRESHOLD && model.hsRankViewer < 2) {
        model.hsRankViewer++;
        model.screenChange = true;
        lastInputTime = millis();
        model.buzz = MOVE_BUZZ;
      }
      
      if(btn && millis() - lastInputTime > BTN_DEBOUNCE_MS){
          model.currentState = STATE_MENU;
          lastInputTime = millis();
      }
    }
    else if(model.currentState == STATE_HTP){
      model.lastPage = model.pageCursor;

      if(xVal > MAX_THRESHOLD){
        if(model.pageCursor > 0){
          model.pageCursor--;
          model.buzz = MOVE_BUZZ;
          lastInputTime = millis();
        }    
      }

      if(xVal < MIN_THRESHOLD){
        if(model.pageCursor < model.numberOfPages - 1){
          model.pageCursor++;
          model.buzz = MOVE_BUZZ;
          lastInputTime = millis();
        }
      }

      if(btn && millis() - lastInputTime > BTN_DEBOUNCE_MS){
          model.currentState = STATE_MENU;
          lastInputTime = millis();
      }
    }
    else if(model.currentState == STATE_CREDITS){
      if(millis() - model.lastScrollTime > model.scrollDelay){
        model.scrollIndex++;
        model.lastScrollTime = millis();
      }

      if(btn && millis() - lastInputTime > BTN_DEBOUNCE_MS){
          model.currentState = STATE_MENU;
          lastInputTime = millis();
      }
    }
    else if(model.currentState == STATE_ADJUST_LCD){
      if(yVal < MIN_THRESHOLD){
        if(model.LCDBrightness > 0){
          if(model.LCDBrightness - 10 < 0)
            model.LCDBrightness = 0;
          else
            model.LCDBrightness -= 10;
          model.buzz = MOVE_BUZZ;
          renderer->lcd.clear();
        }
        lastInputTime = millis();
      }

      if(yVal > MAX_THRESHOLD){
        if(model.LCDBrightness < model.maxLCDBrightness){
          if(model.LCDBrightness + 10 > model.maxLCDBrightness)
            model.LCDBrightness = model.maxLCDBrightness;
          else
            model.LCDBrightness += 10;
          model.buzz = MOVE_BUZZ;
          renderer->lcd.clear();
        }
        lastInputTime = millis();
      }

      analogWrite(PIN_V0, model.LCDBrightness);

      if(btn && millis() - lastInputTime > BTN_DEBOUNCE_MS){
        model.saveLCDBrightness();
        model.currentState = STATE_MENU;
        lastInputTime = millis();
      }
    }
    else if(model.currentState == STATE_ADJUST_MATRIX){
      if(yVal < MIN_THRESHOLD){
        if(model.matrixBrightness > 1){
          model.matrixBrightness -= 1;
          renderer->lcd.clear();
          model.buzz = MOVE_BUZZ;
        }
        lastInputTime = millis();
      }

      if(yVal > MAX_THRESHOLD){
        if(model.matrixBrightness < model.maxMatrixBrightness){
          model.matrixBrightness += 1;
          renderer->lcd.clear();
          model.buzz = MOVE_BUZZ;
        }
        lastInputTime = millis();
      }

      renderer->lc.setIntensity(0, model.matrixBrightness);
      renderer->lc.setIntensity(1, model.matrixBrightness);

      if(btn && millis() - lastInputTime > BTN_DEBOUNCE_MS){
        model.saveMatrixBrightness();
        model.currentState = STATE_MENU;
        lastInputTime = millis();
      }
    }
    else if(model.currentState == STATE_NAME_CHANGE){
      if(xVal > MAX_THRESHOLD){
        if(model.letterCursor != model.numberOfNameChangeOptions){
          if(model.playerNameCopy[model.letterCursor] >= 'A' && model.playerNameCopy[model.letterCursor] < 'Z')
            model.playerNameCopy[model.letterCursor]++;
          else{
            if(model.playerNameCopy[model.letterCursor] == 'Z')
              model.playerNameCopy[model.letterCursor] = ' ';
            else
              model.playerNameCopy[model.letterCursor] = 'A';
          }           
        }
        model.buzz = MOVE_BUZZ;
        lastInputTime = millis();
      }

      if(xVal < MIN_THRESHOLD){
        if(model.letterCursor != model.numberOfNameChangeOptions){
          if(model.playerNameCopy[model.letterCursor] > 'A' && model.playerNameCopy[model.letterCursor] <= 'Z')
            model.playerNameCopy[model.letterCursor]--;
          else{
            if(model.playerNameCopy[model.letterCursor] == 'A')
              model.playerNameCopy[model.letterCursor] = ' ';
            else
              model.playerNameCopy[model.letterCursor] = 'Z';
          }
        }
        model.buzz = MOVE_BUZZ;
        lastInputTime = millis();         
      }

      if(yVal < MIN_THRESHOLD){
        if(model.letterCursor == 0){
          model.letterCursor = model.numberOfNameChangeOptions;
        }
        else
          model.letterCursor--;
        model.buzz = MOVE_BUZZ;
        lastInputTime = millis();
      }

      if(yVal > MAX_THRESHOLD){
        if(model.letterCursor == model.numberOfNameChangeOptions){
          model.letterCursor = 0;
        }
        else
          model.letterCursor++;
        model.buzz = MOVE_BUZZ;
        lastInputTime = millis();
      }

      if(btn && millis() - lastInputTime > BTN_DEBOUNCE_MS) {
        if(model.letterCursor == model.numberOfNameChangeOptions)
          model.currentState = STATE_MENU;
        else{
          for(int i = 0; i < model.numberOfNameChangeOptions; ++i){
            model.playerName[i] = model.playerNameCopy[i];
          }

          model.saveName();
          model.currentState = STATE_MENU;
        }
        lastInputTime = millis();
      }
    }
    else if(model.currentState == STATE_GAME_FINISHED){
      if(millis() - model.stateStartTime > MSG_DISPLAY_MS)
        model.currentState = STATE_MENU;
    }
    else if(model.currentState == STATE_PLAYING){
      if(model.screenChange){
        model.screenChange = false;
      }
      // Check LDR for piece save feature
      uint16_t ldrValue = analogRead(PIN_LDR);

      Serial.println(ldrValue);

      if(ldrValue < LDR_THRESHOLD){
        if(!model.justSavedPiece)
          model.savePiece();
      }
      
      if (millis() - lastInputTime < JOY_DEBOUNCE_MS) return; 

      // Handle joystick movement during gameplay
      bool moved = false;

      if(xVal > MAX_THRESHOLD) {
        model.rotatePiece();
        model.buzz = MOVE_BUZZ;
        moved = true;
      }

      if(xVal < MIN_THRESHOLD) {
        model.moveDown();
        model.buzz = MOVE_BUZZ;
        moved = true;
      }

      if(yVal < MIN_THRESHOLD) {
        model.moveLeft();
        model.buzz = MOVE_BUZZ;
        moved = true;
      }

      if(yVal > MAX_THRESHOLD) {
        model.moveRight();
        model.buzz = MOVE_BUZZ;
        moved = true;
      }

      if(moved) {
        lastInputTime = millis();
      }
    }
  }
};

// Global game objects
GameModel model;
GameRenderer gameRenderer;
GameController controller(model, &gameRenderer);

// Arduino setup function
void setup() {
  pinMode(PIN_JOY_X, INPUT);
  pinMode(PIN_JOY_Y, INPUT);
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_LDR, INPUT);
  pinMode(PIN_V0, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  controller.setup();
  Serial.begin(9600);
}

// Arduino main loop
void loop() {
  controller.loop();
}