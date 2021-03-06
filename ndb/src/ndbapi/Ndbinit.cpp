/* Copyright (C) 2003 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */


#include <ndb_global.h>

#include "NdbApiSignal.hpp"
#include "NdbImpl.hpp"
#include "NdbOperation.hpp"
#include "NdbConnection.hpp"
#include "NdbRecAttr.hpp"
#include "IPCConfig.hpp"
#include "TransporterFacade.hpp"
#include "ConfigRetriever.hpp"
#include <ndb_limits.h>
#include <NdbOut.hpp>
#include <NdbSleep.h>
#include "ObjectMap.hpp"
#include <NdbIndexScanOperation.hpp>
#include <NdbIndexOperation.hpp>
#include "NdbUtil.hpp"
#include <NdbBlob.hpp>

class NdbGlobalEventBufferHandle;
NdbGlobalEventBufferHandle *NdbGlobalEventBuffer_init(int);
void NdbGlobalEventBuffer_drop(NdbGlobalEventBufferHandle *);

/**
 * Static object for NDB
 */

// only needed for backwards compatability, before ndb_cluster_connection
static char *ndbConnectString = 0;
static int theNoOfNdbObjects = 0;
static Ndb_cluster_connection *global_ndb_cluster_connection= 0;


/***************************************************************************
Ndb(const char* aDataBase);

Parameters:    aDataBase : Name of the database.
Remark:        Connect to the database.
***************************************************************************/
Ndb::Ndb( const char* aDataBase , const char* aSchema)
  : theImpl(NULL)
{
  DBUG_ENTER("Ndb::Ndb()");
  DBUG_PRINT("enter",("(old)Ndb::Ndb this=0x%x", this));
  if (theNoOfNdbObjects < 0)
    abort(); // old and new Ndb constructor used mixed
  theNoOfNdbObjects++;
  if (global_ndb_cluster_connection == 0) {
    global_ndb_cluster_connection= new Ndb_cluster_connection(ndbConnectString);
    global_ndb_cluster_connection->connect(12,5,1);
  }
  setup(global_ndb_cluster_connection, aDataBase, aSchema);
  DBUG_VOID_RETURN;
}

Ndb::Ndb( Ndb_cluster_connection *ndb_cluster_connection,
	  const char* aDataBase , const char* aSchema)
  : theImpl(NULL)
{
  DBUG_ENTER("Ndb::Ndb()");
  DBUG_PRINT("enter",("Ndb::Ndb this=0x%x", this));
  if (global_ndb_cluster_connection != 0 &&
      global_ndb_cluster_connection != ndb_cluster_connection)
    abort(); // old and new Ndb constructor used mixed
  theNoOfNdbObjects= -1;
  setup(ndb_cluster_connection, aDataBase, aSchema);
  DBUG_VOID_RETURN;
}

void Ndb::setup(Ndb_cluster_connection *ndb_cluster_connection,
	  const char* aDataBase , const char* aSchema)
{
  DBUG_ENTER("Ndb::setup");

  assert(theImpl == NULL);
  theImpl= new NdbImpl(ndb_cluster_connection,*this);
  theDictionary= &(theImpl->m_dictionary);

  thePreparedTransactionsArray= NULL;
  theSentTransactionsArray= NULL;
  theCompletedTransactionsArray= NULL;
  theNoOfPreparedTransactions= 0;
  theNoOfSentTransactions= 0;
  theNoOfCompletedTransactions= 0;
  theNoOfAllocatedTransactions= 0;
  theMaxNoOfTransactions= 0;
  theMinNoOfEventsToWakeUp= 0;
  prefixEnd= NULL;
  theTransactionList= NULL;
  theConnectionArray= NULL;
  the_last_check_time= 0;
  theFirstTransId= 0;
  theRestartGCI= 0;
  theNdbBlockNumber= -1;
  theInitState= NotConstructed;

  theNode= 0;
  theFirstTransId= 0;
  theMyRef= 0;

  fullyQualifiedNames = true;

#ifdef POORMANSPURIFY
  cgetSignals =0;
  cfreeSignals = 0;
  cnewSignals = 0;
  creleaseSignals = 0;
#endif

  theError.code = 0;

  theConnectionArray = new NdbConnection * [MAX_NDB_NODES];
  theCommitAckSignal = NULL;
  
  int i;
  for (i = 0; i < MAX_NDB_NODES ; i++) {
    theConnectionArray[i] = NULL;
  }//forg
  for (i = 0; i < 2048 ; i++) {
    theFirstTupleId[i] = 0;
    theLastTupleId[i] = 0;
  }//for
  
  BaseString::snprintf(theDataBase, sizeof(theDataBase), "%s",
           aDataBase ? aDataBase : "");
  BaseString::snprintf(theDataBaseSchema, sizeof(theDataBaseSchema), "%s",
	   aSchema ? aSchema : "");

  int len = BaseString::snprintf(prefixName, sizeof(prefixName), "%s%c%s%c",
                     theDataBase, table_name_separator,
                     theDataBaseSchema, table_name_separator);
  prefixEnd = prefixName + (len < (int) sizeof(prefixName) ? len : 
                            sizeof(prefixName) - 1);

  theImpl->theWaiter.m_mutex =  TransporterFacade::instance()->theMutexPtr;

  // Signal that the constructor has finished OK
  if (theInitState == NotConstructed)
    theInitState = NotInitialised;

  {
    NdbGlobalEventBufferHandle *h=
      NdbGlobalEventBuffer_init(NDB_MAX_ACTIVE_EVENTS);
    if (h == NULL) {
      ndbout_c("Failed NdbGlobalEventBuffer_init(%d)",NDB_MAX_ACTIVE_EVENTS);
      exit(-1);
    }
    theGlobalEventBufferHandle = h;
  }

  DBUG_VOID_RETURN;
}


void Ndb::setConnectString(const char * connectString)
{
  if (ndbConnectString != 0) {
    free(ndbConnectString);
    ndbConnectString = 0;
  }
  if (connectString)
    ndbConnectString = strdup(connectString);
}

/*****************************************************************************
 * ~Ndb();
 *
 * Remark:        Disconnect with the database. 
 *****************************************************************************/
Ndb::~Ndb()
{ 
  DBUG_ENTER("Ndb::~Ndb()");
  DBUG_PRINT("enter",("Ndb::~Ndb this=0x%x",this));
  doDisconnect();

  NdbGlobalEventBuffer_drop(theGlobalEventBufferHandle);

  if (TransporterFacade::instance() != NULL && theNdbBlockNumber > 0){
    TransporterFacade::instance()->close(theNdbBlockNumber, theFirstTransId);
  }
  
  releaseTransactionArrays();

  delete []theConnectionArray;
  if(theCommitAckSignal != NULL){
    delete theCommitAckSignal; 
    theCommitAckSignal = NULL;
  }

  delete theImpl;

  /**
   * This needs to be put after delete theImpl
   *  as TransporterFacade::instance is delete by global_ndb_cluster_connection
   *  and used by theImpl
   */
  if (global_ndb_cluster_connection != 0) {
    theNoOfNdbObjects--;
    if(theNoOfNdbObjects == 0){
      delete global_ndb_cluster_connection;
      global_ndb_cluster_connection= 0;
    }
  }//if

  /** 
   *  This sleep is to make sure that the transporter 
   *  send thread will come in and send any
   *  signal buffers that this thread may have allocated.
   *  If that doesn't happen an error will occur in OSE
   *  when trying to restore a signal buffer allocated by a thread
   *  that have been killed.
   */
#ifdef NDB_OSE
  NdbSleep_MilliSleep(50);
#endif

#ifdef POORMANSPURIFY
#ifdef POORMANSGUI
  ndbout << "cnewSignals=" << cnewSignals << endl;
  ndbout << "cfreeSignals=" << cfreeSignals << endl;
  ndbout << "cgetSignals=" << cgetSignals << endl;
  ndbout << "creleaseSignals=" << creleaseSignals << endl;
#endif
  // Poor mans purifier
  assert(cnewSignals == cfreeSignals);
  assert(cgetSignals == creleaseSignals);
#endif
  DBUG_VOID_RETURN;
}

NdbWaiter::NdbWaiter(){
  m_node = 0;
  m_state = NO_WAIT;
  m_mutex = 0;
  m_condition = NdbCondition_Create();
}

NdbWaiter::~NdbWaiter(){
  NdbCondition_Destroy(m_condition);
}

NdbImpl::NdbImpl(Ndb_cluster_connection *ndb_cluster_connection,
		 Ndb& ndb)
  : m_ndb_cluster_connection(ndb_cluster_connection->m_impl),
    m_dictionary(ndb),
    theCurrentConnectIndex(0),
    theNdbObjectIdMap(ndb_cluster_connection->m_impl.m_transporter_facade->theMutexPtr,
		      1024,1024),
    theNoOfDBnodes(0)
{
  int i;
  for (i = 0; i < MAX_NDB_NODES; i++) {
    the_release_ind[i] = 0;
  }
  m_optimized_node_selection=
    m_ndb_cluster_connection.m_optimized_node_selection;
}

NdbImpl::~NdbImpl()
{
}

