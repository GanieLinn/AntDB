#include "postgres.h"

#include "parser/parse_type.h"
#include "parser/scanner.h"
#include "plora_kwlist.h"
#include "plpgsql.h"

#include "plora_gram.h"

#define PG_KEYWORD(a,b,c) {a,b,c}

const ScanKeyword plora_reserved_keywords[] = {
	PG_KEYWORD("all", POK_ALL, RESERVED_KEYWORD),
	PG_KEYWORD("alter", POK_ALTER, RESERVED_KEYWORD),
	PG_KEYWORD("and", POK_AND, RESERVED_KEYWORD),
	PG_KEYWORD("any", POK_ANY, RESERVED_KEYWORD),
	PG_KEYWORD("as", POK_AS, RESERVED_KEYWORD),
	PG_KEYWORD("asc", POK_ASC, RESERVED_KEYWORD),
	PG_KEYWORD("at", POK_AT, RESERVED_KEYWORD),

	PG_KEYWORD("begin", POK_BEGIN, RESERVED_KEYWORD),
	PG_KEYWORD("between", POK_BETWEEN, RESERVED_KEYWORD),
	PG_KEYWORD("by", POK_BY, RESERVED_KEYWORD),

	PG_KEYWORD("case", POK_CASE, RESERVED_KEYWORD),
	PG_KEYWORD("check", POK_CHECK, RESERVED_KEYWORD),
	PG_KEYWORD("cluster", POK_CLUSTER, RESERVED_KEYWORD),
	PG_KEYWORD("clusters", POK_CLUSTERS, RESERVED_KEYWORD),
	PG_KEYWORD("colauth", POK_COLAUTH, RESERVED_KEYWORD),
	PG_KEYWORD("columns", POK_COLUMNS, RESERVED_KEYWORD),
	PG_KEYWORD("compress", POK_COMPRESS, RESERVED_KEYWORD),
	PG_KEYWORD("connect", POK_CONNECT, RESERVED_KEYWORD),
	PG_KEYWORD("crash", POK_CRASH, RESERVED_KEYWORD),
	PG_KEYWORD("create", POK_CREATE, RESERVED_KEYWORD),
	PG_KEYWORD("cursor", POK_CURSOR, RESERVED_KEYWORD),

	PG_KEYWORD("declare", POK_DECLARE, RESERVED_KEYWORD),
	PG_KEYWORD("default", POK_DEFAULT, RESERVED_KEYWORD),
	PG_KEYWORD("desc", POK_DESC, RESERVED_KEYWORD),
	PG_KEYWORD("distinct", POK_DISTINCT, RESERVED_KEYWORD),
	PG_KEYWORD("drop", POK_DROP, RESERVED_KEYWORD),

	PG_KEYWORD("else", POK_ELSE, RESERVED_KEYWORD),
	PG_KEYWORD("end", POK_END, RESERVED_KEYWORD),
	PG_KEYWORD("exception", POK_EXCEPTION, RESERVED_KEYWORD),
	PG_KEYWORD("exclusive", POK_EXCLUSIVE, RESERVED_KEYWORD),

	PG_KEYWORD("fetch", POK_FETCH, RESERVED_KEYWORD),
	PG_KEYWORD("for", POK_FOR, RESERVED_KEYWORD),
	PG_KEYWORD("from", POK_FROM, RESERVED_KEYWORD),
	PG_KEYWORD("function", POK_FUNCTION, RESERVED_KEYWORD),

	PG_KEYWORD("goto", POK_GOTO, RESERVED_KEYWORD),
	PG_KEYWORD("grant", POK_GRANT, RESERVED_KEYWORD),
	PG_KEYWORD("group", POK_GROUP, RESERVED_KEYWORD),

	PG_KEYWORD("having", POK_HAVING, RESERVED_KEYWORD),

	PG_KEYWORD("identified", POK_IDENTIFIED, RESERVED_KEYWORD),
	PG_KEYWORD("if", POK_IF, RESERVED_KEYWORD),
	PG_KEYWORD("in", POK_IN, RESERVED_KEYWORD),
	PG_KEYWORD("index", POK_INDEX, RESERVED_KEYWORD),
	PG_KEYWORD("indexes", POK_INDEXES, RESERVED_KEYWORD),
	PG_KEYWORD("insert", POK_INSERT, RESERVED_KEYWORD),
	PG_KEYWORD("intersect", POK_INTERSECT, RESERVED_KEYWORD),
	PG_KEYWORD("into", POK_INTO, RESERVED_KEYWORD),
	PG_KEYWORD("is", POK_IS, RESERVED_KEYWORD),

	PG_KEYWORD("like", POK_LIKE, RESERVED_KEYWORD),
	PG_KEYWORD("lock", POK_LOCK, RESERVED_KEYWORD),

	PG_KEYWORD("minus", POK_MINUS, RESERVED_KEYWORD),
	PG_KEYWORD("mode", POK_MODE, RESERVED_KEYWORD),

	PG_KEYWORD("nocompress", POK_NOCOMPRESS, RESERVED_KEYWORD),
	PG_KEYWORD("not", POK_NOT, RESERVED_KEYWORD),
	PG_KEYWORD("nowait", POK_NOWAIT, RESERVED_KEYWORD),
	PG_KEYWORD("null", POK_NULL, RESERVED_KEYWORD),

	PG_KEYWORD("of", POK_OF, RESERVED_KEYWORD),
	PG_KEYWORD("on", POK_ON, RESERVED_KEYWORD),
	PG_KEYWORD("option", POK_OPTION, RESERVED_KEYWORD),
	PG_KEYWORD("or", POK_OR, RESERVED_KEYWORD),
	PG_KEYWORD("order", POK_ORDER, RESERVED_KEYWORD),
	PG_KEYWORD("overlaps", POK_OVERLAPS, RESERVED_KEYWORD),

	PG_KEYWORD("procedure", POK_PROCEDURE, RESERVED_KEYWORD),
	PG_KEYWORD("public", POK_PUBLIC, RESERVED_KEYWORD),

	PG_KEYWORD("resource", POK_RESOURCE, RESERVED_KEYWORD),
	PG_KEYWORD("revoke", POK_REVOKE, RESERVED_KEYWORD),

	PG_KEYWORD("select", POK_SELECT, RESERVED_KEYWORD),
	PG_KEYWORD("share", POK_SHARE, RESERVED_KEYWORD),
	PG_KEYWORD("size", POK_SIZE, RESERVED_KEYWORD),
	PG_KEYWORD("sql", POK_SQL, RESERVED_KEYWORD),
	PG_KEYWORD("start", POK_START, RESERVED_KEYWORD),
	PG_KEYWORD("subtype", POK_SUBTYPE, RESERVED_KEYWORD),

	PG_KEYWORD("tabauth", POK_TABAUTH, RESERVED_KEYWORD),
	PG_KEYWORD("table", POK_TABLE, RESERVED_KEYWORD),
	PG_KEYWORD("then", POK_THEN, RESERVED_KEYWORD),
	PG_KEYWORD("to", POK_TO, RESERVED_KEYWORD),
	PG_KEYWORD("type", POK_TYPE, RESERVED_KEYWORD),

	PG_KEYWORD("union", POK_UNION, RESERVED_KEYWORD),
	PG_KEYWORD("unique", POK_UNIQUE, RESERVED_KEYWORD),
	PG_KEYWORD("update", POK_UPDATE, RESERVED_KEYWORD),

	PG_KEYWORD("values", POK_VALUES, RESERVED_KEYWORD),
	PG_KEYWORD("view", POK_VIEW, RESERVED_KEYWORD),
	PG_KEYWORD("views", POK_VIEWS, RESERVED_KEYWORD),

	PG_KEYWORD("when", POK_WHEN, RESERVED_KEYWORD),
	PG_KEYWORD("where", POK_WHERE, RESERVED_KEYWORD),
	PG_KEYWORD("with", POK_WITH, RESERVED_KEYWORD)
};

const int num_plora_reserved_keywords = lengthof(plora_reserved_keywords);

const ScanKeyword plora_unreserved_keywords[] = {
	PG_KEYWORD("a", POK_A, UNRESERVED_KEYWORD),
	PG_KEYWORD("accessible", POK_ACCESSIBLE, UNRESERVED_KEYWORD),
	PG_KEYWORD("add", POK_ADD, UNRESERVED_KEYWORD),
	PG_KEYWORD("agent", POK_AGENT, UNRESERVED_KEYWORD),
	PG_KEYWORD("aggregate", POK_AGGREGATE, UNRESERVED_KEYWORD),
	PG_KEYWORD("array", POK_ARRAY, UNRESERVED_KEYWORD),
	PG_KEYWORD("attribute", POK_ATTRIBUTE, UNRESERVED_KEYWORD),
	PG_KEYWORD("authid", POK_AUTHID, UNRESERVED_KEYWORD),
	PG_KEYWORD("avg", POK_AVG, UNRESERVED_KEYWORD),

	PG_KEYWORD("bfile_base", POK_BFILE_BASE, UNRESERVED_KEYWORD),
	PG_KEYWORD("binary", POK_BINARY, UNRESERVED_KEYWORD),
	PG_KEYWORD("blob_base", POK_BLOB_BASE, UNRESERVED_KEYWORD),
	PG_KEYWORD("block", POK_BLOCK, UNRESERVED_KEYWORD),
	PG_KEYWORD("body", POK_BODY, UNRESERVED_KEYWORD),
	PG_KEYWORD("both", POK_BOTH, UNRESERVED_KEYWORD),
	PG_KEYWORD("bound", POK_BOUND, UNRESERVED_KEYWORD),
	PG_KEYWORD("bulk", POK_BULK, UNRESERVED_KEYWORD),
	PG_KEYWORD("byte", POK_BYTE, UNRESERVED_KEYWORD),

	PG_KEYWORD("c", POK_C, UNRESERVED_KEYWORD),
	PG_KEYWORD("call", POK_CALL, UNRESERVED_KEYWORD),
	PG_KEYWORD("calling", POK_CALLING, UNRESERVED_KEYWORD),
	PG_KEYWORD("cascade", POK_CASCADE, UNRESERVED_KEYWORD),
	PG_KEYWORD("char", POK_CHAR, UNRESERVED_KEYWORD),
	PG_KEYWORD("char_base", POK_CHAR_BASE, UNRESERVED_KEYWORD),
	PG_KEYWORD("character", POK_CHARACTER, UNRESERVED_KEYWORD),
	PG_KEYWORD("charset", POK_CHARSET, UNRESERVED_KEYWORD),
	PG_KEYWORD("charsetform", POK_CHARSETFORM, UNRESERVED_KEYWORD),
	PG_KEYWORD("charsetid", POK_CHARSETID, UNRESERVED_KEYWORD),
	PG_KEYWORD("clob_base", POK_CLOB_BASE, UNRESERVED_KEYWORD),
	PG_KEYWORD("clone", POK_CLONE, UNRESERVED_KEYWORD),
	PG_KEYWORD("close", POK_CLOSE, UNRESERVED_KEYWORD),
	PG_KEYWORD("collect", POK_COLLECT, UNRESERVED_KEYWORD),
	PG_KEYWORD("comment", POK_COMMENT, UNRESERVED_KEYWORD),
	PG_KEYWORD("commit", POK_COMMIT, UNRESERVED_KEYWORD),
	PG_KEYWORD("committed", POK_COMMITTED, UNRESERVED_KEYWORD),
	PG_KEYWORD("compiled", POK_COMPILED, UNRESERVED_KEYWORD),
	PG_KEYWORD("constant", POK_CONSTANT, UNRESERVED_KEYWORD),
	PG_KEYWORD("constructor", POK_CONSTRUCTOR, UNRESERVED_KEYWORD),
	PG_KEYWORD("context", POK_CONTEXT, UNRESERVED_KEYWORD),
	PG_KEYWORD("continue", POK_CONTINUE, UNRESERVED_KEYWORD),
	PG_KEYWORD("convert", POK_CONVERT, UNRESERVED_KEYWORD),
	PG_KEYWORD("count", POK_COUNT, UNRESERVED_KEYWORD),
	PG_KEYWORD("credential", POK_CREDENTIAL, UNRESERVED_KEYWORD),
	PG_KEYWORD("current", POK_CURRENT, UNRESERVED_KEYWORD),
	PG_KEYWORD("customdatum", POK_CUSTOMDATUM, UNRESERVED_KEYWORD),

	PG_KEYWORD("dangling", POK_DANGLING, UNRESERVED_KEYWORD),
	PG_KEYWORD("data", POK_DATA, UNRESERVED_KEYWORD),
	PG_KEYWORD("date", POK_DATE, UNRESERVED_KEYWORD),
	PG_KEYWORD("date_base", POK_DATE_BASE, UNRESERVED_KEYWORD),
	PG_KEYWORD("day", POK_DAY, UNRESERVED_KEYWORD),
	PG_KEYWORD("define", POK_DEFINE, UNRESERVED_KEYWORD),
	PG_KEYWORD("delete", POK_DELETE, UNRESERVED_KEYWORD),
	PG_KEYWORD("deterministic", POK_DETERMINISTIC, UNRESERVED_KEYWORD),
	PG_KEYWORD("directory", POK_DIRECTORY, UNRESERVED_KEYWORD),
	PG_KEYWORD("double", POK_DOUBLE, UNRESERVED_KEYWORD),
	PG_KEYWORD("duration", POK_DURATION, UNRESERVED_KEYWORD),

	PG_KEYWORD("element", POK_ELEMENT, UNRESERVED_KEYWORD),
	PG_KEYWORD("elsif", POK_ELSIF, UNRESERVED_KEYWORD),
	PG_KEYWORD("empty", POK_EMPTY, UNRESERVED_KEYWORD),
	PG_KEYWORD("escape", POK_ESCAPE, UNRESERVED_KEYWORD),
	PG_KEYWORD("except", POK_EXCEPT, UNRESERVED_KEYWORD),
	PG_KEYWORD("exceptions", POK_EXCEPTIONS, UNRESERVED_KEYWORD),
	PG_KEYWORD("execute", POK_EXECUTE, UNRESERVED_KEYWORD),
	PG_KEYWORD("exists", POK_EXISTS, UNRESERVED_KEYWORD),
	PG_KEYWORD("exit", POK_EXIT, UNRESERVED_KEYWORD),
	PG_KEYWORD("external", POK_EXTERNAL, UNRESERVED_KEYWORD),

	PG_KEYWORD("final", POK_FINAL, UNRESERVED_KEYWORD),
	PG_KEYWORD("first", POK_FIRST, UNRESERVED_KEYWORD),
	PG_KEYWORD("fixed", POK_FIXED, UNRESERVED_KEYWORD),
	PG_KEYWORD("float", POK_FLOAT, UNRESERVED_KEYWORD),
	PG_KEYWORD("forall", POK_FORALL, UNRESERVED_KEYWORD),
	PG_KEYWORD("force", POK_FORCE, UNRESERVED_KEYWORD),

	PG_KEYWORD("general", POK_GENERAL, UNRESERVED_KEYWORD),

	PG_KEYWORD("hash", POK_HASH, UNRESERVED_KEYWORD),
	PG_KEYWORD("heap", POK_HEAP, UNRESERVED_KEYWORD),
	PG_KEYWORD("hidden", POK_HIDDEN, UNRESERVED_KEYWORD),
	PG_KEYWORD("hour", POK_HOUR, UNRESERVED_KEYWORD),

	PG_KEYWORD("immediate", POK_IMMEDIATE, UNRESERVED_KEYWORD),
	PG_KEYWORD("including", POK_INCLUDING, UNRESERVED_KEYWORD),
	PG_KEYWORD("indicator", POK_INDICATOR, UNRESERVED_KEYWORD),
	PG_KEYWORD("indices", POK_INDICES, UNRESERVED_KEYWORD),
	PG_KEYWORD("infinite", POK_INFINITE, UNRESERVED_KEYWORD),
	PG_KEYWORD("instantiable", POK_INSTANTIABLE, UNRESERVED_KEYWORD),
	PG_KEYWORD("int", POK_INT, UNRESERVED_KEYWORD),
	PG_KEYWORD("interface", POK_INTERFACE, UNRESERVED_KEYWORD),
	PG_KEYWORD("interval", POK_INTERVAL, UNRESERVED_KEYWORD),
	PG_KEYWORD("invalidate", POK_INVALIDATE, UNRESERVED_KEYWORD),
	PG_KEYWORD("isolation", POK_ISOLATION, UNRESERVED_KEYWORD),

	PG_KEYWORD("java", POK_JAVA, UNRESERVED_KEYWORD),

	PG_KEYWORD("language", POK_LANGUAGE, UNRESERVED_KEYWORD),
	PG_KEYWORD("large", POK_LARGE, UNRESERVED_KEYWORD),
	PG_KEYWORD("leading", POK_LEADING, UNRESERVED_KEYWORD),
	PG_KEYWORD("length", POK_LENGTH, UNRESERVED_KEYWORD),
	PG_KEYWORD("level", POK_LEVEL, UNRESERVED_KEYWORD),
	PG_KEYWORD("library", POK_LIBRARY, UNRESERVED_KEYWORD),
	PG_KEYWORD("like2", POK_LIKE2, UNRESERVED_KEYWORD),
	PG_KEYWORD("like4", POK_LIKE4, UNRESERVED_KEYWORD),
	PG_KEYWORD("likec", POK_LIKEC, UNRESERVED_KEYWORD),
	PG_KEYWORD("limit", POK_LIMIT, UNRESERVED_KEYWORD),
	PG_KEYWORD("limited", POK_LIMITED, UNRESERVED_KEYWORD),
	PG_KEYWORD("local", POK_LOCAL, UNRESERVED_KEYWORD),
	PG_KEYWORD("long", POK_LONG, UNRESERVED_KEYWORD),
	PG_KEYWORD("loop", POK_LOOP, UNRESERVED_KEYWORD),

	PG_KEYWORD("map", POK_MAP, UNRESERVED_KEYWORD),
	PG_KEYWORD("max", POK_MAX, UNRESERVED_KEYWORD),
	PG_KEYWORD("maxlen", POK_MAXLEN, UNRESERVED_KEYWORD),
	PG_KEYWORD("member", POK_MEMBER, UNRESERVED_KEYWORD),
	PG_KEYWORD("merge", POK_MERGE, UNRESERVED_KEYWORD),
	PG_KEYWORD("min", POK_MIN, UNRESERVED_KEYWORD),
	PG_KEYWORD("minute", POK_MINUTE, UNRESERVED_KEYWORD),
	PG_KEYWORD("mod", POK_MOD, UNRESERVED_KEYWORD),
	PG_KEYWORD("modify", POK_MODIFY, UNRESERVED_KEYWORD),
	PG_KEYWORD("month", POK_MONTH, UNRESERVED_KEYWORD),
	PG_KEYWORD("multiset", POK_MULTISET, UNRESERVED_KEYWORD),

	PG_KEYWORD("name", POK_NAME, UNRESERVED_KEYWORD),
	PG_KEYWORD("nan", POK_NAN, UNRESERVED_KEYWORD),
	PG_KEYWORD("national", POK_NATIONAL, UNRESERVED_KEYWORD),
	PG_KEYWORD("native", POK_NATIVE, UNRESERVED_KEYWORD),
	PG_KEYWORD("nchar", POK_NCHAR, UNRESERVED_KEYWORD),
	PG_KEYWORD("new", POK_NEW, UNRESERVED_KEYWORD),
	PG_KEYWORD("nocopy", POK_NOCOPY, UNRESERVED_KEYWORD),
	PG_KEYWORD("number_base", POK_NUMBER_BASE, UNRESERVED_KEYWORD),

	PG_KEYWORD("object", POK_OBJECT, UNRESERVED_KEYWORD),
	PG_KEYWORD("ocicoll", POK_OCICOLL, UNRESERVED_KEYWORD),
	PG_KEYWORD("ocidate", POK_OCIDATE, UNRESERVED_KEYWORD),
	PG_KEYWORD("ocidatetime", POK_OCIDATETIME, UNRESERVED_KEYWORD),
	PG_KEYWORD("ociduration", POK_OCIDURATION, UNRESERVED_KEYWORD),
	PG_KEYWORD("ociinterval", POK_OCIINTERVAL, UNRESERVED_KEYWORD),
	PG_KEYWORD("ociloblocator", POK_OCILOBLOCATOR, UNRESERVED_KEYWORD),
	PG_KEYWORD("ocinumber", POK_OCINUMBER, UNRESERVED_KEYWORD),
	PG_KEYWORD("ociraw", POK_OCIRAW, UNRESERVED_KEYWORD),
	PG_KEYWORD("ociref", POK_OCIREF, UNRESERVED_KEYWORD),
	PG_KEYWORD("ocirefcursor", POK_OCIREFCURSOR, UNRESERVED_KEYWORD),
	PG_KEYWORD("ocirowid", POK_OCIROWID, UNRESERVED_KEYWORD),
	PG_KEYWORD("ocistring", POK_OCISTRING, UNRESERVED_KEYWORD),
	PG_KEYWORD("ocitype", POK_OCITYPE, UNRESERVED_KEYWORD),
	PG_KEYWORD("old", POK_OLD, UNRESERVED_KEYWORD),
	PG_KEYWORD("only", POK_ONLY, UNRESERVED_KEYWORD),
	PG_KEYWORD("opaque", POK_OPAQUE, UNRESERVED_KEYWORD),
	PG_KEYWORD("open", POK_OPEN, UNRESERVED_KEYWORD),
	PG_KEYWORD("operator", POK_OPERATOR, UNRESERVED_KEYWORD),
	PG_KEYWORD("oracle", POK_ORACLE, UNRESERVED_KEYWORD),
	PG_KEYWORD("oradata", POK_ORADATA, UNRESERVED_KEYWORD),
	PG_KEYWORD("organization", POK_ORGANIZATION, UNRESERVED_KEYWORD),
	PG_KEYWORD("orlany", POK_ORLANY, UNRESERVED_KEYWORD),
	PG_KEYWORD("orlvary", POK_ORLVARY, UNRESERVED_KEYWORD),
	PG_KEYWORD("others", POK_OTHERS, UNRESERVED_KEYWORD),
	PG_KEYWORD("out", POK_OUT, UNRESERVED_KEYWORD),
	PG_KEYWORD("overriding", POK_OVERRIDING, UNRESERVED_KEYWORD),

	PG_KEYWORD("package", POK_PACKAGE, UNRESERVED_KEYWORD),
	PG_KEYWORD("parallel_enable", POK_PARALLEL_ENABLE, UNRESERVED_KEYWORD),
	PG_KEYWORD("parameter", POK_PARAMETER, UNRESERVED_KEYWORD),
	PG_KEYWORD("parameters", POK_PARAMETERS, UNRESERVED_KEYWORD),
	PG_KEYWORD("parent", POK_PARENT, UNRESERVED_KEYWORD),
	PG_KEYWORD("partition", POK_PARTITION, UNRESERVED_KEYWORD),
	PG_KEYWORD("pascal", POK_PASCAL, UNRESERVED_KEYWORD),
	PG_KEYWORD("pipe", POK_PIPE, UNRESERVED_KEYWORD),
	PG_KEYWORD("pipelined", POK_PIPELINED, UNRESERVED_KEYWORD),
	PG_KEYWORD("pluggable", POK_PLUGGABLE, UNRESERVED_KEYWORD),
	PG_KEYWORD("pragma", POK_PRAGMA, UNRESERVED_KEYWORD),
	PG_KEYWORD("precision", POK_PRECISION, UNRESERVED_KEYWORD),
	PG_KEYWORD("prior", POK_PRIOR, UNRESERVED_KEYWORD),
	PG_KEYWORD("private", POK_PRIVATE, UNRESERVED_KEYWORD),

	PG_KEYWORD("raise", POK_RAISE, UNRESERVED_KEYWORD),
	PG_KEYWORD("range", POK_RANGE, UNRESERVED_KEYWORD),
	PG_KEYWORD("raw", POK_RAW, UNRESERVED_KEYWORD),
	PG_KEYWORD("read", POK_READ, UNRESERVED_KEYWORD),
	PG_KEYWORD("record", POK_RECORD, UNRESERVED_KEYWORD),
	PG_KEYWORD("ref", POK_REF, UNRESERVED_KEYWORD),
	PG_KEYWORD("reference", POK_REFERENCE, UNRESERVED_KEYWORD),
	PG_KEYWORD("relies_on", POK_RELIES_ON, UNRESERVED_KEYWORD),
	PG_KEYWORD("rem", POK_REM, UNRESERVED_KEYWORD),
	PG_KEYWORD("remainder", POK_REMAINDER, UNRESERVED_KEYWORD),
	PG_KEYWORD("rename", POK_RENAME, UNRESERVED_KEYWORD),
	PG_KEYWORD("result", POK_RESULT, UNRESERVED_KEYWORD),
	PG_KEYWORD("result_cache", POK_RESULT_CACHE, UNRESERVED_KEYWORD),
	PG_KEYWORD("return", POK_RETURN, UNRESERVED_KEYWORD),
	PG_KEYWORD("returning", POK_RETURNING, UNRESERVED_KEYWORD),
	PG_KEYWORD("reverse", POK_REVERSE, UNRESERVED_KEYWORD),
	PG_KEYWORD("rollback", POK_ROLLBACK, UNRESERVED_KEYWORD),
	PG_KEYWORD("row", POK_ROW, UNRESERVED_KEYWORD),

	PG_KEYWORD("sample", POK_SAMPLE, UNRESERVED_KEYWORD),
	PG_KEYWORD("save", POK_SAVE, UNRESERVED_KEYWORD),
	PG_KEYWORD("savepoint", POK_SAVEPOINT, UNRESERVED_KEYWORD),
	PG_KEYWORD("sb1", POK_SB1, UNRESERVED_KEYWORD),
	PG_KEYWORD("sb2", POK_SB2, UNRESERVED_KEYWORD),
	PG_KEYWORD("sb4", POK_SB4, UNRESERVED_KEYWORD),
	PG_KEYWORD("second", POK_SECOND, UNRESERVED_KEYWORD),
	PG_KEYWORD("segment", POK_SEGMENT, UNRESERVED_KEYWORD),
	PG_KEYWORD("self", POK_SELF, UNRESERVED_KEYWORD),
	PG_KEYWORD("separate", POK_SEPARATE, UNRESERVED_KEYWORD),
	PG_KEYWORD("sequence", POK_SEQUENCE, UNRESERVED_KEYWORD),
	PG_KEYWORD("serializable", POK_SERIALIZABLE, UNRESERVED_KEYWORD),
	PG_KEYWORD("set", POK_SET, UNRESERVED_KEYWORD),
	PG_KEYWORD("short", POK_SHORT, UNRESERVED_KEYWORD),
	PG_KEYWORD("size_t", POK_SIZE_T, UNRESERVED_KEYWORD),
	PG_KEYWORD("some", POK_SOME, UNRESERVED_KEYWORD),
	PG_KEYWORD("sparse", POK_SPARSE, UNRESERVED_KEYWORD),
	PG_KEYWORD("sqlcode", POK_SQLCODE, UNRESERVED_KEYWORD),
	PG_KEYWORD("sqldata", POK_SQLDATA, UNRESERVED_KEYWORD),
	PG_KEYWORD("sqlname", POK_SQLNAME, UNRESERVED_KEYWORD),
	PG_KEYWORD("sqlstate", POK_SQLSTATE, UNRESERVED_KEYWORD),
	PG_KEYWORD("standard", POK_STANDARD, UNRESERVED_KEYWORD),
	PG_KEYWORD("static", POK_STATIC, UNRESERVED_KEYWORD),
	PG_KEYWORD("stddev", POK_STDDEV, UNRESERVED_KEYWORD),
	PG_KEYWORD("stored", POK_STORED, UNRESERVED_KEYWORD),
	PG_KEYWORD("strict", POK_STRICT, UNRESERVED_KEYWORD),
	PG_KEYWORD("string", POK_STRING, UNRESERVED_KEYWORD),
	PG_KEYWORD("struct", POK_STRUCT, UNRESERVED_KEYWORD),
	PG_KEYWORD("style", POK_STYLE, UNRESERVED_KEYWORD),
	PG_KEYWORD("submultiset", POK_SUBMULTISET, UNRESERVED_KEYWORD),
	PG_KEYWORD("subpartition", POK_SUBPARTITION, UNRESERVED_KEYWORD),
	PG_KEYWORD("substitutable", POK_SUBSTITUTABLE, UNRESERVED_KEYWORD),
	PG_KEYWORD("sum", POK_SUM, UNRESERVED_KEYWORD),
	PG_KEYWORD("synonym", POK_SYNONYM, UNRESERVED_KEYWORD),

	PG_KEYWORD("tdo", POK_TDO, UNRESERVED_KEYWORD),
	PG_KEYWORD("the", POK_THE, UNRESERVED_KEYWORD),
	PG_KEYWORD("time", POK_TIME, UNRESERVED_KEYWORD),
	PG_KEYWORD("timestamp", POK_TIMESTAMP, UNRESERVED_KEYWORD),
	PG_KEYWORD("timezone_abbr", POK_TIMEZONE_ABBR, UNRESERVED_KEYWORD),
	PG_KEYWORD("timezone_hour", POK_TIMEZONE_HOUR, UNRESERVED_KEYWORD),
	PG_KEYWORD("timezone_minute", POK_TIMEZONE_MINUTE, UNRESERVED_KEYWORD),
	PG_KEYWORD("timezone_region", POK_TIMEZONE_REGION, UNRESERVED_KEYWORD),
	PG_KEYWORD("trailing", POK_TRAILING, UNRESERVED_KEYWORD),
	PG_KEYWORD("transaction", POK_TRANSACTION, UNRESERVED_KEYWORD),
	PG_KEYWORD("transactional", POK_TRANSACTIONAL, UNRESERVED_KEYWORD),
	PG_KEYWORD("trusted", POK_TRUSTED, UNRESERVED_KEYWORD),

	PG_KEYWORD("ub1", POK_UB1, UNRESERVED_KEYWORD),
	PG_KEYWORD("ub2", POK_UB2, UNRESERVED_KEYWORD),
	PG_KEYWORD("ub4", POK_UB4, UNRESERVED_KEYWORD),
	PG_KEYWORD("under", POK_UNDER, UNRESERVED_KEYWORD),
	PG_KEYWORD("unplug", POK_UNPLUG, UNRESERVED_KEYWORD),
	PG_KEYWORD("unsigned", POK_UNSIGNED, UNRESERVED_KEYWORD),
	PG_KEYWORD("untrusted", POK_UNTRUSTED, UNRESERVED_KEYWORD),
	PG_KEYWORD("use", POK_USE, UNRESERVED_KEYWORD),
	PG_KEYWORD("using", POK_USING, UNRESERVED_KEYWORD),

	PG_KEYWORD("valist", POK_VALIST, UNRESERVED_KEYWORD),
	PG_KEYWORD("value", POK_VALUE, UNRESERVED_KEYWORD),
	PG_KEYWORD("variable", POK_VARIABLE, UNRESERVED_KEYWORD),
	PG_KEYWORD("variance", POK_VARIANCE, UNRESERVED_KEYWORD),
	PG_KEYWORD("varray", POK_VARRAY, UNRESERVED_KEYWORD),
	PG_KEYWORD("varying", POK_VARYING, UNRESERVED_KEYWORD),
	PG_KEYWORD("void", POK_VOID, UNRESERVED_KEYWORD),

	PG_KEYWORD("while", POK_WHILE, UNRESERVED_KEYWORD),
	PG_KEYWORD("work", POK_WORK, UNRESERVED_KEYWORD),
	PG_KEYWORD("wrapped", POK_WRAPPED, UNRESERVED_KEYWORD),
	PG_KEYWORD("write", POK_WRITE, UNRESERVED_KEYWORD),

	PG_KEYWORD("year", POK_YEAR, UNRESERVED_KEYWORD),

	PG_KEYWORD("zone", POK_ZONE, UNRESERVED_KEYWORD)
};

const int num_plora_unreserved_keywords = lengthof(plora_unreserved_keywords);
