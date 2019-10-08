/*Non-Canonical Input Processing*/
/*Receptor:
• lê os caracteres (um a um) da porta série, em modo não canónico, até receber o caracter de fim
de string (‘\0’);
• imprime a string com printf("%s\n", ...).
• reenvia a a string recebida do Emissor, escrevendo os caracteres respectivos (incluindo ‘\0’) na
porta série*/

#include <sys/types.h>
#include <sys/stat.h>
#include <wait.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;


typedef enum state {
  START_ST,
  FLAG_ST,
  A_ST,
  C_ST,
  BCC_ST,
  STOP_ST
} State;

typedef enum {
  F_RCV,
  A_RCV,
  C_RCV,
  BCC_RCV,
  OTHER_RCV
} Flags;

typedef enum {
  SET_CTRL = 3
} Control_Field;

void state_machine(State *currState, Flags lastFlag, __uint8_t* a_msg, __uint8_t* c_msg, __uint8_t* bcc_msg)
{
    switch (*currState)
    {
      case START_ST:
      printf("START\n");
        printf("%u\n", lastFlag);
        if(lastFlag == F_RCV) *currState = FLAG_ST; break;
      case FLAG_ST:
        printf("FLAG\n");
        printf("%u\n", lastFlag);
        if(lastFlag == A_RCV) *currState = A_ST;
        else if (lastFlag == F_RCV) *currState = FLAG_ST;
        else *currState = START_ST;
        break;
      case A_ST:
        printf("A\n");
        if(lastFlag == C_RCV && *c_msg == SET_CTRL) *currState = C_ST;
        else if (lastFlag == F_RCV) *currState = FLAG_ST;
        else *currState = START_ST;
        break;
      case C_ST:
        printf("C\n");
        if(lastFlag == BCC_RCV && *bcc_msg == (*a_msg ^ *c_msg)) *currState = BCC_ST;
        else if (lastFlag == F_RCV) *currState = FLAG_ST;
        else *currState = START_ST;
        break;
      case BCC_ST:
        printf("BCC\n");
        if(lastFlag == F_RCV) *currState = STOP_ST;
        else *currState = START_ST;
        break;
      default:
        break;
    }
}

int main(int argc, char** argv)
{
  __uint8_t a_msg = 2;
  __uint8_t c_msg = SET_CTRL;
  __uint8_t bcc_msg = (a_msg ^ c_msg);

  State currState = START_ST;
  Flags lastFlag = F_RCV;
  while(currState != STOP_ST)
  {

    while(val = read.dfgklsefjkqhef)
    {
    }


    state_machine(&currState, lastFlag, &a_msg, &c_msg, &bcc_msg);

    //printf("%u\n", currState);
    //printf("%u\n", lastFlag);


    sleep(1);

    if(lastFlag == BCC_RCV)
      lastFlag = F_RCV;
    else
      lastFlag = (lastFlag+1)%5;


  }

  printf("STOP\n");

  //   int fd, res;
  //   //int c;
  //   struct termios oldtio,newtio;
  //   char buf[255];
  //
  //   if ( (argc < 2) ||
  // 	    ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  // 	    (strcmp("/dev/ttyS1", argv[1])!=0) &&
  // 	    (strcmp("/dev/ttyS2", argv[1])!=0) )) {
  //     printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
  //     exit(1);
  //   }
  //
  //
  // /*
  //   Open serial port device for reading and writing and not as controlling tty
  //   because we don't want to get killed if linenoise sends CTRL-C.
  // */
  //
  //
  //   fd = open(argv[1], O_RDWR | O_NOCTTY );
  //   if (fd <0) {perror(argv[1]); exit(-1); }
  //
  //   if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
  //     perror("tcgetattr");
  //     exit(-1);
  //   }
  //
  //   bzero(&newtio, sizeof(newtio));
  //   newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  //   newtio.c_iflag = IGNPAR;
  //   newtio.c_oflag = 0;
  //
  //   /* set input mode (non-canonical, no echo,...) */
  //   newtio.c_lflag = 0;
  //
  //   newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  //   newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */
  //
  //
  //
  // /*
  //   VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
  //   leitura do(s) pr�ximo(s) caracter(es)
  // */
  //
  //
  //
  //   tcflush(fd, TCIOFLUSH);
  //
  //   if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
  //     perror("tcsetattr");
  //     exit(-1);
  //   }
  //
  //   printf("New termios structure set\n");
  //
  //
  //   while (STOP==FALSE) {       /* loop for input */
  //     res = read(fd,buf,255);   /* returns after 5 chars have been input */
  //     buf[res]=0;               /* so we can printf... */
  //     printf(":%s:%d\n", buf, res);
  //     if (buf[0]=='\0') STOP=TRUE;
  //   }
  //
  //
  //
  // /*
  //   O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o
  // */
  //
  //
  //
  //   tcsetattr(fd,TCSANOW,&oldtio);
  //   close(fd);
  //   return 0;
}
