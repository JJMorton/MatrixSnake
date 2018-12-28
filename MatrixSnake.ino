#include <LedControl.h>

#define MATRIX_SIZE 8

// Duration of each frame
#define FRAME_PERIOD 300

// Socket numbers that the buttons are connected to
#define BTN_UP    5
#define BTN_RIGHT 4
#define BTN_DOWN  3
#define BTN_LEFT  2

// Socket numbers for the matrix
#define MAT_DIN 12
#define MAT_CLK 10
#define MAT_CS  11


LedControl ledCtrl = LedControl(MAT_DIN, MAT_CLK, MAT_CS, 1);


// 0, 0 being top left
struct position
{
  int x;
  int y;
  struct position *next;
};

enum direction
{
  UP, RIGHT, DOWN, LEFT
};

struct snake
{
  struct position *head;
  int size;
  enum direction dir;
  bool alive;
};


struct position createFood();
struct snake createSnake();
struct position *createSegment(int x, int y);
void addToHead(struct snake *s, struct position *segment);
void removeTail(struct snake *s);
bool isValidPosition(struct snake *s);
bool isOppositeDirection(enum direction a, enum direction b);
struct position *movePosition(struct position *prevPos, enum direction dir);
void displayMatrix(const int devNum, byte *matrix);
void addToMatrix(struct position *pos, byte *matrix);


// The time that the last frame was rendered at
int lastFrame = 0;

// The position that the snake previously moved in
enum direction lastDirection = NULL;

// Initialise the snake and food
struct snake player = createSnake();
struct position food = createFood();


void setup()
{
  // Define the buttons as inputs
  pinMode(BTN_UP   , INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_DOWN , INPUT_PULLUP);
  pinMode(BTN_LEFT , INPUT_PULLUP);

  // Init matrix display
  ledCtrl.shutdown(0, false);
  ledCtrl.setIntensity(0, 3);
  ledCtrl.clearDisplay(0);
}

void loop()
{
  
  /*
   * HANDLE INPUTS
   */
  if (digitalRead(BTN_UP) == LOW && !isOppositeDirection(lastDirection, UP))
  {
    player.dir = UP;
  }
  else if (digitalRead(BTN_RIGHT) == LOW && !isOppositeDirection(lastDirection, RIGHT))
  {
    player.dir = RIGHT;
  }
  else if (digitalRead(BTN_DOWN) == LOW && !isOppositeDirection(lastDirection, DOWN))
  {
    player.dir = DOWN;
  }
  else if (digitalRead(BTN_LEFT) == LOW && !isOppositeDirection(lastDirection, LEFT))
  {
    player.dir = LEFT;
  }


  /*
   * GAME LOGIC
   */
  int currentTime = millis();

  // Only update the matrix every FRAME_PERIOD
  if (currentTime - lastFrame >= FRAME_PERIOD)
  {
    // If the player died last frame, respawn
    if (!player.alive) player = createSnake();

    /*
     * MOVING THE SNAKE
     */
    addToHead(&player, movePosition(player.head, player.dir));
    // If the player is over some food, don't remove the tail, extending the snake
    bool eaten = player.head->x == food.x && player.head->y == food.y;
    if (eaten)
    {
      food = createFood();
    }
    else
    {
      removeTail(&player);
    }
    lastDirection = player.dir;

    /*
     * RENDERING A FRAME
     */
    if (isValidPosition(&player))
    {
      // Init the matrix to all zeroes
      byte matrix[MATRIX_SIZE] =
      {
        0, 0, 0, 0, 0, 0, 0, 0
      };
  
      struct position *currentSegment = player.head;
      while (currentSegment != (struct position *) 0)
      {
        addToMatrix(currentSegment, matrix);
        currentSegment = currentSegment->next;
      }

      addToMatrix(&food, matrix);
      
      displayMatrix(0, matrix);
    }
    else
    {
      player.alive = false;
      food = createFood();
      byte matrix[MATRIX_SIZE] =
      {
        B10000001,
        B01000010,
        B00100100,
        B00011000,
        B00011000,
        B00100100,
        B01000010,
        B10000001
      };
      displayMatrix(0, matrix);
      currentTime += FRAME_PERIOD; // Show this for twice the duration
    }

    lastFrame = currentTime;
  }
}


struct position createFood()
{
  return (struct position)
  {
    (int) random(MATRIX_SIZE),
    (int) random(MATRIX_SIZE),
    (struct position *) 0
  };
}

struct snake createSnake()
{
  struct position *head = createSegment(1, 3);
  return (struct snake)
  {
    head,
    0,
    RIGHT,
    true
  };
}

struct position *createSegment(int x, int y)
{
  struct position *newSegment = (struct position *) malloc(sizeof(struct position));
  newSegment->x = x;
  newSegment->y = y; 
  newSegment->next = (struct position *) 0;
  return newSegment;
}

void addToHead(struct snake *s, struct position *segment)
{
  segment->next = s->head;
  s->head = segment;
  s->size++;
}

void removeTail(struct snake *s)
{
  if (s->head->next == (struct position *) 0)
  {
    // Cannot remove the head if it is the only segment
    return;
  }
  
  struct position *currentSegment = s->head;
  while (currentSegment->next->next != (struct position *) 0)
  {
    currentSegment = currentSegment->next;
  }

  // currentSegment is now the second last segment
  free(currentSegment->next);
  currentSegment->next = (struct position *) 0;
}

bool isOppositeDirection(enum direction a, enum direction b)
{
  return abs(a - b) == 2;
}

struct position *movePosition(struct position *prevPos, enum direction dir)
{
  struct position *pos = createSegment(prevPos->x, prevPos->y);
  switch (dir)
  {
    case UP:
      pos->y--;
      break;
    case RIGHT:
      pos->x++;
      break;
    case DOWN:
      pos->y++;
      break;
    case LEFT:
      pos->x--;
  }
  return pos;
}

bool isValidPosition(struct snake *s)
{
  // Check for the head being out of bounds or intercepting with the rest of the snake
  
  if (
    s->head->x < 0 ||
    s->head->x >= MATRIX_SIZE ||
    s->head->y < 0 ||
    s->head->y >= MATRIX_SIZE
  ) return false;

  struct position *segment = s->head->next;
  while (segment != (struct position *) 0)
  {
    if (s->head->x == segment->x && s->head->y == segment->y) return false;
    segment = segment->next;
  }

  return true;
}

void displayMatrix(const int devNum, byte *matrix)
{
  for (int i = 0; i < MATRIX_SIZE; i++)
  {
    ledCtrl.setRow(devNum, i, *matrix++);
  }
}

void addToMatrix(struct position *pos, byte *matrix)
{
  *(matrix + pos->y) |= B10000000 >> pos->x;
}
