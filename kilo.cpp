#include <unistd.h>
#include <termios.h>


void enableRawMode() {
  struct termios raw;
  // Terminal attributes can be read into a termios struct by tcgetattr(). 
  // After modifying them, you can then apply them to the terminal using tcsetattr(). 
  // The TCSAFLUSH argument specifies when to apply the change: in this case, it waits for all pending output to be written to the terminal, 
  // and also discards any input that hasn’t been read.
  tcgetattr(STDIN_FILENO, &raw);
  // echoing is the default behavior where the terminal automatically prints out every single key you type onto the screen so that you can see it, this stops that
  raw.c_lflag &= ~(ECHO);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    enableRawMode();
    char c;
    // read 1 byte into variable c, and repeat until read() returns 0 (EOF) or -1 (error)
    // by default your terminal starts in canonical mode, also called cooked mode. 
    // In this mode, keyboard input is only sent to your program when the user presses 
    // if it reads a 'q', it will exit the loop and the program will end. Otherwise, it will keep waiting for input.
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    // What we want is raw mode. Unfortunately, there is no simple switch you can flip to set the terminal to raw mode. 
    // Raw mode is achieved by turning off a great many flags in the terminal
    return 0;
}