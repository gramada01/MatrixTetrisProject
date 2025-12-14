# Matrix Tetris Project

## 1. Task:

Create a Tetris game implemented on Arduino with 2 8x8 LED matrices for gameplay and a 16x2 LCD display for game information. The game includes mechanics such as piece rotation with wall kick system, row clearing, progressive difficulty, high score tracking, and an LDR-based piece saving mechanism. The system includes a menu system with settings, player name customization, and persistent storage of high scores and settings.

## 2. Implementation:

### Hardware Components:

1. Arduino Uno: Main controller for the game system
2. 2x 8x8 LED Matrices with 2 MAX7219-based shields daisychained: Display the 16x8 game board (stacked vertically) and navigation arrows
3. 16x2 LCD Display: Shows game information, menu options, next piece, saved piece, and high scores
4. Joystick: X-axis and Y-axis for movement/rotation, button for selection
5. LDR (Light Dependent Resistor): Used for piece save/swap feature (cover to save, cover again to retrieve)
6. Buzzer: Audio feedback for game events (movement, scoring, difficulty increase, game over)
7. Potentiometer (V0): LCD contrast control via PWM

### Pin Map:

| Pin | Component | Function |
|-----|-----------|----------|
| 2 | Joystick Button | Select/Confirm (with internal pull-up) |
| 3 | Buzzer | Audio feedback (PWM) |
| 4-7 | LCD | Data lines D7-D4 (4-bit mode) |
| 8 | LCD | Enable pin (EN) |
| 9 | LCD | Register Select (RS) |
| 10 | LCD | Contrast control (V0, PWM) |
| 11 | LED Matrix | Data input (DIN) |
| 12 | LED Matrix | Load/CS pin (LOAD) |
| 13 | LED Matrix | Clock pin (CLK) |
| A0 | Joystick | X-axis (left/right movement, rotation) |
| A1 | Joystick | Y-axis (up/down movement) |
| A5 | LDR | Light sensor for piece save feature |

### Game Features:

#### Gameplay:
- 7 Tetris Pieces: I, J, L, O, S, T, Z pieces with 4 rotation states each
- 16x8 Game Board: Displayed across 2 stacked 8x8 LED matrices
- Wall Kick System: Rotation system that attempts 4 different positions when rotating near walls or previously placed pieces
- Row Clearing: Complete rows are cleared and blocks above fall down
- Progressive Difficulty: Game speed increases every 60 seconds (up to 4 difficulty levels)
- Scoring System: Points awarded based on number of rows cleared simultaneously (rows² × multiplier, multiplier is based on difficulty level)
- Piece Blinking: Current falling piece blinks at 200ms intervals for visibility

#### Unique Features:
- LDR Piece Save: Cover the LDR sensor to save current piece, cover again to retrieve it
- Next Piece Preview: Displayed on LCD screen during gameplay
- Saved Piece Display: Shows currently saved piece on LCD
- High Score System: Top 3 scores saved in EEPROM with player names and completion times

#### Menu System:
The game has 10 different states:
1. STARTUP: 5-second splash screen showing game name and player name
2. MENU: Main menu with 7 options (navigated with joystick)
3. PLAYING: Active gameplay state
4. HIGHSCORE: View top 3 high scores (navigable with joystick)
5. HOW TO PLAY: 10-page tutorial
6. CREDITS: Scrolling credits text
7. ADJUST LCD: Adjust LCD brightness (saved to EEPROM)
8. ADJUST MATRIX: Adjust LED matrix brightness (saved to EEPROM)
9. NAME CHANGE: Customize player name (16 characters, A-Z and space) (saved to EEPROM and used in higscore system)
10. GAME FINISHED: End game screen showing score and time

### Game Controls:

#### During Gameplay:
- Joystick X-axis > 700: Rotate piece
- Joystick X-axis < 300: Move piece down faster
- Joystick Y-axis < 300: Move piece left
- Joystick Y-axis > 700: Move piece right
- LDR Covered: Save/swap current piece

#### Menu Navigation:
- Joystick X-axis: Navigate menu options (left/right)
- Joystick Button: Select/Confirm
- Joystick Y-axis: Navigate in name editor and brightness settings

### Buzzer Feedback:

- MOVE_BUZZ: 700Hz, 20ms - Piece movement/rotation
- SCORE_BUZZ: 1500Hz, 300ms - Row cleared
- DIFFICULTY_UP_BUZZ: 2500Hz, 500ms - Difficulty level increased
- GAME_OVER_BUZZ: 200Hz, 2000ms - Game over

### Software Architecture:

The implementation uses object-oriented programming with a clean separation of concerns:

#### GameModel Class:
- Handles all game logic, physics, and state management
- Manages piece spawning, movement, rotation, and placement
- Implements collision detection and row clearing algorithms
- Handles scoring, difficulty progression, and timing
- Manages EEPROM storage for high scores, player name, and brightness settings

#### GameRenderer Class (implements IRenderer):
- Handles all display rendering operations
- Manages LCD output for all game states
- Updates LED matrices with game board and current piece
- Implements piece blinking animation
- Displays navigation arrows on LED matrices for menu states
- Handles scrolling text for credits

#### GameController Class:
- Coordinates game loop execution (50ms physics step)
- Processes all user input with debouncing
- Handles state-specific input logic
- Manages startup sequence and EEPROM loading
- Coordinates between model and renderer

### Algorithms:

#### Collision Detection:
- Checks piece boundaries against walls and existing blocks
- Uses bitwise operations to check piece shape against game board
- Validates piece position before movement/rotation

#### Wall Kick System:
- Attempts rotation at 4 different positions when standard rotation fails:
  1. Standard position (no offset)
  2. Shift right (+1 column)
  3. Shift left (-1 column)
  4. Shift up (-1 row)
- Allows pieces to rotate near walls and edges

#### Row Clearing:
- Scans board from bottom to top for complete rows
- Shifts all rows above cleared row down
- Updates row block counter for efficient checking
- Calculates score: rows_cleared² × difficulty_multiplier

#### Difficulty Progression:
- Increases every 60 seconds (difficultyIncreaseDelay)
- Maximum 4 difficulty levels
- Each level: speed increases (autoMoveWaitPeriod decreases by 150ms), score multiplier increases
- Initial speed: 1000ms per drop, minimum: 400ms per drop

### EEPROM Storage:

- High Scores: 3 entries stored at addresses 0-47 (each entry: score, time, 16-char name)
- Player Name: 16 characters at address 100
- LCD Brightness: 2 bytes at address 250 (0-1023)
- Matrix Brightness: 1 byte at address 200 (1-15)

### Timing Constants:

- Physics Step: 50ms
- Button Debounce: 300ms
- Joystick Debounce: 200ms
- Message Display: 3000ms
- Startup Duration: 5000ms
- Piece Blink: 200ms
- Letter Blink: 500ms
- Scroll Delay: 600ms

#### Setup:
![Setup1](/Setup1.jpeg "Setup1")
![Setup2](/Setup2.jpeg "Setup2")

#### [Video of project implementation](www.youtube.com/watch?si=UdX2BUxr0ekZxRZw&v=z8rKnogZTEY&feature=youtu.be)