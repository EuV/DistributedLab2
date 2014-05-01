#include "la2.h"

void transfer( void* parent_data, local_id src, local_id dst,  balance_t amount ) {
	// student, please implement me
}


int main( int argc, char* argv[] ) {

	EventsLog = open( evengs_log, O_CREAT | O_TRUNC | O_WRONLY | O_APPEND, 0777 );
	PipesLog = open( pipes_log, O_CREAT | O_TRUNC | O_WRONLY | O_APPEND, 0777 );

	int procTotal;
	balance_t balance[ MAX_PROCESS_ID ];
	if( !getBranchesInitialBalance( argc, argv, &procTotal, balance ) ) {
		return 1;
	}

	createFullyConnectedTopology( procTotal );

	makePipeLog( procTotal );

	createBranches( procTotal, balance );

	//bank_robbery(parent_data);
	//print_history(all);

	return 0;
}


void accountService( const Process * const proc ) {

	closeUnusedPipes( proc );

	// STARTED
	Message startedMsg;
	sprintf( startedMsg.s_payload, log_started_fmt, get_physical_time(), proc -> localId, getpid(), getppid(), proc -> initialBalance );
	startedMsg.s_header.s_type = STARTED;
	startedMsg.s_header.s_magic = MESSAGE_MAGIC;
	startedMsg.s_header.s_local_time = get_physical_time();
	startedMsg.s_header.s_payload_len = strlen( startedMsg.s_payload );

	makeLogging( startedMsg.s_payload, startedMsg.s_header.s_payload_len );

	if ( send_multicast( ( void* )proc, &startedMsg ) == IPC_FAILURE ) {
		printf( "Multicast error in Process %d\n", proc -> localId );
		exit( 1 );
	}

	// Receive STARTED
	receiveAll( ( void* )proc, STARTED, proc -> total - 1 );
	sprintf( LogBuf, log_received_all_started_fmt, get_physical_time(), proc -> localId );
	makeLogging( LogBuf, strlen( LogBuf ) );



	/*
	// DONE
	Message doneMsg;
	fillMessage( &doneMsg, DONE, proc -> localId, proc -> initialBalance );
	makeLogging( doneMsg.s_payload, doneMsg.s_header.s_payload_len );
	if ( send_multicast( ( void* )proc, &doneMsg ) == IPC_FAILURE ) {
		printf( "Multicast error in Process %d\n", proc -> localId );
		exit( 1 );
	}

	// Receive DONE
	receiveAll( ( void* )proc, DONE, proc -> total - 1 );
	sprintf( LogBuf, log_received_all_done_fmt, proc -> localId );
	makeLogging( LogBuf, strlen( LogBuf ) );*/

	closeOtherPipes( proc );
}


void customerService( const Process * const proc ) {

	closeUnusedPipes( proc );

	// Receive STARTED
	receiveAll( ( void* )proc, STARTED, proc -> total );
	sprintf( LogBuf, log_received_all_started_fmt, get_physical_time(), proc -> localId );
	makeLogging( LogBuf, strlen( LogBuf ) );

	/*
	// Receive DONE
	receiveAll( ( void* )proc, DONE, proc -> total );
	sprintf( LogBuf, log_received_all_done_fmt, proc -> localId );
	makeLogging( LogBuf, strlen( LogBuf ) );*/

	waitForBranches();

	closeOtherPipes( proc );
}

// ================================================================================================

bool getBranchesInitialBalance( const int argc, char** const argv, int* procTotal, balance_t* balance ) {

	if( argc < 5 ) return false;

	*procTotal = atoi( argv[ 2 ] );
	if( *procTotal < 2 || MAX_PROCESS_ID < *procTotal || argc - 3 < *procTotal ) return false;

	for( int i = 0; i < *procTotal; i++ ) {
		balance[ i ] = atoi( argv[ 3 + i ] );
	}

	return true;
}


void createFullyConnectedTopology( const int procTotal ) {
	for ( int row = 0; row <= procTotal; row++ ) {
		for ( int col = 0; col <= procTotal; col++ ) {
			if ( row == col ) continue;
			pipe( Pipes[ row ][ col ] );
			fcntl( Pipes[ row ][ col ][ READ ], F_SETFL, O_NONBLOCK );
		}
	}
}


void makePipeLog( const int procTotal ) {
	char buf[ 10 ];
	for ( int row = 0; row <= procTotal; row++ ) {
		for ( int col = 0; col <= procTotal; col++ ) {
			sprintf( buf, "| %3d %3d ", Pipes[ row ][ col ][ READ ], Pipes[ row ][ col ][ WRITE ] );
			write( PipesLog, buf, strlen( buf ) );
		}
		write( PipesLog, "|\n", 2 );
	}
}


void createBranches( const int procTotal, const balance_t* const balance ) {
	for ( int i = 1; i <= procTotal; i++ ) {
		Process process = { procTotal, PARENT_ID, balance[ i - 1 ] };
		if ( fork() == 0 ) {
			process.localId = i;
			accountService( &process );
			break; // To avoid fork() in a child
		} else if ( i == procTotal ) { // The last child has been created
			customerService( &process );
		}
	}
}

// ================================================================================================

void closeUnusedPipes( const Process * const proc ) {
	for ( int row = PARENT_ID; row <= proc -> total; row++ ) {
		for ( int col = PARENT_ID; col <= proc -> total; col++ ) {
			if ( row == col ) continue;
			if ( row == proc -> localId ) {
				close( Pipes[ row ][ col ][ READ ] );
			} else {
				close( Pipes[ row ][ col ][ WRITE ] );
				if ( col != proc -> localId ) {
					close( Pipes[ row ][ col ][ READ ] );
				}
			}
		}
	}
}


void fillMessage( Message * msg, const MessageType msgType, const local_id id, const balance_t initialBalance ) {
	switch( msgType ) {
		case STARTED:
			sprintf( msg -> s_payload, log_started_fmt, get_physical_time(), id, getpid(), getppid(), initialBalance );
			break;
		case DONE:
			sprintf( msg -> s_payload, log_done_fmt, id );
			break;
		default:
			sprintf( msg -> s_payload, "Unsupported type of message\n" );
			break;
	}
	msg -> s_header.s_type = msgType;
	msg -> s_header.s_magic = MESSAGE_MAGIC;
	msg -> s_header.s_payload_len = strlen( msg -> s_payload );
}


void makeLogging( const char * const buf, const size_t count ) {
	write( STDOUT_FILENO, buf, count );
	write( EventsLog, buf, count );
}


void receiveAll( void* self, const MessageType msgType, const int expectedNumber ) {
	Message incomingMsg;
	int counter = 0;
	while ( counter != expectedNumber ) {
		int result = receive_any( self, &incomingMsg );
		if( result == IPC_SUCCESS && incomingMsg.s_header.s_type == msgType ) {
			counter++;
		} else {
			printf( "Receive error in Process %d\n", getpid() );
			exit( 1 );
		}
	}
}


void closeOtherPipes( const Process * const proc ) {
	for ( int rowOrCol = PARENT_ID; rowOrCol <= proc -> total; rowOrCol++ ) {
		if ( rowOrCol == proc -> localId ) continue;
		close( Pipes[ proc -> localId ][ rowOrCol ][ WRITE ] );
		close( Pipes[ rowOrCol ][ proc -> localId ][ READ ] );
	}
}

// ================================================================================================

void waitForBranches() {
	int status;
	pid_t pid;
	while ( ( pid = wait( &status ) ) != -1 ) {
		// printf( "Process %d has been done with exit code %d\n", pid, status );
		if( WIFSIGNALED( status ) ) printf( "!!! Interrupted by signal %d !!!\n", WTERMSIG( status ) );
	}
}
