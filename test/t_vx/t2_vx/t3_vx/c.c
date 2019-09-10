#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <ctype.h>

#define BUFF_SIZE 1024
#define DEFAULT_PORT "80"
#define IS_A_NUMBER 0
#define NOT_A_NUMBER -1
#define NOT_FOUND -1
#define BODY_FLAG "-p"
#define PARAMETERS_FLAG "-r"
#define INVALID -1
#define VALID 1


   

