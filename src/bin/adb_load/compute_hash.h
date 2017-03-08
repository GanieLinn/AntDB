#ifndef COMPUTE_HASH_H
#define COMPUTE_HASH_H

#include "libpq-fe.h"

#include "msg_queue.h"
#include "msg_queue_pipe.h"

typedef pthread_t	HashThreadID;

typedef struct HashField
{
	int  *field_loc;     /* hash fields locations , begin at 1 */
	Oid  *field_type;    /* hash fields type */	
	int   field_nums;
	char *func_name;
	int   node_nums;
	Oid  *node_list;
	char *delim;        /* separator */
	char *copy_options; /* copy options */
	char quotec;        /* csv mode use */
	bool has_qoute;
	char escapec;
	bool has_escape;
	char *hash_delim;
} HashField;

typedef struct HashComputeInfo
{
	bool				 is_need_create;
	int					 thread_nums;
	int					 output_queue_size;
	char				*func_name;
	char				*conninfo;
	MessageQueuePipe	*input_queue;	
	MessageQueuePipe   **output_queue;
	HashField			*hash_field;
} HashComputeInfo;

typedef enum ThreadWorkState
{
	THREAD_DEFAULT,
	THREAD_MEMORY_ERROR,
	THREAD_CONNECTION_ERROR,
	THREAD_SEND_ERROR,
	THREAD_SELECT_ERROR,
	THREAD_COPY_STATE_ERROR,
	THREAD_COPY_END_ERROR,
	THREAD_GET_COPY_DATA_ERROR,
	THREAD_RESTART_COPY_STREAM,
	THREAD_FIELD_ERROR,
	THREAD_MESSAGE_CONFUSION_ERROR,
	THREAD_DEAL_COMPLETE,
	THREAD_EXIT_BY_OTHERS,
	THREAD_EXIT_NORMAL
} ThreadWorkState;

typedef struct ComputeThreadInfo
{
	HashThreadID 		thread_id;
	long				send_seq;
	int					output_queue_size;
	MessageQueuePipe  	*input_queue;
	MessageQueuePipe  	**output_queue;
	MessageQueue		*inner_queue;
	HashField	  		*hash_field;
	char		  		*func_name;
	char 		  		*conninfo;
	char				*copy_str;
	PGconn   	  		*conn;
	ThreadWorkState	state;
	bool				exit;
	void 				* (* thr_startroutine)(void *); /* thread start function */
} ComputeThreadInfo;

typedef struct HashThreads
{
	int 			   hs_thread_count;
	int				   hs_thread_cur;
	ComputeThreadInfo **hs_threads;
	pthread_mutex_t	   mutex;
} HashThreads;

#define	HASH_COMPUTE_ERROR		0
#define	HASH_COMPUTE_OK			1

int InitHashCompute(int thread_nums, char * func, char * conninfo, MessageQueuePipe * message_queue_in,
			MessageQueuePipe ** message_queue_out, int queue_size, HashField * field);

/**
* @brief CheckComputeState
* 
* return  running thread numbers
* @return int  running thread numbers
*/
int CheckComputeState(void);

/* caller don't need to free memory */
HashThreads *GetExitThreadsInfo(void);

int StopHash(void);

/* make sure all threads had exited */
void CleanHashResource(void);

void SetHashFileStartCmd(char * start_cmd);
#endif