/*--------------------------------------------------------------------------
 * dr_private.h
 *	header file for dynamic reduce private
 *
 * src/include/utils/dr_private.h
 *--------------------------------------------------------------------------
 */
#ifndef DR_PRIVATE_H_
#define DR_PRIVATE_H_

#include "access/tupdesc.h"
#include "access/tuptypeconvert.h"
#include "lib/oidbuffer.h"
#include "lib/stringinfo.h"
#include "storage/latch.h"
#include "storage/shm_mq.h"
#include "utils/hsearch.h"

/* Identifier for shared memory segments used by this extension. */
#define ADB_DYNAMIC_REDUCE_MQ_MAGIC		0xa553038f
#define ADB_DR_MQ_BACKEND_SENDER		0
#define ADB_DR_MQ_WORKER_SENDER			1
#define INVALID_EVENT_SET_POS			(-1)
#define INVALID_PLAN_ID					(-1)

#define DR_WAIT_EVENT_SIZE_STEP			32
#define DR_HTAB_DEFAULT_SIZE			1024
#define DR_SOCKET_BUF_SIZE_START		32768
#define DR_SOCKET_BUF_SIZE_STEP			4096

/* worker and reduce MQ message */
#define ADB_DR_MQ_MSG_CONFIRM			'\x01'
#define ADB_DR_MQ_MSG_PORT				'\x02'
#define ADB_DR_MQ_MSG_RESET				'\x10'
#define ADB_DR_MQ_MSG_STARTUP			'\x11'
#define ADB_DR_MQ_MSG_CONNECT			'\x12'
#define ADB_DR_MQ_MSG_START_PLAN_NORMAL	'\x13'

#define ADB_DR_MSG_INVALID				'\x00'
#define ADB_DR_MSG_NODEOID				'\x01'
#define ADB_DR_MSG_TUPLE				'\x02'
#define ADB_DR_MSG_END_OF_PLAN			'\x03'

/* define DRD_CONNECT 1 */
#ifdef DRD_CONNECT
#define DR_CONNECT_DEBUG(rest) ereport_domain(LOG_SERVER_ONLY, PG_TEXTDOMAIN("DynamicReduce"), rest)
#else
#define DR_CONNECT_DEBUG(rest) ((void*)0)
#endif

/* define DRD_PLAN 1 */
#ifdef DRD_PLAN
#define DR_PLAN_DEBUG(rest)	ereport_domain(LOG_SERVER_ONLY, PG_TEXTDOMAIN("DynamicReduce"), rest)
#else
#define DR_PLAN_DEBUG(rest)	((void*)0)
#endif

#define DRD_PLAN_EOF 1
#ifdef DRD_PLAN_EOF
#define DR_PLAN_DEBUG_EOF(rest) ereport_domain(LOG_SERVER_ONLY, PG_TEXTDOMAIN("DynamicReduce"), rest)
#else
#define DR_PLAN_DEBUG_EOF(rest)	((void*)0)
#endif

/* define DRD_NODE 1 */
#ifdef DRD_NODE
#define DR_NODE_DEBUG(rest)	ereport_domain(LOG_SERVER_ONLY, PG_TEXTDOMAIN("DynamicReduce"), rest)
#else
#define DR_NODE_DEBUG(rest)	((void*)0)
#endif


typedef enum DR_STATUS
{
	DRS_STARTUPED = 1,		/* ready for listen */
	DRS_LISTENED,			/* listen created */
	DRS_RESET,
	DRS_CONNECTING,			/* connecting network */
	DRS_CONNECTED			/* ready for reduce */
}DR_STATUS;

/* event structs */
typedef enum DR_EVENT_DATA_TYPE
{
	DR_EVENT_DATA_POSTMASTER = 1,
	DR_EVENT_DATA_LATCH,
	DR_EVENT_DATA_LISTEN,
	DR_EVENT_DATA_NODE
}DR_EVENT_DATA_TYPE;

typedef enum DR_NODE_STATUS
{
	DRN_ACCEPTED = 1,		/* initialized for accept */
	DRN_CONNECTING,			/* connecting from local */
	DRN_WORKING,			/* sended or received node oid */
	DRN_FAILED,				/* got error message, don't try send or recv any message */
	DRN_WAIT_CLOSE
}DR_NODE_STATUS;

typedef struct DREventData
{
	DR_EVENT_DATA_TYPE type;
	void (*OnEvent)(WaitEvent *ev);
	void (*OnPreWait)(struct DREventData *base, int pos);	/* can be null */
	void (*OnError)(struct DREventData *base, int pos);		/* can be null */
}DREventData;

typedef DREventData DRPostmasterEventData;

typedef struct DRListenEventData
{
	DREventData	base;
	uint16		port;
	bool		noblock;
}DRListenEventData;
//static int dr_listen_pos = INVALID_EVENT_SET_POS;

typedef struct DRLatchEventData
{
	DREventData		base;

	OidBufferData	net_oid_buf;	/* not include myself */
	OidBufferData	work_oid_buf;	/* not include myself */
	OidBufferData	work_pid_buf;	/* not include myself */
}DRLatchEventData;
extern DRLatchEventData *dr_latch_data;

typedef struct DRNodeEventData
{
	DREventData		base;
	StringInfoData	sendBuf;
	DR_NODE_STATUS	status;
	StringInfoData	recvBuf;
	Oid				nodeoid;
	int				owner_pid;	/* remote owner */

	/*
	 * when it is not INVALID_PLAN_ID,
	 * it waiting plan connect to me or send buffer changed
	 */
	int				waiting_plan_id;

	struct addrinfo *addrlist;
	struct addrinfo *addr_cur;
}DRNodeEventData;

typedef struct PlanWorkerInfo
{
	int				worker_id;			/* -1 for main */

	/*
	 * when not InvalidOid,
	 * waiting for remote node connect or send buffer change
	 */
	Oid				waiting_node;

	shm_mq_handle  *worker_sender;
	shm_mq_handle  *reduce_sender;
	TupleTableSlot *slot_plan_src;		/* type convert for recv tuple in from plan */
	TupleTableSlot *slot_plan_dest;		/* type convert for recv tuple out from plan */
	TupleTableSlot *slot_node_src;		/* type convert for recv tuple in from other node */
	TupleTableSlot *slot_node_dest;		/* type convert for recv tuple out from other node */
	void		   *last_data;			/* last received data from worker */
	Oid			   *dest_oids;
	uint32			dest_count;
	uint32			dest_cursor;
	Size			last_size;			/* last received data size */
	StringInfoData	sendBuffer;			/* cached data for send to worker */
	char			last_msg_type;		/* last message type for waiting send */
	bool			end_of_plan_recv;
	bool			end_of_plan_send;
}PlanWorkerInfo;

typedef struct PlanInfo PlanInfo;
struct PlanInfo
{
	int				plan_id;

	/*
	 * when not InvalidOid,
	 * waiting for remote node connect or send buffer change
	 */
	Oid				waiting_node;

	void (*OnLatchSet)(PlanInfo *pi);
	bool (*OnNodeRecvedData)(PlanInfo *pi, const char *data, int len, Oid nodeoid);
	void (*OnNodeIdle)(PlanInfo *pi, WaitEvent *we, DRNodeEventData *ned);
	bool (*OnNodeEndOfPlan)(PlanInfo *pi, Oid nodeoid);
	void (*OnPlanError)(PlanInfo *pi);
	void (*OnDestroy)(PlanInfo *pi);
	void (*OnPreWait)(PlanInfo *pi);

	TupleTypeConvert   *type_convert;
	MemoryContext		convert_context;

	dsm_segment		   *seg;
	OidBufferData		end_of_plan_nodes;
	PlanWorkerInfo	   *pwi;
};

/* public variables */
extern struct WaitEventSet *dr_wait_event_set;
extern struct WaitEvent	   *dr_wait_event;
extern Size					dr_wait_count;
extern Size					dr_wait_max;
extern DR_STATUS			dr_status;

/* public shared memory variables in dr_shm.c */
extern dsm_segment		   *dr_mem_seg;
extern shm_mq_handle	   *dr_mq_backend_sender;
extern shm_mq_handle	   *dr_mq_worker_sender;

/* public function */
void DRCheckStarted(void);
void DREnlargeWaitEventSet(void);
void DRGetEndOfPlanMessage(PlanWorkerInfo *pwi);

/* public plan functions in plan_public.c */
bool DRSendPlanWorkerMessage(PlanWorkerInfo *pwi, PlanInfo *pi);
bool DRRecvPlanWorkerMessage(PlanWorkerInfo *pwi, PlanInfo *pi);
void ActiveWaitingPlan(DRNodeEventData *ned);
void DRSetupPlanTypeConvert(PlanInfo *pi, TupleDesc desc);
void DRSetupPlanWorkTypeConvert(PlanInfo *pi, PlanWorkerInfo *pwi);
TupleTableSlot* DRStoreTypeConvertTuple(TupleTableSlot *slot, const char *data, uint32 len, HeapTuple head);
void DRInitPlanSearch(void);
PlanInfo* DRPlanSearch(int planid, HASHACTION action, bool *found);
bool DRPlanSeqInit(HASH_SEQ_STATUS *seq);


/* normal plan functions in plan_normal.c */
void DRStartNormalPlanMessage(StringInfo msg);


/* connect functions in dr_connect.c */
void FreeNodeEventInfo(DRNodeEventData *ned);
void ConnectToAllNode(const DynamicReduceNodeInfo *info, uint32 count);
DRListenEventData* GetListenEventData(void);

/* node functions in dr_node.c */
void DROnNodeConectSuccess(DRNodeEventData *ned, WaitEvent *ev);
bool PutMessageToNode(DRNodeEventData *ned, char msg_type, const char *data, uint32 len, int plan_id);
ssize_t RecvMessageFromNode(DRNodeEventData *ned, WaitEvent *ev);
void DRNodeReset(DRNodeEventData *ned);
void DRActiveNode(int planid);
void DRInitNodeSearch(void);
DRNodeEventData* DRSearchNodeEventData(Oid nodeoid, HASHACTION action, bool *found);

/* dynamic reduce utils functions in dr_utils.c */
bool DynamicReduceHandleMessage(void *data, Size len);
void SerializeTupleDesc(StringInfo buf, TupleDesc desc);
TupleDesc RestoreTupleDesc(StringInfo buf);
void DRConnectNetMsg(StringInfo msg);
void DRUtilsReset(void);
void DRUtilsAbort(void);

/* dynamic reduce shared memory functions in dr_shm.c */
void DRSetupShmem(void);
void DRResetShmem(void);
void DRAttachShmem(Datum datum);
void DRDetachShmem(void);

bool DRSendMsgToReduce(const char *data, Size len, bool nowait);
bool DRRecvMsgFromReduce(Size *sizep, void **datap, bool nowait);
bool DRSendMsgToBackend(const char *data, Size len, bool nowait);
bool DRRecvMsgFromBackend(Size *sizep, void **datap, bool nowait);

bool DRSendConfirmToBackend(bool nowait);
bool DRRecvConfirmFromReduce(bool nowait);

#endif							/* DR_PRIVATE_H_ */
