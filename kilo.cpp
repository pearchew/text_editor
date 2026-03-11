/*** includes ***/

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** data ***/

struct termios orig_termios;

/*** terminal ***/

/** error handling
 * Prints an error message and exits the program.
 */
void die(const char *s)
{
  perror(s);
  exit(1);
}

void disableRawMode()
{
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode()
{
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
    die("tcgetattr");
  /**
   * Terminal attributes can be read into a termios struct by tcgetattr().
   * After modifying them, you can then apply them to the terminal using tcsetattr()
   */
  tcgetattr(STDIN_FILENO, &orig_termios);
  /**
   * atexit() comes from <stdlib.h>. We use it to register our disableRawMode()
   */
  atexit(disableRawMode);
  /**
   * We store the original terminal attributes in a global variable, orig_termios.
   * We assign the orig_termios struct to the raw struct in order to make a copy of it before we start making our changes.
   */
  struct termios raw = orig_termios;
  /**  iflag vs lflag vs oflag
   * oflag: These flags control the processing of bytes as they are sent to the terminal, right before they are displayed on the screen.
   * iflag: These flags control the low-level processing of the raw bytes right as they arrive from the keyboard, before anything else happens.
   * lflag: It controls the overall "mode" of the terminal.
   * cflag: These flags control the low-level details of how the terminal handles input and output bytes, such as character size and parity checking.
   */
  /** types of flags to exclude
   * IXON comes from <termios.h>. The I stands for “input flag” (which it is, unlike the other I flags we’ve seen so far) and XON comes from the names of the two control characters that Ctrl-S and Ctrl-Q produce: XOFF to pause transmission and XON to resume transmission.
   * ISIG, disables Ctrl-C, Ctrl-Z, and Ctrl-Y, because these send signals that instantly kill or suspend the program instead of letting the editor use them as inputs, resulting in Ctrl-C being read as byte 3 and Ctrl-Z as byte 26.
   * IXON, disables Ctrl-S and Ctrl-Q, because these trigger legacy software flow control which freezes and resumes terminal output, resulting in Ctrl-S being read as byte 19 and Ctrl-Q as byte 17.
   * IEXTEN, disables Ctrl-V (and Ctrl-O on macOS), because Ctrl-V prompts the terminal to send the next character literally and macOS natively discards Ctrl-O, resulting in Ctrl-V being read as byte 22 and Ctrl-O as byte 15.
   * ICRNL, disables the translation of Ctrl-M and the Enter key, because the terminal automatically translates carriage returns (byte 13) into newlines (byte 10), resulting in Ctrl-M and Enter being accurately read as byte 13 instead of 10.
   * OPOST, Normally, when you print \n (newline), the terminal magically translates it into \r\n. So this just mandates to turn this on
   * \r (Carriage Return): Moves the cursor to the far left of the screen.
   * \n (Line Feed): Moves the cursor down one row.
   * CS8, This flag sets the character size to 8 bits per byte, which is the most common setting for modern terminals.
   * BRKINT,  break condition will cause a SIGINT signal to be sent to the program, like pressing Ctrl-C.
   * INPCK enables parity checking, which doesn’t seem to apply to modern terminal emulators.
   * ISTRIP, causes the 8th bit of each input byte to be stripped, meaning it will set it to 0. This is probably already turned off.
   */
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  // &= ~ turns off the feature and use Bitwise AND combined with NOT. The ~ flips the flag's bits (so everything is 1 except the feature we want to kill).
  // |= turns on the feature with a bitwise OR It forces the specific bit to 1 without touching the others.
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  /** indexes into the c_cc field
   * which stands for “control characters”
   * The VMIN value sets the minimum number of bytes of input needed before read() can return.
   * VTIME value sets the maximum amount of time to wait before read() returns. It is in tenths of a second, so we set it to 1/10 of a second, or 100 milliseconds.
   */
  // read() pauses the entire program and waits forever until you press a key. By setting VMIN = 0 (minimum bytes to wait for) 
  // and VTIME = 1 (wait max 100 milliseconds), read() will quickly return 0 if you don't type anything.
  // Because later, we will need the editor to do things in the background while you aren't typing—like blinking the cursor or handling a window resize.
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  /** TCSAFLUSH
   * TCAFLUSH argument specifies when to apply the change:
   * in this case, it waits for all pending output to be written to the terminal,
   * and also discards any input that hasn’t been read.
   */
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

/*** init ***/

int main()
{
  /** Raw Mode
   * Raw mode is achieved by turning off a great many flags in the terminal
   */
  enableRawMode();
  /** Converting Keypresses to Bytes
   * Most ordinary keys translate directly into the characters they represent
   * But things such as control keys and escape sequences for arrow keys and function keys translate into multiple bytes
   * that aren’t correspond to printable characters.
   */
  /** how function works
   * printf() can print multiple representations of a byte.
   * Iscntrl() tests whether a character is a control character. Control characters are nonprintable characters that we don’t want to print to the screen.
   * %d tells it to format the byte as a decimal number (its ASCII code)
   * %c tells it to write out the byte directly, as a character.
   */
  while (1)
  {
    char c = '\0';
    /** EAGAIN
     * Because we told read() to timeout after 100ms, some systems (like Cygwin on Windows) 
     * will interpret a timeout as an error and set an error code called EAGAIN (meaning "I tried, but there is no data, try again later").
     * We add && errno != EAGAIN to our die() check so that our program doesn't panic and crash just because you paused typing for a split second. */
    if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
      die("read");
    /** reading bytes
     * Read 1 byte into variable c
     * (by default your terminal starts in canonical mode, also called cooked mode.)
     */
    if (iscntrl(c))
    {
      printf("%d\r\n", c);
    }
    else
    {
      printf("%d ('%c')\r\n", c, c);
    }
    /* If it reads a 'q',
    it will exit the loop and the program will end. Otherwise, it will keep waiting for input. */
    if (c == 'q')
      break;
  }
  return 0;
}