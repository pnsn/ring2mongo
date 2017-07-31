  /*****************************************************************
   *                            ring2mongo.c                        *
   *                                                               *
   * Canabalized sniffwave program that reads ring data and writes to 
     mongo db
        
        
        Since we always want data, the data param has been removed
       otherwise is functions just like sniffwav.
   *                                                               *

   * Usage: ring2mongo  <Ring> <Sta> <Comp> <Net> <Loc>   *
   *    or: ring2mongo  <Ring> <Sta> <Comp> <Net>        *
   *    or: ring2mongo  <Ring>                                      *
   *                                                               *
   *****************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <transport.h>
#include <swap.h>
#include <time_ew.h>
#include <trace_buf.h>
#include <earthworm.h>
#include <libmseed.h>

#include <mongoc.h>
#include <bson.h>
#include <bcon.h>

#define NLOGO 5

#define TYPE_NOTUSED 254

int IsWild( char *str )
{
  if( (strcmp(str,"wild")==0)  ) return 1;
  if( (strcmp(str,"WILD")==0)  ) return 1;
  if( (strcmp(str,"*")   ==0)  ) return 1;
  return 0;
}



/* Cast quality fields to unsigned char when you want convert to hexadecimal. */
#define CAST_QUALITY( q ) ( (unsigned char) q )

/************************************************************************************/


int main( int argc, char **argv )
{
   SHM_INFO        region;
   long            RingKey;         /* Key to the transport ring to read from */
   MSG_LOGO        getlogo[NLOGO], logo;
   long            gotsize;
   char            msg[MAX_TRACEBUF_SIZ];
   char           *getSta, *getComp, *getNet, *getLoc, *inRing;
   char            wildSta, wildComp, wildNet, wildLoc;
   unsigned char   Type_Mseed;
   unsigned char   Type_TraceBuf,Type_TraceBuf2;
   unsigned char   Type_TraceComp, Type_TraceComp2;
   unsigned char   InstWildcard, ModWildcard;
   short          *short_data;
   int            *long_data;
   TRACE2_HEADER  *trh;
   char            orig_datatype[3];
   char            stime[256];
   char            etime[256];
   int             i;
   int             rc;
   int             nLogo = NLOGO;
   static unsigned char InstId;        /* local installation id              */
   char            ModName[MAX_MOD_STR];
   char		   *modid_name;
   unsigned char   sequence_number = 0;
   // int             statistics_flag;
   time_t monitor_start_time = 0;
   double start_time, end_time;
   unsigned long packet_total=0;
   unsigned long packet_total_size=0;
   MSRecord *msr = NULL;	/* mseed record */
   logit_init("ring2mongo", 200, 256, 1); 
   char *mongo_user = getenv("MONGO_USER");
   char *mongo_passwd=getenv("MONGO_PASSWD");
   char *mongo_host=getenv("MONGO_HOST");
   char *mongo_port=getenv("MONGO_PORT");
   if(mongo_user == NULL ||
      mongo_passwd==NULL ||
      mongo_host== NULL  ||
      mongo_port==NULL){
        printf("Ensure the following ENV varibles are set\n *MONGO_USER\n *MONGO_PASSWD\n *MONGO_HOST\n *MONGO_PORT\n");
        exit(1);
        return(1);
      }
   char mongo_str[128]="mongodb://";
   strcat(mongo_str, mongo_user);
   strcat(mongo_str, ":");
   strcat(mongo_str, mongo_passwd);
   strcat(mongo_str, "@");
   strcat(mongo_str, mongo_host);
   strcat(mongo_str, ":");
   strcat(mongo_str, mongo_port);   
   strcat(mongo_str, "/");
   //mongo stuff
   mongoc_client_t *m_client;
   mongoc_collection_t *m_collection;
   mongoc_init ();
   m_client = mongoc_client_new (mongo_str);
   m_collection = mongoc_client_get_collection (m_client, "waveforms", "ring");
  
   mongoc_bulk_operation_t *m_bulk;
   bson_error_t m_error;
   bson_t *m_doc;
   bson_t m_reply;
   bson_t *m_data;
   /*wait for this many messages before performing bulk write
   For real time continuous data it shoudl be ~ number of channels on ring*/
   int MONGO_BULK_MAX = 30; 
   int m_count = 0; /* counter for mongo bulk */
   bool m_ret;
   /*end mongo stuff*/
   
   
   

  /* Initialize pointers
  **********************/
  trh  = (TRACE2_HEADER *) msg;
  long_data  =  (int *)( msg + sizeof(TRACE2_HEADER) );
  short_data = (short *)( msg + sizeof(TRACE2_HEADER) );


  /* Check command line argument
  
  *****************************/
  if ( argc < 2 || argc > 6)
  {
     if(argc > 6)
     fprintf(stderr, "ring2mongo: Too many parameters\n");
     fprintf(stderr,"Usage: %s <ring name> [sta] [comp] [net] [loc] \n", argv[0]);
     fprintf(stderr, " Note: All parameters are positional, and all but first are optional.\n");
     fprintf(stderr, " the full data contained in the packet is printed out.\n");
     fprintf(stderr, " If sta comp net (but not loc) are specified then only TraceBuf\n");
     fprintf(stderr, " packets will be fetched (not TraceBuf2); otherwise both are fetched.\n");
     fprintf(stderr, " Example: %s WAVE_RING PHOB wild NC wild n\n", argv[0]);
     fprintf(stderr, " MSEED capability starting with version 2.5.1, prints mseed headers\n");
     fprintf(stderr, " of TYPE_MSEED packets (no filtering yet).\n");
     exit( 1 );
     return 1;
  }

  /* process given parameters */
  inRing  = argv[1];         /* first parameter is ring name */

  /* any parameters not given are set as wildcards */
  getSta = getComp = getNet = getLoc = "";
  wildSta = wildComp = wildNet = wildLoc = 1;
  if(argc > 2)
  {  /* at least station parameter given */
    getSta = argv[2];
    wildSta  = IsWild(getSta);
    if(argc > 3)
    {  /* channel (component) parameter given */
      getComp = argv[3];
      wildComp = IsWild(getComp);
      if(argc > 4)
      {  /* network parameter given */
        getNet = argv[4];
        wildNet = IsWild(getNet);
        if(argc > 5)
        {  /* location parameter given (SCNL) */
          getLoc  = argv[5];
          wildLoc = IsWild(getLoc);
        }
        else
        {  /* SCN without location parameter given */
          nLogo = 3;    /* do not include tracebuf2s in search */
        }
      }
    }
  }


  /* Attach to ring
  *****************/
  if ((RingKey = GetKey( inRing )) == -1 )
  {
    fprintf( stderr, "Invalid RingName; exiting!\n" );
    exit( -1 );
    return -1;
  }
  tport_attach( &region, RingKey );

/* Look up local installation id
   *****************************/
   if ( GetLocalInst( &InstId ) != 0 )
   {
      fprintf(stderr, "ring2mongo: error getting local installation id; exiting!\n" );
      exit( -1 );
      return -1;
   }

  /* Specify logos to get
  ***********************/
  if ( GetType( "TYPE_MSEED", &Type_Mseed ) != 0 ) {
     fprintf(stderr, "%s: WARNING: Invalid message type <TYPE_MSEED>! Missing from earthworm.d or earthworm_global.d\n", argv[0] );
     Type_Mseed = TYPE_NOTUSED;
  }
  if ( GetType( "TYPE_TRACEBUF", &Type_TraceBuf ) != 0 ) {
     fprintf(stderr, "%s: Invalid message type <TYPE_TRACEBUF>! Missing from earthworm.d or earthworm_global.d\n", argv[0] );
     exit( -1 );
     return -1;
  }
  if ( GetType( "TYPE_TRACE_COMP_UA", &Type_TraceComp ) != 0 ) {
     fprintf(stderr, "%s: Invalid message type <TYPE_TRACE_COMP_UA>! Missing from earthworm.d or earthworm_global.d\n", argv[0] );
     exit( -1 );
     return -1;
  }
  if ( GetType( "TYPE_TRACEBUF2", &Type_TraceBuf2 ) != 0 ) {
     fprintf(stderr, "%s: Invalid message type <TYPE_TRACEBUF2>! Missing from earthworm.d or earthworm_global.d\n", argv[0] );
     exit( -1 );
     return -1;
  }
  if ( GetType( "TYPE_TRACE2_COMP_UA", &Type_TraceComp2 ) != 0 ) {
     fprintf(stderr,"%s: Invalid message type <TYPE_TRACE2_COMP_UA>! Missing from earthworm.d or earthworm_global.d\n", argv[0] );
     exit( -1 );
     return -1;
  }
  if ( GetModId( "MOD_WILDCARD", &ModWildcard ) != 0 ) {
     fprintf(stderr, "%s: Invalid moduleid <MOD_WILDCARD>! Missing from earthworm.d or earthworm_global.d\n", argv[0] );
     exit( -1 );
     return -1;
  }
  if ( GetInst( "INST_WILDCARD", &InstWildcard ) != 0 ) {
     fprintf(stderr, "%s: Invalid instid <INST_WILDCARD>! Missing from earthworm.d or earthworm_global.d\n", argv[0] );
     exit( -1 );
     return -1;
  }

  for( i=0; i<nLogo; i++ ) {
      getlogo[i].instid = InstWildcard;
      getlogo[i].mod    = ModWildcard;
  }
  getlogo[0].type = Type_Mseed;
  getlogo[1].type = Type_TraceBuf;
  getlogo[2].type = Type_TraceComp;
  if (nLogo >= 4) {     /* if nLogo=5 then include TraceBuf2s */
      getlogo[3].type = Type_TraceBuf2;
      getlogo[4].type = Type_TraceComp2;
  }
  logit("", "Starting ring2mongo");
  /* Flush the ring
  *****************/
  while ( tport_copyfrom( &region, getlogo, nLogo, &logo, &gotsize,
            (char *)&msg, MAX_TRACEBUF_SIZ, &sequence_number ) != GET_NONE ){
         packet_total++;
         packet_total_size+=gotsize;
  }
  logit( "et",  "ring2mongo: inRing flushed %ld packets of %ld bytes total.\n",
	packet_total, packet_total_size);

  while (tport_getflag( &region ) != TERMINATE ) {
    rc = tport_copyfrom( &region, getlogo, nLogo,
               &logo, &gotsize, msg, MAX_TRACEBUF_SIZ, &sequence_number );

    if ( rc == GET_NONE ){
      sleep_ew( 200 );
      continue;
    }

    if ( rc == GET_TOOBIG ){
      logit("et", "ring2mongo: retrieved message too big (%ld) for msg\n",
        gotsize );
      continue;
    }
    if ( rc == GET_NOTRACK )
      logit("et", "ring2mongo: Tracking error.\n");

    if ( rc == GET_MISS_LAPPED )
      logit("et", "ring2mongo: Got lapped on the ring.\n");

    if ( rc == GET_MISS_SEQGAP )
      logit( "et", "ring2mongo: Gap in sequence numbers\n");

    if ( rc == GET_MISS )
      logit( "et", "ring2mongo: Missed messages\n");

    /* Check SCNL of the retrieved message */


    if (Type_Mseed != TYPE_NOTUSED && logo.type == Type_Mseed) {
      /* Unpack record header and not data samples */
      /*hard coded zero for dataflag(1) and verbose(0)for ring2mongo*/
      if ( msr_unpack (msg, gotsize, &msr, 1, 0) != MS_NOERROR) {
         logit("et", "Error parsing mseed record\n");
         continue;
      }

      /* Print record information */
      msr_print (msr, 0);

      msr_free (&msr);
      continue;
    }
    /*end Mseed*/
    
    if ( (wildSta  || (strcmp(getSta,trh->sta)  ==0)) &&
         (wildComp || (strcmp(getComp,trh->chan)==0)) &&
         (wildNet  || (strcmp(getNet,trh->net)  ==0)) &&
         (((logo.type == Type_TraceBuf2 ||
            logo.type == Type_TraceComp2) &&
         (wildLoc  || (strcmp(getLoc,trh->loc) == 0))) ||
         ( (logo.type == Type_TraceBuf ||
             logo.type == Type_TraceComp))))
    {
      
      
      strcpy(orig_datatype, trh->datatype);
      char scnl[20];     
      scnl[0] = 0;
      strcat( scnl, trh->sta);
      strcat( scnl, ".");
      strcat( scnl, trh->chan);
      strcat( scnl, ".");
      strcat( scnl, trh->net);
      strcat( scnl, ".");
      strcat( scnl, trh->loc);
      
      if(WaveMsg2MakeLocal( trh ) < 0){
        
        char  dt[3];        
        
        /* now put a space if there are any punctuation marks */

        for ( i=0; i<15; i++ ) {
          if ( !isalnum(scnl[i]) && !ispunct(scnl[i]))
            scnl[i] = ' ';
        }
        strncpy( dt, trh->datatype, 2 );
        for ( i=0; i<2; i++ ) {
          if ( !isalnum(dt[i]) && !ispunct(dt[i]))
            dt[i] = ' ';
        }
        dt[i] = 0;
        logit("et", "WARNING: WaveMsg2MakeLocal rejected tracebuf.  Discard (%s).\n",
          scnl );
        logit("et", "\tdatatype=[%s]\n", dt);
        continue;
      }
        
      /*lets get ready to mongo....*/
      if(m_count < MONGO_BULK_MAX){
        /*initialize when m_count ==0 */
        if(m_count==0){
          m_bulk = mongoc_collection_create_bulk_operation (m_collection, true, NULL);
        }       
        m_data = bson_new ();
        m_doc = BCON_NEW ("key",   BCON_UTF8 (scnl),
                          "nsamp", BCON_INT32 (trh->nsamp),
                          "starttime", BCON_DOUBLE (trh->starttime*1000),
                          "endtime", BCON_DOUBLE (trh->endtime*1000),
                          "samprate", BCON_DOUBLE (trh->samprate),
                          "datatype", BCON_UTF8 (trh->datatype)
        );
       char index[4];
       bson_append_array_begin (m_doc, "data", -1, m_data);
        for ( i = 0; i < trh->nsamp; i++ ){
          snprintf(index, 4, "%d", i);
          if ( (strcmp (trh->datatype, "s2")==0) || (strcmp (trh->datatype, "i2")==0) ){
            bson_append_int32 (m_data, index, -1, short_data[i]);
           }else{
             bson_append_int32 (m_data, index, -1, long_data[i]);
           }
          }
        bson_append_array_end (m_doc, m_data);                             
        mongoc_bulk_operation_insert (m_bulk, m_doc);
        bson_destroy (m_doc);
        bson_destroy (m_data);
        
        m_count++;
        if(m_count==MONGO_BULK_MAX){
          
          /*write and destroty above stuff */  
          m_ret = mongoc_bulk_operation_execute (m_bulk, &m_reply, &m_error);
          if (!m_ret){
             logit ("et", "Error: %s\n", m_error.message);
           }
          bson_destroy (&m_reply);
          mongoc_bulk_operation_destroy (m_bulk);
          m_count = 0;
        }
      } 
    }
  } /* end of while loop */
  logit("et", "signing off"); 
  exit (0);
  return 0;
}
