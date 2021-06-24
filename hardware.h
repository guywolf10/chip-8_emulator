// this file will define data structures needed for the "hardware" of a chip-8


// 4kB of RAM
typedef unsigned char byte;
typedef byte[4096] RAM;


// the display, each pixel is either on or off
enum b {false, true};
typedef enum b bool;
typedef bool[32][64] display;


// the stack for the subroutine call addresses
typedef struct {
    unsigned short int s[]
};


