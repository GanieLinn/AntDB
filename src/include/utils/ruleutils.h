/*-------------------------------------------------------------------------
 *
 * ruleutils.h
 *		Declarations for ruleutils.c
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/ruleutils.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef RULEUTILS_H
#define RULEUTILS_H

#include "nodes/nodes.h"
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"


extern char *pg_get_indexdef_string(Oid indexrelid);
extern char *pg_get_indexdef_columns(Oid indexrelid, bool pretty);

extern char *pg_get_partkeydef_columns(Oid relid, bool pretty);
extern char *pg_get_partconstrdef_string(Oid partitionId, char *aliasname);

extern char *pg_get_constraintdef_command(Oid constraintId);
extern char *deparse_expression(Node *expr, List *dpcontext,
								bool forceprefix, bool showimplicit);
extern List *deparse_context_for(const char *aliasname, Oid relid);
extern List *deparse_context_for_plan_rtable(List *rtable, List *rtable_names);
extern List *set_deparse_context_planstate(List *dpcontext,
										   Node *planstate, List *ancestors);
extern List *select_rtable_names_for_explain(List *rtable,
											 Bitmapset *rels_used);
extern char *generate_collation_name(Oid collid);
extern char *get_range_partbound_string(List *bound_datums);

#ifdef ADB
extern List *deparse_context_for_plan(Node *plan, List *ancestors,
									  List *rtable);

extern void deparse_query(Query *query, StringInfo buf, List *parentnamespace,
						  bool finalise_aggs, bool sortgroup_colno);
extern char* deparse_to_reduce_modulo(Relation rel, RelationLocInfo *loc);
#endif

#endif							/* RULEUTILS_H */
