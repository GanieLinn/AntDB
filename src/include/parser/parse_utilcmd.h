/*-------------------------------------------------------------------------
 *
 * parse_utilcmd.h
 *		parse analysis for utility commands
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/parser/parse_utilcmd.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_UTILCMD_H
#define PARSE_UTILCMD_H

#include "parser/parse_node.h"


extern List *transformCreateStmt(CreateStmt *stmt, const char *queryString ADB_ONLY_COMMA_ARG(Node **transform_stmt));
extern List *transformAlterTableStmt(Oid relid, AlterTableStmt *stmt,
									 const char *queryString);
extern IndexStmt *transformIndexStmt(Oid relid, IndexStmt *stmt,
									 const char *queryString);
extern void transformRuleStmt(RuleStmt *stmt, const char *queryString,
							  List **actions, Node **whereClause);
extern List *transformCreateSchemaStmt(CreateSchemaStmt *stmt);
extern PartitionBoundSpec *transformPartitionBound(ParseState *pstate, Relation parent,
												   PartitionBoundSpec *spec);
extern IndexStmt *generateClonedIndexStmt(RangeVar *heapRel,
										  Relation source_idx,
										  const AttrNumber *attmap, int attmap_length,
										  Oid *constraintOid);
#ifdef ADB
extern int transformDistributeCluster(ParseState *pstate, Relation rel, PartitionKey key,
									  PartitionSpec *spec, PGXCSubCluster *cluster, char loc_type,
									  List **values, Oid **nodeoids);
#endif /* ADB */

#endif							/* PARSE_UTILCMD_H */
