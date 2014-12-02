#include <stdio.h>
#include <unistd.h>

int main(int argc , char *argv[])
{

  char stdin_buf[1024];
  char input[1024];

  setvbuf(stdin, stdin_buf, _IOFBF, 1024);
  //setbuf(STDIN_FILENO, stdin_buf);


  sleep(2);
  stdin_buf[2] = '\0';
  fgets(input, 1024, stdin);

  printf("%s", input);
  fflush(stdout);

}
