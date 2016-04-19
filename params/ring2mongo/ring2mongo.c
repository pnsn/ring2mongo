#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
// #include <kom.h>
#include <transport.h>
#include <trace_buf.h>
// #include <swap.h>
#include <rdpickcoda.h>

typedef struct {
        int     pinno;                 /* Pin number */
        int     nsamp;                 /* Number of samples in packet */
        double  starttime;             /* time of first sample in epoch seconds
                                          (seconds since midnight 1/1/1970) */
        double  endtime;               /* Time of last sample in epoch seconds */
        double  samprate;              /* Sample rate; nominal */
        char    sta[TRACE2_STA_LEN];   /* Site name (NULL-terminated) */
        char    net[TRACE2_NET_LEN];   /* Network name (NULL-terminated) */
        char    chan[TRACE2_CHAN_LEN]; /* Component/channel code (NULL-terminated)*/
        char    loc[TRACE2_LOC_LEN];   /* Location code (NULL-terminated) */
        char    version[2];            /* version field */
        char    datatype[3];           /* Data format code (NULL-terminated) */
        /* quality and pad are available in version 20, see TRACE2X_HEADER */
        char    quality[2];            /* Data-quality field */
        char    pad[2];                /* padding */
        int     data[4096];
} TRACEBUF2_PLAIN;



/* Functions in this file */
void StartupErrorMsg( FILE *std );
TRACEBUF2_PLAIN * unpack_tracebuf2(char *msg, int msglen, unsigned char type);
void tbuf2string(TRACEBUF2_PLAIN *tbuf);
int write2mongo(char *msg, int msglen, unsigned char type );
int tracehead2json( char *msg, char *buf, int buflen );


#define MAX_RINGS	5
#define NUMLOGOS	2
#define MAXMSGLEN	4096  
#define MINARGS		1


// #define MAXFIFO 200 //num of TraceBuf2 in Buf
//
// /* FIFO management Functions */
// typedef struct _FIFOMSG {
//   char      *msg;
//   int        msglen;
//   long      seq;
//   int        type;
//   struct _FIFOMSG  *next;
// } FIFOMSG;



/* Globals */
unsigned char	Type_TraceBuf2;
unsigned char	Type_PickSCNL;
unsigned char	ModWildcard;
unsigned char	InstWildcard;
//fifo
// FIFOMSG    *fifoStart = NULL;
// FIFOMSG    *fifoEnd = NULL;
// int      fifoC = 0;
// int      fifokey = 0;


/* Main Program */
int main( int argc, char **argv ){
  MSG_LOGO	GetLogo[NUMLOGOS];		/* Message logos to retrieve */
	SHM_INFO	InRegion[MAX_RINGS];
	long		InRingKey[MAX_RINGS];	/* keys of transport input ring */
	unsigned char 	seq[MAX_RINGS];		/* Sequence numbers */
	int			nRings = 0;				/* Number of considered rings increment by reading ring args*/
	char		*msgbuf;				/* Message buffer */
	long		recsize;				/* Length of received message */
	MSG_LOGO	reclogo;				/* Logo of received message */
	int			rc;						/* Receive code */
	int			i;						/* Generic counter */
	int 		seqno;					/* Internal msg sequence number */
	char		*argps, *argpe;			/* For parsing arguments */
	int			cargs = 0;				/* Counting mandatory arguments */
 
	/* Process input arguments */
	if( argc < 2 ){
		/* Give error msg */
    StartupErrorMsg( stderr );
		return -1;
	}else{
		for( i = 1; i < argc; i++ ){
			/* Earthworm rings */
      if( strstr( argv[i], "-r" ) != NULL ){
				while( ++i < argc && argv[i][0] != '-' ){
					if( nRings < MAX_RINGS ){
						if( ( InRingKey[nRings++] = GetKey( argv[i] ) ) == -1 ){
							/* Invalid ring name */
							fprintf( stderr, "%s: Invalid ring - %s\n", argv[0],
									argv[i] );
							exit( -1 );
						}
						cargs++;
					}else{
						/* Number of rings exceeds maximum */
						fprintf( stderr, 
								"%s: Maximum number of rings <%d> exceeded.\n",
								argv[0], MAX_RINGS );
						StartupErrorMsg( stderr );
						exit( -1 );
					}
				}
				i--;
			} //end ew rings
		} //end argv args
	}
	/* Check mandatory arguments */
	if( cargs < MINARGS ){
		fprintf( stderr, "%s: Mandatory arguments are missing.\n",
				argv[0] );
		StartupErrorMsg( stderr );
		exit( -1 );
	}
		

	/* Specify Logos to get */
   printf("&Type_TraceBuf2 %d\n", Type_TraceBuf2);
	if( GetType( "TYPE_TRACEBUF2", &Type_TraceBuf2 ) != 0 ){
		fprintf(stderr, 
				"%s: Invalid message type <TYPE_TRACEBUF2>!\n", argv[0] );
		exit( -1 );
	}
	if( GetType( "TYPE_PICK_SCNL", &Type_PickSCNL ) != 0 ){
		fprintf(stderr, 
				"%s: Invalid message type <TYPE_PICK_SCNL!\n", argv[0] );
		exit( -1 );
	}
	if( GetModId( "MOD_WILDCARD", &ModWildcard ) != 0 ){
		fprintf(stderr, "%s: Invalid moduleid <MOD_WILDCARD>!\n", argv[0] );
		exit( -1 );
	}
	if( GetInst( "INST_WILDCARD", &InstWildcard ) != 0 ){
		fprintf(stderr, "%s: Invalid instid <INST_WILDCARD>!\n", argv[0] );
		exit( -1 );
	}
	GetLogo[0].instid = InstWildcard;
	GetLogo[0].mod = ModWildcard;
	GetLogo[0].type = Type_TraceBuf2;
	GetLogo[1].instid = InstWildcard;
	GetLogo[1].mod = ModWildcard;
	GetLogo[1].type = Type_PickSCNL;
	
	/* Allocate memory for message buffer */
	msgbuf = (char*) malloc( sizeof( char ) * MAXMSGLEN );
	if(msgbuf==NULL){
		fprintf( stderr, 
				"%s: Unable to allocate memory for message buffer\n", argv[0]);
		exit(-1);
	}
	
	/* Attach to shared memory rings */
	for( i = 0; i < nRings; i++ ){
		tport_attach(&InRegion[i], InRingKey[i]);
		
		/* Flush the ring */
		while(tport_copyfrom(&InRegion[i], GetLogo, NUMLOGOS, &reclogo,
				&recsize, msgbuf, MAXMSGLEN, &seq[i]) != GET_NONE);
	}
	/* Setup mutex semaphore */
  CreateMutex_ew();
  
	
  /* Start main cycle */
	while( tport_getflag( &InRegion[0] ) != TERMINATE ){
		int nmsgs = 0;
		
		
		/* Check for messages */
		for( i = 0; i < nRings; i++ ){
			rc = tport_copyfrom( &InRegion[i], GetLogo, NUMLOGOS, &reclogo, 
					&recsize, msgbuf, MAXMSGLEN, &seq[i] );
     
      // printf("inRegion.sid=%d\n", InRegion[i].sid);
      
			/* Decide what to do with message */
			switch(rc){
				case GET_OK:      // got a message, no errors or warnings (1)
					break;
               
				case GET_NONE:    // no messages of interest, check again later (0)
					continue;
               
				case GET_NOTRACK: // got a msg, but can't tell if any were missed
					fprintf( stderr,
							"Msg received (i%u m%u t%u); "
							"transport.h NTRACK_GET exceeded\n",
							reclogo.instid, reclogo.mod, reclogo.type );
					break;

				case GET_MISS_LAPPED:     // got a msg, but also missed lots
					fprintf( stderr,
							"Missed msg(s) from logo (i%u m%u t%u)\n",
							reclogo.instid, reclogo.mod, reclogo.type );
					break;

				case GET_MISS_SEQGAP:     // got a msg, but seq gap
					fprintf( stderr,
							"Saw sequence# gap for logo (i%u m%u t%u s%u)\n",
							reclogo.instid, reclogo.mod, reclogo.type, seq[i] );
					break;

				case GET_TOOBIG:  // next message was too big
					fprintf( stderr,
							"Retrieved msg[%ld] (i%u m%u t%u) "
							"too big for msgbuf[%d]\n",
							recsize, reclogo.instid, reclogo.mod, reclogo.type,
							MAXMSGLEN );
					continue;

				default:         // Unknown result
					fprintf( stderr, "Unknown tport_copyfrom result:%d\n", rc );
					continue;
			}
			msgbuf[recsize] = '\0'; // Null terminate for ease of printing
      nmsgs++;
			
			
      TRACEBUF2_PLAIN *tbuf = unpack_tracebuf2(msgbuf, recsize, reclogo.type);
      tbuf2string(tbuf);
      // tbuf2string(tbuf);
			if( seqno == -1 ){
				fprintf( stderr, "%s: Error adding message to Mongo.\n", argv[0]);
			}
		} // End of for cycle
		
		/* Wait for new messages */
		if(nmsgs == 0)
      sleep_ew(50);
} // End of while cycle
	
	/* detach from shared memory */
	for( i=0; i<nRings; i++ ) 
    tport_detach(&InRegion[i]);
	
	/* Free msg buffer */
	free(msgbuf);
	

	
	/* Destroy semaphore */
	CloseMutex();
	
	return 0;
}


void StartupErrorMsg( FILE *std ){
	fprintf( std, "WebSWave - Version: %s\n"
			"Usage: ring2mongo -r [Earthworm Rings] [options]\n"
			"Arguments:\n"
			"  -r <EW rings>  : Mandatory list of earthworm ring names. At \n"
			"                   least one ring must be given.\n"
			"Examples:\n"
			"  mongo -r WAVE_RING\n"
			"earthworm_forum\n"
      );
	return;
}


TRACEBUF2_PLAIN * unpack_tracebuf2(char *msg, int msglen, unsigned char type){
  int i;
  TRACEBUF2_PLAIN   *tbuf = ( TRACEBUF2_PLAIN* ) msg;
  if(strcmp(tbuf->datatype,"s4")==0 || strcmp(tbuf->datatype, "s2")==0){
    SwapInt(&tbuf->pinno);
    SwapInt(&tbuf->nsamp);
    SwapInt(&tbuf->starttime);
    SwapInt(&tbuf->endtime);
    SwapInt(&tbuf->samprate);
  }
  for(i=0; i< sizeof(tbuf->data); i+=4){
    SwapInt(&tbuf->data[i]);
  }
  return tbuf;
}

void tbuf2string(TRACEBUF2_PLAIN *tbuf){
  printf("pinno: %d\n "
        "nsamp: %d\n "
        "starttime: %f "
        "endtime: %f "
        "samprate: %f "
        "sta: %s "
        "datatype: %s\n",
         tbuf->pinno,
         tbuf->nsamp,
         tbuf->starttime,
         tbuf->endtime,
         tbuf->samprate,
         tbuf->sta,
         tbuf->datatype
      );
}

int write2mongo(char *msg, int msglen, unsigned char type){
	long seq = -1;
	int status = 1;
	unsigned char buffer[MAXMSGLEN + 100];
	int				msgtype;
	short			shortlen;
	char			*msgtypestr;
	char			msgtypestrlen;

	//TODO Check Mongo here
  //TEMP
  int mongoReady =1;
	if(mongoReady >0){
		/* This is a websocket request - set while cycle to send messages */
    // printf("type = %d\n", type);
      
		msgtypestr = GetTypeName( ( unsigned char ) type );
		msgtypestrlen = strlen( msgtypestr );
    //       printf("msgtypestr=%s\n", msgtypestr);
    // printf("msgtypestrlen=%d\n", msgtypestrlen);
    //       printf("msglen=%d\n\n", msglen);
    TRACEBUF2_PLAIN   *thead = ( TRACEBUF2_PLAIN* ) msg;
    printf("nsamp:%d \n", thead->nsamp);
			
  }
}










/*
 * tracehead2json : Was supposed to generate a json header to include 
 * but sending the raw message may be better so this is not used for now.
 */
int tracehead2json( char *msg, char *buf, int buflen ){
  TRACE2_HEADER   *thead = ( TRACE2_HEADER* ) msg;

  return snprintf( buf, buflen,
      "{"
      "\"typ\":\"tbuf2\","
      "\"sta\":\"%s\",\"cha\":\"%s\",\"net\":\"%s\",\"loc\":\"%s\","
      "\"st\":%.3f,\"et\":%.3f,\"sr\":%.3f,\"ns\":%d"
      "}",
      thead->sta, thead->chan, thead->net, thead->loc,
      thead->starttime, thead->endtime, thead->samprate, thead->nsamp );
}


