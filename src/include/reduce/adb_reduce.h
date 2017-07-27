/*-------------------------------------------------------------------------
 *
 * adb_reduce.h
 *	  header file for integrated adb reduce daemon for backend called.
 *
 * Portions Copyright (c) 2010-2017 ADB Development Group
 *
 * IDENTIFICATION
 * 			src/include/reduce/adb_reduce.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef ADB_REDUCE_H
#define ADB_REDUCE_H

#include "executor/tuptable.h"
#include "reduce/rdc_comm.h"
#include "reduce/rdc_msg.h"

extern void EndSelfReduce(int code, Datum arg);

extern RdcPort *ConnectSelfReduce(RdcPortType self_type, RdcPortId self_id);

extern int StartSelfReduceLauncher(RdcPortId rid);

extern void StartSelfReduceGroup(RdcMask *rdc_masks, int num);

extern void EndSelfReduceGroup(void);

extern void SendPlanCloseToSelfReduce(RdcPort *port, bool broadcast);

extern void SendEofToRemote(RdcPort *port);

extern void SendSlotToRemote(RdcPort *port, List *destNodes, TupleTableSlot *slot);

extern TupleTableSlot* GetSlotFromRemote(RdcPort *port, TupleTableSlot *slot, bool *eof, List **closed_remote);

#endif /* ADB_BROKER_H */