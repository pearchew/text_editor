#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode()
{
  // Terminal attributes can be read into a termios struct by tcgetattr().
  // After modifying them, you can then apply them to the terminal using tcsetattr().
  tcgetattr(STDIN_FILENO, &orig_termios);
  // atexit() comes from <stdlib.h>. We use it to register our disableRawMode()
  atexit(disableRawMode);
  // We store the original terminal attributes in a global variable, orig_termios.
  // We assign the orig_termios struct to the raw struct in order to make a copy of it before we start making our changes.
  struct termios raw = orig_termios;
  // echoing is the default behavior where the terminal automatically prints out every single key you type onto the screen so that you can see it, this stops that
  // The c_lflag field is for “local flags”. A comment in macOS’s <termios.h> describes it as a “dumping ground for other state”.
  // So perhaps it should be thought of as “miscellaneous flags”. The other flag fields are c_iflag (input flags), c_oflag (output flags),
  // and c_cflag (control flags), all of which we will have to modify to enable raw mode.
  raw.c_lflag &= ~(ECHO);
  // The TCSAFLUSH argument specifies when to apply the change: in this case, it waits for all pending output to be written to the terminal,
  // and also discards any input that hasn’t been read.
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main()
{
  // What we want is raw mode. Unfortunately, there is no simple switch you can flip to set the terminal to raw mode.
  // Raw mode is achieved by turning off a great many flags in the terminal
  enableRawMode();
  char c;
  // read 1 byte into variable c, and repeat until read() returns 0 (EOF) or -1 (error)
  // by default your terminal starts in canonical mode, also called cooked mode.
  // In this mode, keyboard input is only sent to your program when the user presses
  // if it reads a 'q', it will exit the loop and the program will end. Otherwise, it will keep waiting for input.
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
    ;

  return 0;
}