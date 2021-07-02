// the implementation of a chip-8 CPU





#include <SDL2/SDL.h>
#include "cpu.h"
#include <stdio.h>
#include <unistd.h>





#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define PIXEL_SIZE 8



// function prototypes
void cycle();
void update_screen();
void load_ROM(char *file);
void load_font();
int check_ROM(char *file);






// 4kB of RAM
unsigned char memory[4096];

// the display
unsigned char display[DISPLAY_WIDTH][DISPLAY_HEIGHT];

// 15 general use registers, V0-VE, and 1 flag register, VF
unsigned short V[16];

// index register and program counter
unsigned short PC;
unsigned short I;

// stack and stack pointer
unsigned short stack[16];
unsigned short sp;

// delay and sound timers
unsigned char delay_timer;
unsigned char sound_timer;

// sound and display draw flags, whether or not we need to beep or draw in this sycle
unsigned char sound_flag;
unsigned char draw_flag;

// keypad
unsigned char keypad[16];

// font
unsigned char font[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,   // 0
    0x20, 0x60, 0x20, 0x20, 0x70,   // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,   // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,   // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,   // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,   // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,   // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,   // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,   // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,   // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,   // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,   // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,   // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,   // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,   // E
    0xF0, 0x80, 0xF0, 0x80, 0x80    // F
};













// pushs to the top of the stack
void push(unsigned short address)
{
    stack[++sp] = address;
}


// returns the top of the stack
unsigned short pop()
{
    return stack[sp--];
}









SDL_Window *window;

SDL_Renderer *renderer;

int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Failed to initialize the SDL2 library\n");
        return -1;
    }

    window = SDL_CreateWindow("chip-8 emulator",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              DISPLAY_WIDTH * PIXEL_SIZE, DISPLAY_HEIGHT * PIXEL_SIZE,
                              0);

    if (!window)
    {
        printf("Failed to create window\n");
        return -1;
    }

    SDL_Surface *window_surface = SDL_GetWindowSurface(window);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!window_surface)
    {
        printf("Failed to get the surface from the window\n");
        return -1;
    }


    

    if (argc > 1) {
        if (!check_ROM(argv[1])) {
            printf("couldn't open the ROM file \"%s\"\n", argv[1]);
            return 1;
        }
        load_ROM(argv[1]);
        PC = 0x200;
        I = 0;
        
        int keep_window_open = 1;
        while (keep_window_open)
        {
            SDL_Event e;
            while (SDL_PollEvent(&e) > 0)
            {
                switch (e.type)
                {
                case SDL_QUIT:
                    keep_window_open = 0;
                    break;
                }

                
                if (PC < 644) {
                    cycle();
                    if (draw_flag)
                        update_screen();

                    // delaying because chip-8 was supposed to run on slower devices
                    usleep(1000);
                }
            }
        }
    }

    free(window_surface);
    free(window);
    free(renderer);

    return 0;
}

























// the cycle of fetch -> decode -> execute
void cycle()
{

    printf("\n\nPC:\t%04X (%d)\n", PC, PC);
    SDL_Surface *window_surface = SDL_GetWindowSurface(window);

    // we currently don't need to update the sound or screen
    draw_flag = 0;
    sound_flag = 0;

    // the fetch stage
    unsigned short opcode = (memory[PC] << 8) | memory[PC+1];
    PC += 2;

    printf("opcode:\t%04X\n", opcode);

    // the decode stage
    unsigned short x = (opcode & 0x0F00) >> 8; // the second nibble
    unsigned short y = (opcode & 0x00F0) >> 4; // the third nibble
    unsigned short n = (opcode & 0x000F);      // the fourth nibble
    unsigned short nn = (opcode & 0x00FF);     // the third and fourth nibbles
    unsigned short nnn = (opcode & 0x0FFF);    // the second, third and fourth nibbles

    // the execute stage (and the rest of the decode stage)
    switch ((opcode & 0xF000) >> 12)
    {
    case 0x0:
        switch (n)
        {
        // 00E0, clear screen by setting each pixel to 0
        case 0x0:
            draw_flag = 1;
            for (int i = 0; i < DISPLAY_WIDTH; i++)
                for (int j = 0; j < DISPLAY_HEIGHT; j++)
                    display[i][j] = 0;
            printf("clearing the screen\n");
            break;

        // 00EE, return from a subroutine
        case 0xE:
            PC = pop();
            break;
        }
        break;

    // 1nnn, jump to address nnn
    case 0x1:
        PC = nnn;
        printf("jumping to address %d\n", nnn);
        break;

    case 0x2:
        break;

    case 0x3:
        break;

    case 0x4:
        break;

    case 0x5:
        break;

    // 6xnn, set Vx to nn
    case 0x6:
        V[x] = nn;
        printf("setting V%x to %d\n", x, nn);
        break;

    // 7xnn, add nn to Vx
    case 0x7:
        V[x] += nn;
        printf("adding %d to V%x\n", nn, x);
        break;

    case 0x8:
        break;

    case 0x9:
        break;

    // Annn, set I to nnn
    case 0xA:
        I = nnn;
        printf("setting I to %d", nnn);
        break;

    case 0xB:
        break;

    case 0xC:
        break;

    // Dxyn, display n-byte sprite starting at memory location I on the screen at coordinates (Vx, Vy) and sets V[0xF] to 1 if (and only if) there a collision
    case 0xD:
        V[0xF] = 0;
        draw_flag = 1;
        for (int row = 0; row < n; row++)
            for (int column = 0; column < 8; column++) {
                unsigned char pixel = (memory[I+row] >> (7-column)) & 1;
                if (pixel == 1 && display[V[x] + row][V[y] + column] == 1)  // if there is a collision
                    V[0xF] = 1;
                display[V[x] + column][V[y] + row] ^= pixel;
            }
        printf("displaying/drawing\n");
        break;

    case 0xE:
        break;

    case 0xF:
        switch (y)
        {
        case 0x0:
            switch (n)
            {
            // Fx07, set Vx to the delay timer's value
            case 0x7:
                V[x] = delay_timer;
                break;

            // Fx0A, wait for a keypress and store the key's value in Vx
            case 0xA:
                break;
            }
            break;

        case 0x1:
            switch (n)
            {
            // Fx15, set the delay timer to Vx's value
            case 0x5:
                delay_timer = V[x];
                break;

            // Fx18, set the sound timer to Vx's value
            case 0x8:
                sound_timer = V[x];
                break;

            // Fx1E, add Vx's value to I
            case 0xE:
                I += V[x];
                break;
            }
            break;

        case 0x2:
            break;

        case 0x3:
            break;

        // Fx55, store V0-Vx in memory starting at I
        case 0x5:
            for (int i = 0; i <= x; i++)
                memory[I + i] = V[i];
            break;

        // Fx65, read V0-Vx from memory starting at I
        case 0x6:
            for (int i = 0; i <= x; i++)
                V[i] = memory[I + i];
            break;
        }
        break;
    }

    // update the delay timer
    if (delay_timer > 0)
        delay_timer--;
    
    // beep and update the sound timer
    if (sound_timer > 0) {
        sound_flag = 1;
        sound_timer--;
    }
}























void update_screen()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int i = 0; i < DISPLAY_WIDTH; i++)
        for (int j = 0; j < DISPLAY_HEIGHT; j++)
            if (display[i][j]) {
                SDL_Rect rect;
                rect.x = i * PIXEL_SIZE;
                rect.y = j * PIXEL_SIZE;
                rect.w = PIXEL_SIZE;
                rect.h = PIXEL_SIZE;
                SDL_RenderFillRect(renderer, &rect);
            }
    SDL_RenderPresent(renderer);
}
















void load_ROM(char *file)
{
    FILE *fileptr = fopen(file, "rb");

    // get size of the file for malloc:
    fseek(fileptr, 0, SEEK_END);
    long buffer_size = ftell(fileptr);
    rewind(fileptr);

    char *buffer = (char *)malloc((buffer_size + 1) * sizeof(char)); // enough memory for file + \0
    if (fread(buffer, buffer_size, 1, fileptr));    // using an if statement to avoid compiler warnings
    fclose(fileptr);

    for (int i = 0; i < buffer_size; i++)
    {
        memory[512 + i] = buffer[i]; // first 512 bytes are reserved
    }

    free(buffer);
}







int check_ROM(char *file)
{
    FILE *fileptr = fopen(file, "rb");

    if (!fileptr)
        return 0;
    
    fclose(fileptr);
    return 1;
}





void load_font()
{
}