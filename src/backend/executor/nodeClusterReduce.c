
#include "postgres.h"
#include "miscadmin.h"

#include "access/parallel.h"
#include "executor/executor.h"
#include "executor/nodeClusterReduce.h"
#include "executor/nodeCtescan.h"
#include "executor/nodeMaterial.h"
#include "executor/nodeReduceScan.h"
#include "executor/tuptable.h"
#include "lib/binaryheap.h"
#include "lib/oidbuffer.h"
#include "nodes/enum_funcs.h"
#include "nodes/execnodes.h"
#include "nodes/nodeFuncs.h"
#include "pgstat.h"
#include "pgxc/pgxc.h"
#include "storage/buffile.h"
#include "storage/shm_mq.h"
#include "storage/condition_variable.h"
#include "utils/dynamicreduce.h"
#include "utils/hsearch.h"

typedef enum ReduceType
{
	RT_NOTHING = 1,
	RT_NORMAL,
	RT_ADVANCE,
	RT_ADVANCE_PARALLEL,
	RT_MERGE
}ReduceType;

typedef struct NormalReduceState
{
	dsm_segment	   *dsm_seg;
	DynamicReduceIOBuffer
					drio;
}NormalReduceState;

typedef struct AdvanceNodeInfo
{
	BufFile			   *file;
	Oid					nodeoid;
}AdvanceNodeInfo;

typedef struct AdvanceReduceState
{
	NormalReduceState	normal;
	StringInfoData		read_buf;
	uint32				nnodes;
	bool				got_remote;
	AdvanceNodeInfo	   *cur_node;
	AdvanceNodeInfo		nodes[FLEXIBLE_ARRAY_MEMBER];
}AdvanceReduceState;

#define APR_NPART			2
#define APR_BACKEND_PART	0
#define APR_REDUCE_PART		1

typedef struct AdvanceParallelSharedMemory
{
	ConditionVariable	cv;
	pg_atomic_flag		got_remote;
	char				padding[(sizeof(ConditionVariable)+sizeof(pg_atomic_flag))%MAXIMUM_ALIGNOF ? 
								MAXIMUM_ALIGNOF-(sizeof(ConditionVariable)+sizeof(pg_atomic_flag))%MAXIMUM_ALIGNOF : 0];
	DynamicReduceSTSData sts;
}AdvanceParallelSharedMemory;

typedef struct AdvanceParallelNode
{
	SharedTuplestoreAccessor
					   *accessor;
	Oid					nodeoid;
}AdvanceParallelNode;

typedef struct AdvanceParallelState
{
	NormalReduceState	normal;
	uint32				nnodes;
	AdvanceParallelSharedMemory
					   *shm;
	AdvanceParallelNode *cur_node;
	AdvanceParallelNode	nodes[FLEXIBLE_ARRAY_MEMBER];
}AdvanceParallelState;

typedef struct MergeNodeInfo
{
	TupleTableSlot	   *slot;
	BufFile			   *file;
	StringInfoData		read_buf;
	Oid					nodeoid;
}MergeNodeInfo;

typedef struct MergeReduceState
{
	NormalReduceState	normal;
	MergeNodeInfo	   *nodes;
	binaryheap		   *binheap;
	SortSupport			sortkeys;
	uint32				nkeys;
	uint32				nnodes;
}MergeReduceState;

extern bool enable_cluster_plan;

static int cmr_heap_compare_slots(Datum a, Datum b, void *arg);
static bool DriveClusterReduceState(ClusterReduceState *node);
static bool DriveCteScanState(PlanState *node);
static bool DriveMaterialState(PlanState *node);
static bool DriveClusterReduceWalker(PlanState *node);
static bool IsThereClusterReduce(PlanState *node);

/* ======================= nothing reduce========================== */
static TupleTableSlot* ExecNothingReduce(PlanState *pstate)
{
	return ExecClearTuple(pstate->ps_ResultTupleSlot);
}

/* ======================= normal reduce ========================== */
static TupleTableSlot* ExecNormalReduce(PlanState *pstate)
{
	ClusterReduceState *node = castNode(ClusterReduceState, pstate);
	NormalReduceState  *normal = node->private_state;
	TupleTableSlot *slot;
	Assert(normal != NULL && node->reduce_method == RT_NORMAL);

	slot = DynamicReduceFetchSlot(&normal->drio);
	if (TupIsNull(slot))
	{
		shm_mq_detach(normal->drio.mqh_receiver);
		normal->drio.mqh_receiver = NULL;
		shm_mq_detach(normal->drio.mqh_sender);
		normal->drio.mqh_sender = NULL;
	}
	return slot;
}

static TupleTableSlot* ExecReduceFetchLocal(void *pstate, ExprContext *econtext)
{
	TupleTableSlot *slot = ExecProcNode(pstate);
	econtext->ecxt_outertuple = slot;
	return slot;
}

static void SetupNormalReduceState(NormalReduceState *normal, DynamicReduceMQ drmq,
								   ClusterReduceState *crstate, bool init);
static void InitNormalReduceState(NormalReduceState *normal, Size shm_size, ClusterReduceState *crstate)
{
	normal->dsm_seg = dsm_create(shm_size, 0);
	SetupNormalReduceState(normal, dsm_segment_address(normal->dsm_seg), crstate, true);
}
static void SetupNormalReduceState(NormalReduceState *normal, DynamicReduceMQ drmq,
								   ClusterReduceState *crstate, bool init)
{
	Expr			   *expr;
	ClusterReduce	   *plan;

	DynamicReduceInitFetch(&normal->drio,
						   normal->dsm_seg,
						   crstate->ps.ps_ResultTupleSlot->tts_tupleDescriptor,
						   drmq->worker_sender_mq, init ? sizeof(drmq->worker_sender_mq):0,
						   drmq->reduce_sender_mq, init ? sizeof(drmq->reduce_sender_mq):0);
	normal->drio.econtext = crstate->ps.ps_ExprContext;
	normal->drio.FetchLocal = ExecReduceFetchLocal;
	normal->drio.user_data = outerPlanState(crstate);

	/* init reduce expr */
	plan = castNode(ClusterReduce, crstate->ps.plan);
	if(plan->special_node == PGXCNodeOid)
	{
		Assert(plan->special_reduce != NULL);
		expr = plan->special_reduce;
	}else
	{
		expr = plan->reduce;
	}
	Assert(expr != NULL);
	normal->drio.expr_state = ExecInitReduceExpr(expr);
}
static void InitNormalReduce(ClusterReduceState *crstate)
{
	MemoryContext		oldcontext;
	NormalReduceState  *normal;
	Assert(crstate->private_state == NULL);

	oldcontext = MemoryContextSwitchTo(GetMemoryChunkContext(crstate));
	normal = palloc0(sizeof(NormalReduceState));
	InitNormalReduceState(normal, sizeof(DynamicReduceMQData), crstate);
	crstate->private_state = normal;
	ExecSetExecProcNode(&crstate->ps, ExecNormalReduce);
	DynamicReduceStartNormalPlan(crstate->ps.plan->plan_node_id, 
								 normal->dsm_seg,
								 dsm_segment_address(normal->dsm_seg),
								 crstate->ps.ps_ResultTupleSlot->tts_tupleDescriptor,
								 castNode(ClusterReduce, crstate->ps.plan)->reduce_oids,
								 true);
	MemoryContextSwitchTo(oldcontext);
}

static void InitParallelReduce(ClusterReduceState *crstate, ParallelContext *pcxt)
{
	MemoryContext		oldcontext;
	NormalReduceState  *normal;
	DynamicReduceMQ		drmq;
	char			   *addr;
	int					i;
	Assert(crstate->private_state == NULL);

	addr = shm_toc_allocate(pcxt->toc, sizeof(Size) + (pcxt->nworkers+1) * sizeof(DynamicReduceMQData));
	*(Size*)addr = RT_NORMAL;
	drmq = (DynamicReduceMQ)(addr + sizeof(Size));
	for(i=0;i<=pcxt->nworkers;++i)
	{
		shm_mq_create(drmq[i].reduce_sender_mq, sizeof(drmq->reduce_sender_mq));
		shm_mq_create(drmq[i].worker_sender_mq, sizeof(drmq->worker_sender_mq));
	}
	shm_toc_insert(pcxt->toc, crstate->ps.plan->plan_node_id, addr);

	oldcontext = MemoryContextSwitchTo(GetMemoryChunkContext(crstate));
	normal = palloc0(sizeof(NormalReduceState));
	SetupNormalReduceState(normal, drmq, crstate, false);
	crstate->private_state = normal;
	ExecSetExecProcNode(&crstate->ps, ExecNormalReduce);
	MemoryContextSwitchTo(oldcontext);
}
static void StartParallelReduce(ClusterReduceState *crstate, ParallelContext *pcxt)
{
	DynamicReduceMQ		drmq;

	char* addr = shm_toc_lookup(pcxt->toc, crstate->ps.plan->plan_node_id, false);
	drmq = (DynamicReduceMQ)(addr + sizeof(Size));
	if (pcxt->nworkers_launched == 0)
		DynamicReduceStartNormalPlan(crstate->ps.plan->plan_node_id, 
									 pcxt->seg,
									 drmq,
									 crstate->ps.ps_ResultTupleSlot->tts_tupleDescriptor,
									 castNode(ClusterReduce, crstate->ps.plan)->reduce_oids,
									 true);
	else
		DynamicReduceStartParallelPlan(crstate->ps.plan->plan_node_id,
									   pcxt->seg,
									   drmq,
									   crstate->ps.ps_ResultTupleSlot->tts_tupleDescriptor,
									   castNode(ClusterReduce, crstate->ps.plan)->reduce_oids,
									   pcxt->nworkers_launched+1,
									   true);
}
static void InitParallelReduceWorker(ClusterReduceState *crstate, ParallelWorkerContext *pwcxt, char *addr)
{
	MemoryContext		oldcontext;
	NormalReduceState  *normal;
	DynamicReduceMQ		drmq;
	Assert(crstate->private_state == NULL);

	drmq = (DynamicReduceMQ)(addr);
	drmq = &drmq[ParallelWorkerNumber+1];

	oldcontext = MemoryContextSwitchTo(GetMemoryChunkContext(crstate));
	normal = palloc0(sizeof(NormalReduceState));
	SetupNormalReduceState(normal, drmq, crstate, false);
	crstate->private_state = normal;
	ExecSetExecProcNode(&crstate->ps, ExecNormalReduce);
	MemoryContextSwitchTo(oldcontext);
}
static inline void EstimateNormalReduce(ParallelContext *pcxt)
{
	shm_toc_estimate_chunk(&pcxt->estimator,
						   sizeof(Size) + (pcxt->nworkers+1) * sizeof(DynamicReduceMQData));
	shm_toc_estimate_keys(&pcxt->estimator, 1);
}
static void EndNormalReduce(NormalReduceState *normal)
{
	DynamicReduceClearFetch(&normal->drio);
	if (normal->dsm_seg)
		dsm_detach(normal->dsm_seg);
}
static void DriveNormalReduce(ClusterReduceState *node)
{
	TupleTableSlot	   *slot;
	NormalReduceState  *normal = node->private_state;

	if (normal->drio.eof_local == false ||
		normal->drio.eof_remote == false ||
		normal->drio.send_buf.len > 0)
	{
		do
		{
			slot = DynamicReduceFetchSlot(&normal->drio);
		}while(!TupIsNull(slot));
	}
}

/* ========================= advance reduce ========================= */
static TupleTableSlot *ExecAdvanceReduce(PlanState *pstate)
{
	AdvanceReduceState *state = castNode(ClusterReduceState, pstate)->private_state;
	AdvanceNodeInfo *cur_info = state->cur_node;
	TupleTableSlot *slot;

re_get_:
	slot = DynamicReduceReadSFSTuple(pstate->ps_ResultTupleSlot, cur_info->file, &state->read_buf);
	if (TupIsNull(slot))
	{
		if (cur_info->nodeoid == PGXCNodeOid &&
			state->got_remote == false)
		{
			char name[MAXPGPATH];
			MemoryContext oldcontext;
			AdvanceNodeInfo *info;
			DynamicReduceSFS sfs = dsm_segment_address(state->normal.dsm_seg);
			uint32 i;
			uint8 flag PG_USED_FOR_ASSERTS_ONLY;

			/* wait dynamic reduce end of plan */
			flag = DynamicReduceRecvTuple(state->normal.drio.mqh_receiver,
										  slot,
										  &state->normal.drio.recv_buf,
										  NULL,
										  false);
			Assert(flag == DR_MSG_RECV && TupIsNull(slot));
			state->got_remote = true;

			/* open remote SFS files */
			oldcontext = MemoryContextSwitchTo(GetMemoryChunkContext(state));
			for (i=0;i<state->nnodes;++i)
			{
				info = &state->nodes[i];
				if (info->file == NULL)
				{
					info->file = BufFileOpenShared(&sfs->sfs,
												   DynamicReduceSFSFileName(name, info->nodeoid));
				}else
				{
					Assert(info->nodeoid == PGXCNodeOid);
				}
			}
			MemoryContextSwitchTo(oldcontext);
		}

		/* next node */
		cur_info = &cur_info[1];
		if (cur_info >= &state->nodes[state->nnodes])
			cur_info = state->nodes;
		if (cur_info->nodeoid != PGXCNodeOid)
			goto re_get_;
	}

	return slot;
}

static void BeginAdvanceReduce(ClusterReduceState *crstate)
{
	MemoryContext		oldcontext = MemoryContextSwitchTo(GetMemoryChunkContext(crstate));
	AdvanceReduceState *state;
	DynamicReduceSFS	sfs;
	AdvanceNodeInfo	   *myinfo;
	TupleTableSlot	   *slot;
	List			   *reduce_oids;
	ListCell		   *lc;
	uint32 				i,count;

	reduce_oids = castNode(ClusterReduce, crstate->ps.plan)->reduce_oids;
	count = list_length(reduce_oids);
	if (count == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Can not find working nodes")));
	}

	state = palloc0(offsetof(AdvanceReduceState, nodes) + sizeof(state->nodes[0]) * count);
	crstate->private_state = state;
	crstate->reduce_method = RT_ADVANCE;
	state->nnodes = count;
	initStringInfo(&state->read_buf);
	InitNormalReduceState(&state->normal, sizeof(*sfs), crstate);
	sfs = dsm_segment_address(state->normal.dsm_seg);
	SharedFileSetInit(&sfs->sfs, state->normal.dsm_seg);

	myinfo = NULL;
	lc = list_head(reduce_oids);
	for(i=0;i<count;++i)
	{
		AdvanceNodeInfo *info = &state->nodes[i];
		info->nodeoid = lfirst_oid(lc);
		lc = lnext(lc);
		if (info->nodeoid == PGXCNodeOid)
		{
			char name[MAXPGPATH];
			info->file = BufFileCreateShared(&sfs->sfs,
											 DynamicReduceSFSFileName(name, info->nodeoid));
			Assert(myinfo == NULL);
			myinfo = info;
		}
	}
	Assert(myinfo != NULL);

	DynamicReduceStartSharedFileSetPlan(crstate->ps.plan->plan_node_id,
										state->normal.dsm_seg,
										dsm_segment_address(state->normal.dsm_seg),
										crstate->ps.ps_ResultTupleSlot->tts_tupleDescriptor,
										reduce_oids);

	while (state->normal.drio.eof_local == false)
	{
		slot = DynamicReduceFetchLocal(&state->normal.drio);
		if (state->normal.drio.send_buf.len > 0)
		{
			DynamicReduceSendMessage(state->normal.drio.mqh_sender,
									 state->normal.drio.send_buf.len,
									 state->normal.drio.send_buf.data,
									 false);
			state->normal.drio.send_buf.len = 0;
		}
		if (!TupIsNull(slot))
			DynamicReduceWriteSFSTuple(slot, myinfo->file);
	}
	if (BufFileSeek(myinfo->file, 0, 0, SEEK_SET) != 0)
	{
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("can not seek SFS file to head")));
	}
	state->cur_node = myinfo;

	ExecSetExecProcNode(&crstate->ps, ExecAdvanceReduce);

	MemoryContextSwitchTo(oldcontext);
}

static void EndAdvanceReduce(AdvanceReduceState *state, ClusterReduceState *crs)
{
	uint32				i,count;
	AdvanceNodeInfo	   *info;
	DynamicReduceSFS	sfs = dsm_segment_address(state->normal.dsm_seg);
	char				name[MAXPGPATH];

	if (state->got_remote == false)
	{
		uint8 flag PG_USED_FOR_ASSERTS_ONLY;
		flag = DynamicReduceRecvTuple(state->normal.drio.mqh_receiver,
									  crs->ps.ps_ResultTupleSlot,
									  &state->normal.drio.recv_buf,
									  NULL,
									  false);
		Assert(flag == DR_MSG_RECV && TupIsNull(crs->ps.ps_ResultTupleSlot));
	}

	for (i=0,count=state->nnodes;i<count;++i)
	{
		info = &state->nodes[i];
		if(info->file)
			BufFileClose(info->file);
		BufFileDeleteShared(&sfs->sfs, DynamicReduceSFSFileName(name, info->nodeoid));
	}
	EndNormalReduce(&state->normal);
}

/* ========================= advance parallel ======================= */
static TupleTableSlot* ExecAdvanceParallelReduce(PlanState *ps)
{
	AdvanceParallelState   *state = castNode(ClusterReduceState, ps)->private_state;
	AdvanceParallelNode	   *cur_node = state->cur_node;
	MinimalTuple			mtup;

re_get_:
	mtup = sts_parallel_scan_next(cur_node->accessor, NULL);
	if (mtup != NULL)
		return ExecStoreMinimalTuple(mtup, ps->ps_ResultTupleSlot, false);

	sts_end_parallel_scan(cur_node->accessor);
	if (cur_node->nodeoid == PGXCNodeOid)
	{
		AdvanceParallelSharedMemory *shm = state->shm;
		if (IsParallelWorker())
		{
			while(pg_atomic_unlocked_test_flag(&shm->got_remote))
				ConditionVariableSleep(&shm->cv, WAIT_EVENT_DATA_FILE_READ);

			ConditionVariableCancelSleep();
		}else
		{
			uint8 flag PG_USED_FOR_ASSERTS_ONLY;
			Assert(pg_atomic_unlocked_test_flag(&shm->got_remote));
			/* wait dynamic reduce end of plan */
			flag = DynamicReduceRecvTuple(state->normal.drio.mqh_receiver,
										  ps->ps_ResultTupleSlot,
										  &state->normal.drio.recv_buf,
										  NULL,
										  false);
			Assert(flag == DR_MSG_RECV && TupIsNull(ps->ps_ResultTupleSlot));

			if (pg_atomic_test_set_flag(&shm->got_remote) == false)
				ereport(ERROR,
						(errcode(ERRCODE_DATA_CORRUPTED),
						 errmsg("set got remote flag failed")));

			ConditionVariableBroadcast(&shm->cv);
		}
	}

	cur_node = &cur_node[1];
	if (cur_node >= &state->nodes[state->nnodes])
		cur_node = state->nodes;
	if (cur_node->nodeoid != PGXCNodeOid)
	{
		sts_begin_parallel_scan(cur_node->accessor);
		state->cur_node = cur_node;
		goto re_get_;
	}

	return ExecClearTuple(ps->ps_ResultTupleSlot);
}

static AdvanceParallelNode* BeginAdvanceSharedTuplestore(AdvanceParallelNode *arr, DynamicReduceSTS sts,
														 List *oids, bool init)
{
	char				name[MAXPGPATH];
	ListCell		   *lc;
	SharedTuplestore   *addr;
	AdvanceParallelNode *my_node;
	uint32				i;

	Assert(list_member_oid(oids, PGXCNodeOid));

	i = 0;
	foreach (lc, oids)
	{
		if (lfirst_oid(lc) == PGXCNodeOid)
			continue;

		arr[i].nodeoid = lfirst_oid(lc);
		addr = DRSTSD_ADDR(sts->sts, APR_NPART, i);
		if (init)
		{
			arr[i].accessor = sts_initialize(addr,
											 APR_NPART,
											 APR_BACKEND_PART,
											 0,
											 SHARED_TUPLESTORE_SINGLE_PASS,
											 &sts->sfs.sfs,
											 DynamicReduceSFSFileName(name, lfirst_oid(lc)));
		}else
		{
			arr[i].accessor = sts_attach_read_only(addr, &sts->sfs.sfs);
		}
		++i;
	}

	Assert(i == list_length(oids)-1);
	my_node = &arr[i];
	my_node->nodeoid = PGXCNodeOid;
	addr = DRSTSD_ADDR(sts->sts, APR_NPART, i);
	if (init)
	{
		my_node->accessor = sts_initialize(addr,
										   APR_NPART,
										   APR_BACKEND_PART,
										   0,
										   SHARED_TUPLESTORE_SINGLE_PASS,
										   &sts->sfs.sfs,
										   DynamicReduceSFSFileName(name, PGXCNodeOid));
	}else
	{
		my_node->accessor = sts_attach_read_only(addr, &sts->sfs.sfs);
	}

	return my_node;
}

static void InitAdvanceParallelReduceWorker(ClusterReduceState *crstate, ParallelWorkerContext *pwcxt, char *addr)
{
	MemoryContext			oldcontext = MemoryContextSwitchTo(GetMemoryChunkContext(crstate));
	List				   *oid_list = castNode(ClusterReduce, crstate->ps.plan)->reduce_oids;
	AdvanceParallelState   *state;
	AdvanceParallelSharedMemory *shm;
	uint32					count;

	count = list_length(oid_list);
	state = palloc0(offsetof(AdvanceParallelState, nodes) + sizeof(state->nodes[0]) * count);
	crstate->private_state = state;
	crstate->reduce_method = RT_ADVANCE_PARALLEL;
	state->nnodes = count;
	state->normal.dsm_seg = dsm_attach(*(dsm_handle*)addr);
	state->shm = shm = dsm_segment_address(state->normal.dsm_seg);
	SharedFileSetAttach(&shm->sts.sfs.sfs, state->normal.dsm_seg);
	state->cur_node = BeginAdvanceSharedTuplestore(state->nodes, &shm->sts, oid_list, false);
	Assert(state->cur_node->nodeoid == PGXCNodeOid);
	sts_begin_parallel_scan(state->cur_node->accessor);

	ExecSetExecProcNode(&crstate->ps, ExecAdvanceParallelReduce);

	MemoryContextSwitchTo(oldcontext);
}

static void BeginAdvanceParallelReduce(ClusterReduceState *crstate)
{
	MemoryContext					oldcontext = MemoryContextSwitchTo(GetMemoryChunkContext(crstate));
	AdvanceParallelState		   *state;
	AdvanceParallelSharedMemory	   *shm;
	SharedTuplestoreAccessor	   *accessor;
	TupleTableSlot				   *slot;
	List						   *oid_list;
	uint32 							count;

	oid_list = castNode(ClusterReduce, crstate->ps.plan)->reduce_oids;
	count = list_length(oid_list);
	if (count == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Can not find working nodes")));
	}

	state = palloc0(offsetof(AdvanceParallelState, nodes) + sizeof(state->nodes[0]) * count);
	crstate->private_state = state;
	crstate->reduce_method = RT_ADVANCE_PARALLEL;
	state->nnodes = count;
	state->normal.dsm_seg = dsm_create(offsetof(AdvanceParallelSharedMemory, sts) + DRSTSD_SIZE(APR_NPART, count), 0);
	state->shm = shm = dsm_segment_address(state->normal.dsm_seg);
	SetupNormalReduceState(&state->normal, &shm->sts.sfs.mq, crstate, true);
	SharedFileSetInit(&shm->sts.sfs.sfs, state->normal.dsm_seg);
	state->cur_node = BeginAdvanceSharedTuplestore(state->nodes, &shm->sts, oid_list, true);
	Assert(state->cur_node->nodeoid == PGXCNodeOid);
	pg_atomic_init_flag(&shm->got_remote);
	ConditionVariableInit(&shm->cv);

	DynamicReduceStartSharedTuplestorePlan(crstate->ps.plan->plan_node_id,
										   state->normal.dsm_seg,
										   &shm->sts,
										   crstate->ps.ps_ResultTupleSlot->tts_tupleDescriptor,
										   oid_list,
										   APR_NPART,
										   APR_REDUCE_PART);

	accessor = state->cur_node->accessor;
	while (state->normal.drio.eof_local == false)
	{
		slot = DynamicReduceFetchLocal(&state->normal.drio);
		if (state->normal.drio.send_buf.len > 0)
		{
			DynamicReduceSendMessage(state->normal.drio.mqh_sender,
									 state->normal.drio.send_buf.len,
									 state->normal.drio.send_buf.data,
									 false);
			state->normal.drio.send_buf.len = 0;
		}
		if (!TupIsNull(slot))
			sts_puttuple(accessor, NULL, ExecFetchSlotMinimalTuple(slot));
	}
	sts_end_write(accessor);
	sts_begin_parallel_scan(accessor);

	ExecSetExecProcNode(&crstate->ps, ExecAdvanceParallelReduce);

	MemoryContextSwitchTo(oldcontext);
}

static void EndAdvanceParallelReduce(AdvanceParallelState *state)
{
	dsm_detach(state->normal.dsm_seg);
}

/* ========================= merge reduce =========================== */
static inline TupleTableSlot* GetMergeReduceResult(MergeReduceState *merge, ClusterReduceState *node)
{
	if (binaryheap_empty(merge->binheap))
		return ExecClearTuple(node->ps.ps_ResultTupleSlot);

	return merge->nodes[DatumGetUInt32(binaryheap_first(merge->binheap))].slot;
}

static TupleTableSlot* ExecMergeReduce(PlanState *pstate)
{
	ClusterReduceState *node = castNode(ClusterReduceState, pstate);
	MergeReduceState   *merge = node->private_state;
	MergeNodeInfo	   *info;
	uint32				i;

	i = DatumGetUInt32(binaryheap_first(merge->binheap));
	info = &merge->nodes[i];
	DynamicReduceReadSFSTuple(info->slot, info->file, &info->read_buf);
	if (TupIsNull(info->slot))
		binaryheap_remove_first(merge->binheap);
	else
		binaryheap_replace_first(merge->binheap, UInt32GetDatum(i));

	return GetMergeReduceResult(merge, node);
}

static BufFile* GetMergeBufFile(MergeReduceState *merge, Oid nodeoid)
{
	uint32	i,count;

	for (i=0,count=merge->nnodes;i<count;++i)
	{
		if (merge->nodes[i].nodeoid == nodeoid)
			return merge->nodes[i].file;
	}

	return NULL;
}

static void OpenMergeBufFiles(MergeReduceState *merge)
{
	MemoryContext		oldcontext;
	MergeNodeInfo	   *info;
	DynamicReduceSFS	sfs;
	uint32				i;
	char				name[MAXPGPATH];

	oldcontext = MemoryContextSwitchTo(GetMemoryChunkContext(merge));
	sfs = dsm_segment_address(merge->normal.dsm_seg);
	for(i=0;i<merge->nnodes;++i)
	{
		info = &merge->nodes[i];
		if (info->file == NULL)
		{
			info->file = BufFileOpenShared(&sfs->sfs,
										   DynamicReduceSFSFileName(name, info->nodeoid));
		}
		if (BufFileSeek(info->file, 0, 0, SEEK_SET) != 0)
		{
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("can not seek SFS file to head")));
		}
	}
	MemoryContextSwitchTo(oldcontext);
}

static void BuildMergeBinaryHeap(MergeReduceState *merge)
{
	MergeNodeInfo  *info;
	uint32			i,count;

	for(i=0,count=merge->nnodes;i<count;++i)
	{
		info = &merge->nodes[i];
		DynamicReduceReadSFSTuple(info->slot, info->file, &info->read_buf);
		if (!TupIsNull(info->slot))
			binaryheap_add_unordered(merge->binheap, UInt32GetDatum(i));
	}
	binaryheap_build(merge->binheap);
}

static void ExecMergeReduceLocal(ClusterReduceState *node)
{
	MergeReduceState   *merge = node->private_state;
	BufFile			   *file;
	TupleTableSlot	   *slot;

	/* find local MergeNodeInfo */
	file = GetMergeBufFile(merge, PGXCNodeOid);
	Assert(file != NULL);

	while(merge->normal.drio.eof_local == false)
	{
		slot = DynamicReduceFetchLocal(&merge->normal.drio);
		if (merge->normal.drio.send_buf.len > 0)
		{
			DynamicReduceSendMessage(merge->normal.drio.mqh_sender,
									 merge->normal.drio.send_buf.len,
									 merge->normal.drio.send_buf.data,
									 false);
			merge->normal.drio.send_buf.len = 0;
		}
		if (!TupIsNull(slot))
			DynamicReduceWriteSFSTuple(slot, file);
	}
}

static TupleTableSlot* ExecMergeReduceFinal(PlanState *pstate)
{
	ClusterReduceState *node = castNode(ClusterReduceState, pstate);
	MergeReduceState   *merge = node->private_state;
	uint8				flag PG_USED_FOR_ASSERTS_ONLY;

	/* wait dynamic reduce end of plan */
	flag = DynamicReduceRecvTuple(merge->normal.drio.mqh_receiver,
								  node->ps.ps_ResultTupleSlot,
								  &merge->normal.drio.recv_buf,
								  NULL,
								  false);
	Assert(flag = DR_MSG_RECV && TupIsNull(node->ps.ps_ResultTupleSlot));
	merge->normal.drio.eof_remote = true;

	ExecSetExecProcNode(&node->ps, ExecMergeReduce);

	OpenMergeBufFiles(merge);
	BuildMergeBinaryHeap(merge);
	return GetMergeReduceResult(merge, node);
}

static TupleTableSlot* ExecMergeReduceFirst(PlanState *pstate)
{
	ExecMergeReduceLocal(castNode(ClusterReduceState, pstate));

	return ExecMergeReduceFinal(pstate);
}

static void InitMergeReduceState(ClusterReduceState *state, MergeReduceState *merge)
{
	TupleDesc		desc = state->ps.ps_ResultTupleSlot->tts_tupleDescriptor;
	ClusterReduce  *plan = castNode(ClusterReduce, state->ps.plan);
	DynamicReduceSFS sfs;
	List		   *reduce_oids;
	ListCell	   *lc;
	uint32			i,count;
	Assert(plan->numCols > 0);

	reduce_oids = castNode(ClusterReduce, state->ps.plan)->reduce_oids;
	count = list_length(reduce_oids);
	if (count == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("Can not find working nodes")));
	}
	InitNormalReduceState(&merge->normal, sizeof(*sfs), state);
	sfs = dsm_segment_address(merge->normal.dsm_seg);
	SharedFileSetInit(&sfs->sfs, merge->normal.dsm_seg);

	merge->nodes = palloc0(sizeof(merge->nodes[0]) * count);
	merge->nnodes = count;
	lc = list_head(reduce_oids);
	for (i=0;i<count;++i)
	{
		MergeNodeInfo *info = &merge->nodes[i];
		info->slot = ExecInitExtraTupleSlot(state->ps.state, desc);
		initStringInfo(&info->read_buf);
		info->nodeoid = lfirst_oid(lc);
		lc = lnext(lc);
		if (info->nodeoid == PGXCNodeOid)
		{
			char name[MAXPGPATH];
			info->file = BufFileCreateShared(&sfs->sfs,
											 DynamicReduceSFSFileName(name, info->nodeoid));
		}
	}

	merge->binheap = binaryheap_allocate(count, cmr_heap_compare_slots, merge);
	merge->sortkeys = palloc0(sizeof(merge->sortkeys[0]) * plan->numCols);
	merge->nkeys = plan->numCols;
	for (i=0;i<merge->nkeys;++i)
	{
		SortSupport sort = &merge->sortkeys[i];
		sort->ssup_cxt = CurrentMemoryContext;
		sort->ssup_collation = plan->collations[i];
		sort->ssup_nulls_first = plan->nullsFirst[i];
		sort->ssup_attno = plan->sortColIdx[i];

		sort->abbreviate = false;

		PrepareSortSupportFromOrderingOp(plan->sortOperators[i], sort);
	}
}
static void InitMergeReduce(ClusterReduceState *crstate)
{
	MemoryContext		oldcontext;
	MergeReduceState   *merge;
	Assert(crstate->private_state == NULL);

	oldcontext = MemoryContextSwitchTo(GetMemoryChunkContext(crstate));
	merge = palloc0(sizeof(MergeReduceState));
	InitMergeReduceState(crstate, merge);
	crstate->private_state = merge;
	ExecSetExecProcNode(&crstate->ps, ExecMergeReduceFirst);

	DynamicReduceStartSharedFileSetPlan(crstate->ps.plan->plan_node_id,
										merge->normal.dsm_seg,
										dsm_segment_address(merge->normal.dsm_seg),
										crstate->ps.ps_ResultTupleSlot->tts_tupleDescriptor,
										castNode(ClusterReduce, crstate->ps.plan)->reduce_oids);

	MemoryContextSwitchTo(oldcontext);
}
static void EndMergeReduce(MergeReduceState *merge)
{
	uint32				i,count;
	MergeNodeInfo	   *info;
	DynamicReduceSFS	sfs = dsm_segment_address(merge->normal.dsm_seg);
	char				name[MAXPGPATH];

	for (i=0,count=merge->nnodes;i<count;++i)
	{
		info = &merge->nodes[i];
		if(info->file)
			BufFileClose(info->file);
		BufFileDeleteShared(&sfs->sfs, DynamicReduceSFSFileName(name, info->nodeoid));
	}
	EndNormalReduce(&merge->normal);
	pfree(merge->sortkeys);
}
#define DriveMergeReduce(node) DriveNormalReduce(node)

static void BeginAdvanceMerge(ClusterReduceState *crstate)
{
	MemoryContext		oldcontext = MemoryContextSwitchTo(GetMemoryChunkContext(crstate));

	InitMergeReduce(crstate);
	MemoryContextSwitchTo(crstate->ps.state->es_query_cxt);
	ExecMergeReduceLocal(crstate);
	ExecSetExecProcNode(&crstate->ps, ExecMergeReduceFinal);

	MemoryContextSwitchTo(oldcontext);
}

/* ======================================================== */
static void InitReduceMethod(ClusterReduceState *crstate)
{
	Assert(crstate->private_state == NULL);
	switch(crstate->reduce_method)
	{
	case RT_NOTHING:
		ExecSetExecProcNode(&crstate->ps, ExecNothingReduce);
		break;
	case RT_NORMAL:
		InitNormalReduce(crstate);
		break;
	case RT_MERGE:
		InitMergeReduce(crstate);
		break;
	default:
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("unknown reduce method %u", crstate->reduce_method)));
		break;
	}
	Assert(crstate->private_state != NULL ||
		   crstate->reduce_method == RT_NOTHING);
}
static TupleTableSlot* ExecDefaultClusterReduce(PlanState *pstate)
{
	ClusterReduceState *crstate = castNode(ClusterReduceState, pstate);
	if (crstate->private_state != NULL)
		return pstate->ExecProcNodeReal(pstate);

	InitReduceMethod(crstate);

	return pstate->ExecProcNodeReal(pstate);
}

void ExecClusterReduceEstimate(ClusterReduceState *node, ParallelContext *pcxt)
{
	switch(node->reduce_method)
	{
	case RT_NOTHING:
		break;
	case RT_NORMAL:
		EstimateNormalReduce(pcxt);
		break;
	case RT_ADVANCE:
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("advance reduce not support parallel yet")));
		break;
	case RT_ADVANCE_PARALLEL:
		shm_toc_estimate_chunk(&pcxt->estimator, sizeof(Size)+sizeof(dsm_handle));
		shm_toc_estimate_keys(&pcxt->estimator, 1);
		break;
	case RT_MERGE:
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("merge reduce not support parallel yet")));
		break;
	default:
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("unknown reduce method %u", node->reduce_method)));
		break;
	}
}
void ExecClusterReduceInitializeDSM(ClusterReduceState *node, ParallelContext *pcxt)
{
	switch(node->reduce_method)
	{
	case RT_NOTHING:
		break;
	case RT_NORMAL:
		InitParallelReduce(node, pcxt);
		break;
	case RT_ADVANCE_PARALLEL:
		{
			char *addr = shm_toc_allocate(pcxt->toc, sizeof(dsm_handle));
			dsm_handle *h = (dsm_handle*)(addr + sizeof(Size));
			AdvanceParallelState *state = node->private_state;
			*(Size*)addr = RT_ADVANCE_PARALLEL;
			*h = dsm_segment_handle(state->normal.dsm_seg);
			shm_toc_insert(pcxt->toc, node->ps.plan->plan_node_id, addr);
		}
		break;
	case RT_MERGE:
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("merge reduce not support parallel yet")));
		break;
	default:
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("unknown reduce method %u", node->reduce_method)));
		break;
	}
}
void ExecClusterReduceReInitializeDSM(ClusterReduceState *node, ParallelContext *pcxt)
{
	ereport(ERROR,
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			 errmsg("parallel reduce not support reinitialize dsm")));
}
void ExecClusterReduceInitializeWorker(ClusterReduceState *node, ParallelWorkerContext *pwcxt)
{
	char *addr = shm_toc_lookup(pwcxt->toc, node->ps.plan->plan_node_id, false);
	node->reduce_method = (uint8)(*(Size*)addr);
	addr += sizeof(Size);

	DynamicReduceStartParallel();
	switch(node->reduce_method)
	{
	case RT_NOTHING:
		break;
	case RT_NORMAL:
		InitParallelReduceWorker(node, pwcxt, addr);
		break;
	case RT_ADVANCE_PARALLEL:
		InitAdvanceParallelReduceWorker(node, pwcxt, addr);
		break;
	case RT_MERGE:
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("merge reduce not support parallel yet")));
		break;
	default:
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("unknown reduce method %u", node->reduce_method)));
		break;
	}
}

void ExecClusterReduceStartedParallel(ClusterReduceState *node, ParallelContext *pcxt)
{
	switch(node->reduce_method)
	{
	case RT_NOTHING:
	case RT_ADVANCE_PARALLEL:
		break;
	case RT_NORMAL:
		StartParallelReduce(node, pcxt);
		break;
	case RT_MERGE:
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("merge reduce not support parallel yet")));
		break;
	default:
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("unknown reduce method %u", node->reduce_method)));
		break;
	}
}

ClusterReduceState *
ExecInitClusterReduce(ClusterReduce *node, EState *estate, int eflags)
{
	ClusterReduceState *crstate;
	Plan			   *outerPlan;
	//TupleDesc			tupDesc;

	Assert(outerPlan(node) != NULL);
	Assert(innerPlan(node) == NULL);

	/*
	 * create state structure
	 */
	crstate = makeNode(ClusterReduceState);
	crstate->ps.plan = (Plan*)node;
	crstate->ps.state = estate;
	crstate->ps.ExecProcNode = ExecDefaultClusterReduce;
	if (list_member_oid(node->reduce_oids, PGXCNodeOid) == false)
		crstate->reduce_method = (uint8)RT_NOTHING;
	else if (node->numCols > 0)
		crstate->reduce_method = (uint8)RT_MERGE;
	else
		crstate->reduce_method = (uint8)RT_NORMAL;

	/*
	 * We must have a tuplestore buffering the subplan output to do backward
	 * scan or mark/restore.  We also prefer to materialize the subplan output
	 * if we might be called on to rewind and replay it many times. However,
	 * if none of these cases apply, we can skip storing the data.
	 */
	crstate->eflags = (eflags & (EXEC_FLAG_REWIND |
								 EXEC_FLAG_BACKWARD |
								 EXEC_FLAG_MARK));

	/*
	 * Tuplestore's interpretation of the flag bits is subtly different from
	 * the general executor meaning: it doesn't think BACKWARD necessarily
	 * means "backwards all the way to start".  If told to support BACKWARD we
	 * must include REWIND in the tuplestore eflags, else tuplestore_trim
	 * might throw away too much.
	 */
	if (eflags & EXEC_FLAG_BACKWARD)
		crstate->eflags |= EXEC_FLAG_REWIND;

	/*
	 * Miscellaneous initialization
	 *
	 * create expression context for node
	 */
	ExecAssignExprContext(estate, &crstate->ps);

	Assert(OidIsValid(PGXCNodeOid));

	/*
	 * Initialize result slot, type and projection.
	 */
	ExecInitResultTupleSlotTL(estate, &crstate->ps);

	/*
	 * initialize child nodes
	 *
	 * We shield the child node from the need to support REWIND, BACKWARD, or
	 * MARK/RESTORE.
	 */
	eflags &= ~(EXEC_FLAG_REWIND | EXEC_FLAG_BACKWARD | EXEC_FLAG_MARK);

	outerPlan = outerPlan(node);
	outerPlanState(crstate) = ExecInitNode(outerPlan, estate, eflags);
	//tupDesc = ExecGetResultType(outerPlanState(crstate));

	estate->es_reduce_plan_inited = true;

	return crstate;
}

/*
 * Compare the tuples in the two given slots.
 */
static int
cmr_heap_compare_slots(Datum a, Datum b, void *arg)
{
	MergeReduceState   *merge = (MergeReduceState*)arg;
	TupleTableSlot	   *s1 = merge->nodes[DatumGetUInt32(a)].slot;
	TupleTableSlot	   *s2 = merge->nodes[DatumGetUInt32(b)].slot;
	uint32				nkeys = merge->nkeys;
	uint32				nkey;

	Assert(!TupIsNull(s1));
	Assert(!TupIsNull(s2));

	for (nkey = 0; nkey < nkeys; nkey++)
	{
		SortSupport sortKey = &merge->sortkeys[nkey];
		AttrNumber	attno = sortKey->ssup_attno;
		Datum		datum1,
					datum2;
		bool		isNull1,
					isNull2;
		int			compare;

		datum1 = slot_getattr(s1, attno, &isNull1);
		datum2 = slot_getattr(s2, attno, &isNull2);

		compare = ApplySortComparator(datum1, isNull1,
									  datum2, isNull2,
									  sortKey);
		if (compare != 0)
			return -compare;
	}

	return 0;
}

void ExecShutdownClusterReduce(ClusterReduceState *node)
{
	if (node->private_state)
	{
		switch(node->reduce_method)
		{
		case RT_NORMAL:
			EndNormalReduce(node->private_state);
			break;
		case RT_ADVANCE:
			EndAdvanceReduce(node->private_state, node);
			break;
		case RT_ADVANCE_PARALLEL:
			EndAdvanceParallelReduce(node->private_state);
			break;
		case RT_MERGE:
			EndMergeReduce(node->private_state);
			break;
		default:
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("unknown reduce method %u", node->reduce_method)));
			break;
		}
		pfree(node->private_state);
		node->private_state = NULL;
	}
}

void
ExecEndClusterReduce(ClusterReduceState *node)
{
	if ((node->eflags & EXEC_FLAG_EXPLAIN_ONLY) != 0)
		DriveClusterReduceState(node);

	ExecShutdownClusterReduce(node);

	ExecEndNode(outerPlanState(node));
}

/* ----------------------------------------------------------------
 *		ExecClusterReduceMarkPos
 *
 *		Calls tuplestore to save the current position in the stored file.
 * ----------------------------------------------------------------
 */
void
ExecClusterReduceMarkPos(ClusterReduceState *node)
{
	ereport(ERROR,
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			 errmsg("cluster reduce not support mark pos")));
}

/* ----------------------------------------------------------------
 *		ExeClusterReduceRestrPos
 *
 *		Calls tuplestore to restore the last saved file position.
 * ----------------------------------------------------------------
 */
void
ExecClusterReduceRestrPos(ClusterReduceState *node)
{
	ereport(ERROR,
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			 errmsg("cluster reduce not support restr pos")));
}

void
ExecReScanClusterReduce(ClusterReduceState *node)
{
	/* Just return if not start yet! */
	if (node->private_state == NULL)
		return;

	if (outerPlanState(node)->chgParam != NULL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cluster reduce not support sub plan change param")));
	}

	switch(node->reduce_method)
	{
	case RT_NOTHING:
		break;
	case RT_NORMAL:
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("normal cluster reduce not support rescan")));
		break;
	case RT_ADVANCE:
		{
			AdvanceReduceState *state = node->private_state;
			AdvanceNodeInfo	   *info;
			uint32				i;

			state->cur_node = NULL;
			for (i=0;i<state->nnodes;++i)
			{
				info = &state->nodes[i];
				if (info->file != NULL &&
					BufFileSeek(info->file, 0, 0, SEEK_SET) != 0)
				{
					ereport(ERROR,
							(errcode_for_file_access(),
							 errmsg("can not seek buffer file to head")));
				}
				if (info->nodeoid == PGXCNodeOid)
					state->cur_node = info;
			}
			Assert(state->cur_node != NULL);
			Assert(state->cur_node->nodeoid == PGXCNodeOid);
		}
		break;
	case RT_ADVANCE_PARALLEL:
		{
			AdvanceParallelState   *state = node->private_state;
			AdvanceParallelNode	   *node;
			uint32					i;

			state->cur_node = NULL;
			sts_end_parallel_scan(state->cur_node->accessor);
			for (i=0;i<state->nnodes;++i)
			{
				node = &state->nodes[i];
				if (node->nodeoid == PGXCNodeOid)
				{
					state->cur_node = node;
					break;
				}
			}
			Assert(state->cur_node != NULL);
			Assert(state->cur_node->nodeoid == PGXCNodeOid);
			sts_begin_parallel_scan(state->cur_node->accessor);
		}
		break;
	case RT_MERGE:
		{
			MergeReduceState   *state = node->private_state;
			MergeNodeInfo	   *node;
			uint32				i;
			if (state->normal.drio.eof_remote == true)
			{
				binaryheap_reset(state->binheap);
				for (i=0;i<state->nnodes;++i)
				{
					node = &state->nodes[i];
					ExecClearTuple(node->slot);
					resetStringInfo(&node->read_buf);
					if (BufFileSeek(node->file, 0, 0, SEEK_SET) != 0)
					{
						ereport(ERROR,
								(errcode_for_file_access(),
								 errmsg("can not seek SFS file to head")));
					}
				}
				BuildMergeBinaryHeap(state);
			}
		}
		break;
	default:
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("unknown cluster reduce method %u", node->reduce_method)));
		break;
	}
}

static bool
DriveClusterReduceState(ClusterReduceState *node)
{
	if (node->private_state == NULL)
		InitReduceMethod(node);

	switch(node->reduce_method)
	{
	case RT_NOTHING:
	case RT_ADVANCE:
	case RT_ADVANCE_PARALLEL:
		break;
	case RT_NORMAL:
		DriveNormalReduce(node);
		break;
	case RT_MERGE:
		DriveMergeReduce(node);
		break;
	default:
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("unknown reduce method %u", node->reduce_method)));
		break;
	}

	return false;
}

static bool
DriveCteScanState(PlanState *node)
{
	TupleTableSlot *slot = NULL;
	ListCell	   *lc = NULL;
	SubPlanState   *sps = NULL;

	Assert(node && IsA(node, CteScanState));

	if (!IsThereClusterReduce(node))
		return false;

	/*
	 * Here we do ExecCteScan instead of just driving ClusterReduce,
	 * because other plan node may need the results of the CteScan.
	 */
	for (;;)
	{
		slot = node->ExecProcNode(node);
		if (TupIsNull(slot))
			break;
	}

	/*
	 * Do not forget to drive subPlan-s.
	 */
	foreach (lc, node->subPlan)
	{
		sps = (SubPlanState *) lfirst(lc);

		Assert(IsA(sps, SubPlanState));
		if (DriveClusterReduceWalker(sps->planstate))
			return true;
	}

	/*
	 * Do not forget to drive initPlan-s.
	 */
	foreach (lc, node->initPlan)
	{
		sps = (SubPlanState *) lfirst(lc);

		Assert(IsA(sps, SubPlanState));
		if (DriveClusterReduceWalker(sps->planstate))
			return true;
	}

	return false;
}

static bool
DriveMaterialState(PlanState *node)
{
	TupleTableSlot *slot = NULL;

	Assert(node && IsA(node, MaterialState));

	if (!IsThereClusterReduce(node))
		return false;

	/*
	 * Here we do ExecMaterial instead of just driving ClusterReduce,
	 * because other plan node may need the results of the Material.
	 */
	for (;;)
	{
		slot = node->ExecProcNode(node);
		if (TupIsNull(slot))
			break;
	}

	return false;
}

static bool
DriveClusterReduceWalker(PlanState *node)
{
	EState	   *estate;
	int			planid;
	bool		res;

	if (node == NULL)
		return false;

	estate = node->state;
	if (list_member_ptr(estate->es_auxmodifytables, node))
	{
		ModifyTableState *mtstate = (ModifyTableState *) node;

		/*
		 * It's safe to drive ClusterReduce if the secondary
		 * ModifyTableState is done(mt_done is true). otherwise
		 * the secondary ModifyTableState will be done by
		 * ExecPostprocessPlan later and it is not correct to
		 * drive here.
		 */
		if (!mtstate->mt_done)
			return false;
	}

	/* do not drive twice */
	planid = PlanNodeID(node->plan);
	if (bms_is_member(planid, estate->es_reduce_drived_set))
		return false;

	if (IsA(node, ClusterReduceState))
	{
		/*
		 * Drive all ClusterReduce to send slot, discard slot
		 * used for local.
		 */
		if (((ClusterReduceState*)node)->reduce_method == RT_NOTHING)
			res = DriveClusterReduceWalker(outerPlanState(node));
		else
			res = DriveClusterReduceState((ClusterReduceState *) node);
	} else
	if (IsA(node, CteScanState))
	{
		res = DriveCteScanState(node);
	} else
	if (IsA(node, MaterialState))
	{
		res = DriveMaterialState(node);
	} else
	{
		res = planstate_tree_exec_walker(node, DriveClusterReduceWalker, NULL);
	}

	estate->es_reduce_drived_set = bms_add_member(estate->es_reduce_drived_set, planid);

	return res;
}

static bool
IsThereClusterReduce(PlanState *node)
{
	if (node == NULL)
		return false;

	if (IsA(node, ClusterReduceState))
		return true;

	if (IsA(node, CteScanState) &&
		IsThereClusterReduce(((CteScanState *) node)->cteplanstate))
		return true;

	return planstate_tree_walker(node, IsThereClusterReduce, NULL);
}

void
TopDownDriveClusterReduce(PlanState *node)
{
	if (!enable_cluster_plan || !IsUnderPostmaster)
		return ;

	/* just return if there is no ClusterReduce plan */
	if (!node->state->es_reduce_plan_inited)
		return ;

	(void) DriveClusterReduceWalker(node);
}

/* =========================================================================== */
#define ACR_FLAG_INVALID	0x0
#define ACR_FLAG_OUTER		0x1
#define ACR_FLAG_INNER		0x2
#define ACR_FLAG_APPEND		0x5
#define ACR_FLAG_SUBQUERY	0x6
#define ACR_MARK_SPECIAL	0xFFFF0000
#define ACR_FLAG_SUBPLAN	0x10000
#define ACR_FLAG_INITPLAN	0x20000
#define ACR_FLAG_PAPPEND	0x40000


static inline void AdvanceReduce(ClusterReduceState *crs, PlanState *parent, uint32 flags)
{
	ClusterReduce *plan = castNode(ClusterReduce, crs->ps.plan);
	bool need_advance = false;

	if (flags & ACR_MARK_SPECIAL)
		need_advance = true;

	if (need_advance == false)
		return;
	
	switch(crs->reduce_method)
	{
	case RT_NOTHING:
		return;
	case RT_NORMAL:
		if (plan->plan.parallel_safe)
			BeginAdvanceParallelReduce(crs);
		else
			BeginAdvanceReduce(crs);
		break;
	case RT_MERGE:
		BeginAdvanceMerge(crs);
		break;
	default:
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("unknown reduce method %u", crs->reduce_method)));
		break;
	}
}

#define WalkerList(list, type_)						\
	if ((list) != NIL)								\
	{												\
		ListCell *lc;								\
		foreach(lc, (list))							\
			AdvanceClusterReduceWorker(lfirst_node(SubPlanState, lc)->planstate, ps, type_); \
	}while(false)

#define WalkerMembers(State, arr, count, type_)		\
	do{												\
		uint32 i,n=(((State*)ps)->count);			\
		PlanState **subs = (((State*)ps)->arr);		\
		for(i=0;i<n;++i)							\
			AdvanceClusterReduceWorker(subs[i], ps, type_);	\
	}while(false)

static void AdvanceClusterReduceWorker(PlanState *ps, PlanState *pps, uint32 flags)
{
	if (ps == NULL)
		return;

	check_stack_depth();

	/* initPlan-s */
	WalkerList(ps->initPlan, ACR_FLAG_INITPLAN);
	if (IsA(ps, ReduceScanState))
	{
		FetchReduceScanOuter((ReduceScanState*)ps);
		WalkerList(ps->subPlan, ACR_FLAG_SUBPLAN);
		return;
	}

	/* outer */
	AdvanceClusterReduceWorker(outerPlanState(ps), ps, 
							   (flags&ACR_MARK_SPECIAL)|ACR_FLAG_OUTER);

	/* inner */
	AdvanceClusterReduceWorker(innerPlanState(ps), ps,
							   (flags&ACR_MARK_SPECIAL)|ACR_FLAG_INNER);

	switch(nodeTag(ps))
	{
	case T_ClusterReduceState:
		Assert(flags != ACR_FLAG_INVALID);
		AdvanceReduce((ClusterReduceState*)ps, pps, flags);
		break;
	case T_ReduceScanState:
		break;
	case T_ModifyTableState:
		WalkerMembers(ModifyTableState, mt_plans, mt_nplans,
					  (flags&ACR_MARK_SPECIAL)|ACR_FLAG_APPEND);
		break;
	case T_AppendState:
		{
			uint32 new_flags = (flags&ACR_MARK_SPECIAL)|ACR_FLAG_APPEND;
			if (ps->plan->parallel_safe)
				new_flags |= ACR_FLAG_PAPPEND;
			WalkerMembers(AppendState, appendplans, as_nplans, new_flags);
		}
		break;
	case T_MergeAppendState:
		WalkerMembers(MergeAppendState, mergeplans, ms_nplans,
					  (flags&ACR_MARK_SPECIAL)|ACR_FLAG_APPEND);
		break;
	case T_BitmapAndState:
		WalkerMembers(BitmapAndState, bitmapplans, nplans,
					  (flags&ACR_MARK_SPECIAL)|ACR_FLAG_APPEND);
		break;
	case T_BitmapOrState:
		WalkerMembers(BitmapOrState, bitmapplans, nplans,
					  (flags&ACR_MARK_SPECIAL)|ACR_FLAG_APPEND);
		break;
	case T_SubqueryScanState:
		AdvanceClusterReduceWorker(((SubqueryScanState*)ps)->subplan, ps,
								   (flags&ACR_MARK_SPECIAL)|ACR_FLAG_SUBQUERY);
		break;
	case T_CustomScanState:
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cluster plan not support custom yet")));
		break;
	default:
		break;
	}

	WalkerList(ps->subPlan, ACR_FLAG_SUBPLAN);
}

void AdvanceClusterReduce(PlanState *pstate)
{
	if (IsParallelWorker())
		return;

	AdvanceClusterReduceWorker(pstate, NULL, ACR_FLAG_INVALID);
}

TupleTableSlot* ExecFakeProcNode(PlanState *pstate)
{
	Plan *plan = pstate->plan;
	const char *node_str = get_enum_string_NodeTag(nodeTag(plan));
	if (node_str == NULL)
	{
		node_str = "unknown";
	}else if(node_str[0] == 'T' &&
			 node_str[1] == '_')
	{
		node_str += 2;
	}

	ereport(ERROR,
			(errcode(ERRCODE_INTERNAL_ERROR),
			 errmsg("plan %s id %d should not be execute", node_str, plan->plan_node_id)));

	return NULL;	/* keep compiler quiet */
}