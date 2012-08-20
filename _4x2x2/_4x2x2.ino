#include <TimerOne.h>

#define NUM_LEDS 512
#define NUM_LAYERS 8
#define NUM_ROWS 8
#define CUBE_FPS 1000

/* LED Cube outputs */

int layer0 = 0;   // MOSFET controlling this layer's connection to GND
int layer1 = 1;
int layer2 = 2;
int layer3 = 3;
int layer4 = 4;
int layer5 = 5;
int layer6 = 6;
int layer7 = 7;
int ser = 8;       // 64-bit output line to shift register array
int rclk = 9;      // output latch for shift register array
int srclk = 10;    // shift register array shift clock

unsigned char layer = 0;
unsigned char buffer0[NUM_LAYERS][NUM_LEDS/NUM_LAYERS]; // the buffer to output to the cube
unsigned char buffer1[NUM_LAYERS][NUM_LEDS/NUM_LAYERS]; // the buffer to output to the cube
int seed = 0;

// the setup routine runs once when you press reset:
void setup() {
  // initialize output pins
  pinMode(srclk, OUTPUT);
  pinMode(ser, OUTPUT);
  pinMode(rclk, OUTPUT);
  pinMode(layer0, OUTPUT);
  pinMode(layer1, OUTPUT);
  pinMode(layer2, OUTPUT);
  pinMode(layer3, OUTPUT);
  pinMode(layer4, OUTPUT);
  pinMode(layer5, OUTPUT);
  pinMode(layer6, OUTPUT);
  pinMode(layer7, OUTPUT);
  
  // initialize timer with period in microseconds
  Timer1.initialize(1000000 / (CUBE_FPS*NUM_LAYERS));
  Timer1.attachInterrupt(writeLayer);
  seed = Timer1.read();
  init_rand();
}

/* writes layer of cube */
void writeLayer() {
  // disable shift register output while shifting in
  PORTB &= B111101;
  
  for (int i=63; i >= 0; i--) {
    PORTB &= B111011;
    PORTB = (PORTB & B111110) | buffer0[layer][i];
    PORTB |= B000100;
  }
  
  // latch the output to the LEDs
  PORTB |= B000010;
  
  // enable appropriate layer
  PORTD = 1 << layer;
  
  // rotate to next layer
  layer = (layer + 1) % NUM_LAYERS;
}

void copy_buffers() {
  for (int i=0; i < NUM_LAYERS; i++) {
    for (int j=0; j < NUM_LEDS/NUM_LAYERS; j++) {
      buffer0[i][j] = buffer1[i][j];
    }
  }
}

// sets entire buffer to value
void init_buffer(unsigned char val) {
  for (int i=0; i < NUM_LAYERS; i++)
    for (int j=0; j < NUM_LEDS/NUM_LAYERS; j++)
      buffer1[i][j] = val;

  copy_buffers();
}

int rand(int range) {
  int m = 991;
  int a = 5;
  int c = 3;
  seed = (a * seed + c) % m;
  return seed % range;
}

void init_rand() {
  for (int i=0; i < NUM_LAYERS; i++) {
    for (int j=0; j < (NUM_LEDS / NUM_LAYERS); j++) {
      buffer1[i][j] = rand(2);
    }
  }
  copy_buffers();
}

int get_buffer_value(int x, int y, int z) {
  if (x < 0 || y < 0 || z < 0 || x >= NUM_LAYERS || y >= NUM_ROWS || z >= NUM_ROWS) {
    return 0;
  }
  return buffer0[x][y * 8 + z];
}

void set_buffer_value(int x, int y, int z, int value) {
  buffer1[x][y * 8 + z] = value;
}

int is_buffer_same() {
  for (int i=0; i < NUM_LAYERS; i++) {
    for (int j=0; j < NUM_LEDS/NUM_LAYERS; j++) {
      if (buffer0[i][j] != buffer1[i][j]) {
        return 0;
      }
    }
  }
  return 1;
}

int num_live_neighbors(int x0, int y0, int z0) {
  int nn = 0;
  for (int x = x0 - 1; x < x0 + 1; x++) {
    for (int y = y0 - 1; y < y0 + 1; y++) {
      for (int z = z0 - 1; z < z0 + 1; z++) {
        nn += get_buffer_value(x, y, z);
      }
    }
  }
  nn -= get_buffer_value(x0, y0, z0);
  return nn;
}

void conways_game() {
  int nn;
  int m = 2, n = 3;
  for (int x=0; x < NUM_LAYERS; x++) {
    for (int y=0; y < NUM_ROWS; y++) {
      for (int z=0; z < NUM_ROWS; z++) {
        nn = num_live_neighbors(x, y, z);
        if (get_buffer_value(x, y, z)) {
          set_buffer_value(x, y, z, (int)(nn < m || nn > n));
        } else {
          set_buffer_value(x, y, z, (int)(nn == n));
        }
      }
    }
  }
}

void signal_death() {
  init_buffer(1);
  delay(2000);
  init_buffer(0);
  delay(2000);
  init_buffer(1);
  delay(2000);
}

// ---------------
#define NUM_BALLS 5

int balls[NUM_BALLS][3];
int velocity[NUM_BALLS][3];

void init_balls() {
  init_buffer(0);
  for (int b = 0; b < NUM_BALLS; b++) {
    balls[b][0] = rand(NUM_LAYERS);
    balls[b][1] = rand(NUM_ROWS);
    balls[b][2] = rand(NUM_ROWS);
    set_buffer_value(balls[b][0], balls[b][1], balls[b][2], 1);

    velocity[b][0] = rand(2) ? 1 : -1;
    velocity[b][1] = rand(2) ? 1 : -1;
    velocity[b][2] = rand(2) ? 1 : -1;
  }
}

void bouncing_balls() {
  init_buffer(0);
  for (int b = 0; b < NUM_BALLS; b++) {
    balls[b][0] += velocity[b][0] % NUM_LAYERS;
    balls[b][1] += velocity[b][1] % NUM_ROWS;
    balls[b][2] += velocity[b][2] % NUM_ROWS;
    set_buffer_value(balls[b][0], balls[b][1], balls[b][2], 1);
  }
}


// ---------------
int rain_particles[NUM_LEDS / NUM_LAYERS];
int rain_velocity[NUM_LEDS / NUM_LAYERS];
void init_rain() {
  for (int i = 0; i < NUM_LEDS / NUM_LAYERS; i++) {
    rain_velocity[i] = 1;
  }
  for (int i = 0; i < NUM_LEDS / NUM_LAYERS; i++) {
    set_buffer_value(0, i, i/NUM_LAYERS, 1);
  }
}

void rain() {
  init_buffer(0);
  for (int i = 0; i < NUM_LEDS / NUM_LAYERS; i++) {
    rain_particles[i] = (rain_particles[i] + rain_velocity[i]) % NUM_ROWS;
    set_buffer_value(7 - rain_particles[i], i, i/NUM_LAYERS, 1);
  }
}


// the loop routine runs over and over again forever:
void loop() {
  int steps = 10;
  init_rand();
  // init_balls();
  // init_rain();

  copy_buffers();

  for (int i = 0; i <= steps; i++) {
    // bouncing_balls();
    conways_game();
    // rain();

    if (is_buffer_same()) {
      signal_death();
      break;
    }

    copy_buffers();
    delay(1000);
  }
}

