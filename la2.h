#include "banking.h"
#include "pa2345.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#define NUMBER_OF_PROCESS 2
#define BUF_SIZE 100
#define WRITE 1
#define READ 0

enum {
	IPC_SUCCESS = 0,
	IPC_FAILURE = -1,
	IPC_PIPE_IS_EMPTY = 1
};

typedef struct {
    int total;
    local_id localId;
    balance_t balance;
    BalanceHistory history;
} Process;

typedef enum { false, true } bool;

bool getBranchesInitialBalance( const int, char** const, int*, balance_t* );
void createFullyConnectedTopology( const int );
void makePipeLog( const int );
void createBranches( const int, const balance_t* const );

void accountService( Process* const );
void customerService( Process* const );
void customerMainLoop( Process * const );
void fastForwardHistory( Process * const, const int );

void closeUnusedPipes( const Process* const );
void fillMessage( Message*, const Process* const, const MessageType );
void makeLogging( const char* const );
void closeOtherPipes( const Process* const );

void waitForBranches();

int Pipes[ MAX_PROCESS_ID + 1 ][ MAX_PROCESS_ID + 1][ 2 ];
int EventsLog;
int PipesLog;
char LogBuf[ BUF_SIZE ];
