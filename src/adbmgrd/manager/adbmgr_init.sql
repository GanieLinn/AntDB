--views
CREATE VIEW adbmgr.host AS
  SELECT
    hostname AS name,
    hostuser AS user,
    hostport AS port,
    CASE hostproto
      WHEN 's' THEN 'ssh'::text
      WHEN 't' THEN 'telnet'::text
    END AS protocol,
    hostagentport  AS agentport,
    hostaddr AS address,
    hostadbhome AS adbhome
  FROM pg_catalog.mgr_host order by 1;

CREATE VIEW adbmgr.parm AS
SELECT
	parmtype		AS	type,
	parmname		AS	name,
	parmvalue		AS	value,
	parmcontext		AS	context,
	parmvartype		AS	vartype,
	parmunit		AS	unit,
	parmminval		AS	minval,
	parmmaxval		AS	maxval,
	parmenumval		AS	enumval
FROM pg_catalog.mgr_parm;

CREATE VIEW adbmgr.updateparm AS
SELECT
	updateparmnodename		AS	nodename,
	CASE updateparmnodetype
		WHEN 'c' THEN 'coordinator master'::text
		WHEN 'd' THEN 'datanode master'::text
		WHEN 'b' THEN 'datanode slave'::text
		WHEN 'g' THEN 'gtm master'::text
		WHEN 'p' THEN 'gtm slave'::text
		WHEN 'G' THEN 'gtm master|slave'::text
		WHEN 'C' THEN 'coordinator master|slave'::text
		WHEN 'D' THEN 'datanode master|slave'::text
	END AS nodetype,
	updateparmkey			AS	key,
	updateparmvalue			AS	value
FROM pg_catalog.mgr_updateparm order by 1,2,3;

CREATE VIEW adbmgr.node AS
	SELECT * FROM(
  SELECT
    mgrnode.nodename    AS  name,
    hostname   AS  host,
    CASE mgrnode.nodetype
      WHEN 'g' THEN 'gtm master'::text
      WHEN 'p' THEN 'gtm slave'::text
      WHEN 'c' THEN 'coordinator master'::text
      WHEN 's' THEN 'coordinator slave'::text
      WHEN 'd' THEN 'datanode master'::text
      WHEN 'b' THEN 'datanode slave'::text
    END AS type,
    node_alise.nodename AS mastername,
    mgrnode.nodeport    AS  port,
    mgrnode.nodesync    AS  sync_state,
    mgrnode.nodepath    AS  path,
    mgrnode.nodeinited  AS  initialized,
    mgrnode.nodeincluster AS incluster,
    mgrnode.nodereadonly AS readonly
  FROM pg_catalog.mgr_node AS mgrnode LEFT JOIN pg_catalog.mgr_host ON mgrnode.nodehost = pg_catalog.mgr_host.oid
		LEFT JOIN pg_catalog.mgr_node AS node_alise ON node_alise.oid = mgrnode.nodemasternameoid) AS node_tb
		order by (case type
			when 'gtm master' then 0
			when 'gtm slave' then 1
			when 'coordinator master' then 2
			when 'coordinator slave' then 2
			when 'datanode master' then 3
			when 'datanode slave' then 4
			End) ASC, mastername ASC, sync_state DESC;

CREATE VIEW adbmgr.job AS
  SELECT
		oid AS joboid,
    name,
    next_time AS nexttime,
    interval,
    status,
		command,
		description
  FROM pg_catalog.monitor_job order by 2;

CREATE VIEW adbmgr.jobitem AS
  SELECT
		jobitem_itemname AS item,
    jobitem_path AS path,
    jobitem_desc AS description
  FROM pg_catalog.monitor_jobitem order by 1;

CREATE VIEW adbmgr.ha as
	SELECT * FROM mgr_monitor_ha() order by 2 asc, 1 desc;

--monitor all
CREATE VIEW adbmgr.monitor_all AS
        select * from mgr_monitor_all() order by 1,
			(case nodetype
				when 'gtm master' then 0
				when 'gtm slave' then 1
				when 'coordinator master' then 2
				when 'coordinator slave' then 3
				when 'datanode master' then 4
				when 'datanode slave' then 5
			End) ASC;

--list hba
CREATE VIEW adbmgr.hba AS
		select * from mgr_hba order by nodename;

--init all
CREATE VIEW adbmgr.initall AS
	SELECT 'init gtm master' AS "operation type",* FROM mgr_init_gtm_master()
	UNION ALL
	SELECT 'start gtm master' AS "operation type", * FROM mgr_start_gtm_master(NULL)
	UNION ALL
	SELECT 'init gtm slave' AS "operation type",* FROM mgr_init_gtm_slave()
	UNION ALL
	SELECT 'start gtm slave' AS "operation type", * FROM mgr_start_gtm_slave(NULL)
	UNION ALL
	SELECT 'init coordinator' AS "operation type",* FROM mgr_init_cn_master(NULL)
	UNION ALL
	SELECT 'start coordinator master' AS "operation type", * FROM mgr_start_cn_master(NULL)
	UNION ALL
	SELECT 'init datanode master' AS "operation type", * FROM mgr_init_dn_master(NULL)
	UNION ALL
	SELECT 'start datanode master' AS "operation type", * FROM mgr_start_dn_master(NULL)
	UNION ALL
	SELECT 'init datanode slave' AS "operation type", * FROM mgr_init_dn_slave_all()
	UNION ALL
	SELECT 'start datanode slave' AS "operation type", * FROM mgr_start_dn_slave(NULL)
	UNION ALL
	SELECT 'config coordinator' AS "operation type", * FROM mgr_configure_nodes_all(NULL);

--start gtm all
CREATE VIEW adbmgr.start_gtm_all AS
	SELECT 'start gtm master' AS "operation type", * FROM mgr_start_gtm_master(NULL)
	UNION all
	SELECT 'start gtm slave' AS "operation type", * FROM mgr_start_gtm_slave(NULL);
--stop gtm all
CREATE VIEW adbmgr.stop_gtm_all AS
	SELECT 'stop gtm slave' AS "operation type", * FROM mgr_stop_gtm_slave('smart', NULL)
	UNION all
	SELECT 'stop gtm master' AS "operation type", * FROM mgr_stop_gtm_master('smart', NULL);
--stop gtm all -m f
CREATE VIEW adbmgr.stop_gtm_all_f AS
	SELECT 'stop gtm slave' AS "operation type", * FROM mgr_stop_gtm_slave('fast', NULL)
	UNION all
	SELECT 'stop gtm master' AS "operation type", * FROM mgr_stop_gtm_master('fast', NULL);
--stop gtm all -m i
CREATE VIEW adbmgr.stop_gtm_all_i AS
	SELECT 'stop gtm slave' AS "operation type", * FROM mgr_stop_gtm_slave('immediate', NULL)
	UNION all
	SELECT 'stop gtm master' AS "operation type", * FROM mgr_stop_gtm_master('immediate', NULL);
--init datanode all
CREATE VIEW adbmgr.initdatanodeall AS
    SELECT 'init datanode master' AS "operation type",* FROM mgr_init_dn_master(NULL)
    UNION all
    SELECT 'init datanode slave' AS "operation type", * FROM mgr_init_dn_slave_all();

--start datanode all
CREATE VIEW adbmgr.start_datanode_all AS
    SELECT 'start datanode master' AS "operation type", * FROM mgr_start_dn_master(NULL)
    UNION all
    SELECT 'start datanode slave' AS "operation type", * FROM mgr_start_dn_slave(NULL);

--start all
CREATE VIEW adbmgr.startall AS
    SELECT 'start gtm master' AS "operation type", * FROM mgr_start_gtm_master(NULL)
    UNION all
    SELECT 'start gtm slave' AS "operation type", * FROM mgr_start_gtm_slave(NULL)
    UNION all
    SELECT 'start coordinator master' AS "operation type", * FROM mgr_start_cn_master(NULL)
    UNION all
    SELECT 'start datanode master' AS "operation type", * FROM mgr_start_dn_master(NULL)
    UNION all
    SELECT 'start datanode slave' AS "operation type", * FROM mgr_start_dn_slave(NULL);

--stop datanode all
CREATE VIEW adbmgr.stop_datanode_all AS
    SELECT 'stop datanode slave' AS "operation type", * FROM mgr_stop_dn_slave('smart', NULL)
    UNION all
    SELECT 'stop datanode master' AS "operation type", * FROM mgr_stop_dn_master('smart', NULL);

CREATE VIEW adbmgr.stop_datanode_all_f AS
    SELECT 'stop datanode slave' AS "operation type", * FROM mgr_stop_dn_slave('fast', NULL)
    UNION all
    SELECT 'stop datanode master' AS "operation type", * FROM mgr_stop_dn_master('fast', NULL);

CREATE VIEW adbmgr.stop_datanode_all_i AS
    SELECT 'stop datanode slave' AS "operation type", * FROM mgr_stop_dn_slave('immediate', NULL)
    UNION all
    SELECT 'stop datanode master' AS "operation type", * FROM mgr_stop_dn_master('immediate', NULL);

--stop all
CREATE VIEW adbmgr.stopall AS
    SELECT 'stop datanode slave' AS "operation type", * FROM mgr_stop_dn_slave('smart', NULL)
    UNION all
    SELECT 'stop datanode master' AS "operation type", * FROM mgr_stop_dn_master('smart', NULL)
    UNION all
    SELECT 'stop coordinator' AS "operation type", * FROM mgr_stop_cn_master('smart', NULL)
    UNION all
    SELECT 'stop gtm slave' AS "operation type", * FROM mgr_stop_gtm_slave('smart', NULL)
    UNION all
    SELECT 'stop gtm master' AS "operation type", * FROM mgr_stop_gtm_master('smart', NULL);

CREATE VIEW adbmgr.stopall_f AS
    SELECT 'stop datanode slave' AS "operation type", * FROM mgr_stop_dn_slave('fast', NULL)
    UNION all
    SELECT 'stop datanode master' AS "operation type", * FROM mgr_stop_dn_master('fast', NULL)
    UNION all
    SELECT 'stop coordinator master' AS "operation type", * FROM mgr_stop_cn_master('fast', NULL)
    UNION all
    SELECT 'stop gtm slave' AS "operation type", * FROM mgr_stop_gtm_slave('fast', NULL)
    UNION all
    SELECT 'stop gtm master' AS "operation type", * FROM mgr_stop_gtm_master('fast', NULL);

CREATE VIEW adbmgr.stopall_i AS
    SELECT 'stop datanode slave' AS "operation type", * FROM mgr_stop_dn_slave('immediate', NULL)
    UNION all
    SELECT 'stop datanode master' AS "operation type", * FROM mgr_stop_dn_master('immediate', NULL)
    UNION all
    SELECT 'stop coordinator master' AS "operation type", * FROM mgr_stop_cn_master('immediate', NULL)
    UNION all
    SELECT 'stop gtm slave' AS "operation type", * FROM mgr_stop_gtm_slave('immediate', NULL)
    UNION all
    SELECT 'stop gtm master' AS "operation type", * FROM mgr_stop_gtm_master('immediate', NULL);

-- for ADB monitor host page: get all host various parameters.
CREATE VIEW adbmgr.get_all_host_parm AS
    select
        mgh.hostname AS hostName,
        mgh.hostaddr AS ip,
        nh.mh_cpu_core_available AS cpu,
        round(c.mc_cpu_usage::numeric, 1) AS cpuRate,
        round((m.mm_total/1024.0/1024.0/1024.0)::numeric, 1) AS mem,
        round(m.mm_usage::numeric, 1) AS memRate,
        round((d.md_total/1024.0/1024.0/1024.0)::numeric, 1) AS disk,
        round(((d.md_used/d.md_total::float) * 100)::numeric, 1) AS diskRate,
        mtc.mt_emergency_threshold AS cpu_threshold,
        mtm.mt_emergency_threshold AS mem_threshold,
        mtd.mt_emergency_threshold AS disk_threshold
    from
        mgr_host mgh,

        (
            select * from (
                            select *, (ROW_NUMBER()OVER(PARTITION BY T.hostname ORDER BY T.mc_timestamptz desc)) as rm
                            from monitor_cpu t
                           ) tt where tt.rm = 1
        ) c,

        (
            select * from (
                            select *,(ROW_NUMBER()OVER(PARTITION BY T.hostname ORDER BY T.mm_timestamptz desc)) as rm
                            from monitor_mem t
                           ) tt where tt.rm =1
        ) m,

        (
            select * from (
                            select *, (ROW_NUMBER()OVER(PARTITION BY T.hostname ORDER BY T.md_timestamptz desc)) as rm
                            from monitor_disk t
                           ) tt where tt.rm = 1
        ) d,

        (
            select * from (
                            select *, (ROW_NUMBER()OVER(PARTITION BY T.hostname ORDER BY T.mh_current_time desc)) as rm
                            from monitor_host t
                           ) tt where tt.rm = 1
        ) nh,

        (
            select * from monitor_host_threshold where mt_type = 1
        ) mtc,

        (
            select * from monitor_host_threshold where mt_type = 2
        ) mtm,

        (
            select * from monitor_host_threshold where mt_type = 3
        )mtd
    where mgh.hostname = c.hostname and
        c.hostname = m.hostname and
        m.hostname = d.hostname and
        d.hostname = nh.hostname;

-- for ADB monitor host page: get specific host various parameters.
CREATE VIEW adbmgr.get_spec_host_parm AS
    select mgh.hostname as hostname,
       mgh.hostaddr as ip,
       mh.mh_system as os,
       mh.mh_cpu_core_available as cpu,
       round((mm.mm_total/1024.0/1024.0/1024.0)::numeric, 1) as mem,
       round((md.md_total/1024.0/1024.0/1024.0)::numeric, 1) as disk,
       mh.mh_current_time - (mh.mh_seconds_since_boot || 'sec')::interval as createtime,
       mh_run_state as state,
       round(mc.mc_cpu_usage::numeric, 1) as cpurate,
       round(mm.mm_usage::numeric, 1) as memrate,
       round(((md.md_used/md.md_total::float) * 100)::numeric, 1) as diskRate,
       round((md.md_io_read_bytes/1024.0/1024.0)/(md.md_io_read_time/1000.0), 1) as ioreadps,
       round((md.md_io_write_bytes/1024.0/1024.0)/(md.md_io_write_time/1000.0), 1) as iowriteps,
       round(mn.mn_recv/1024.0,1) as netinps,
       round(mn.mn_sent/1024.0,1) as netoutps,
       mh.mh_seconds_since_boot as runtime
    from mgr_host mgh,

        (
            select * from (
                            select *, (ROW_NUMBER()OVER(PARTITION BY t.hostname ORDER BY t.mh_current_time desc)) as rm
                            from monitor_host t
                           ) tt where tt.rm = 1
        ) mh,

        (
            select * from (
                            select *,(ROW_NUMBER()OVER(PARTITION BY t.hostname ORDER BY t.mc_timestamptz desc)) as rm
                            from monitor_cpu t
                           ) tt where tt.rm = 1
        ) mc,

        (
            select * from (
                            select *,(ROW_NUMBER()OVER(PARTITION BY t.hostname ORDER BY t.mm_timestamptz desc)) as rm
                            from monitor_mem t
                           ) tt where tt.rm =1
        ) mm,

        (
            select * from (
                            select *, (ROW_NUMBER()OVER(PARTITION BY t.hostname ORDER BY t.md_timestamptz desc)) as rm
                            from monitor_disk t
                           ) tt where tt.rm = 1
        ) md,

        (
            select * from (
                            select *, (ROW_NUMBER()OVER(PARTITION BY t.hostname ORDER BY t.mn_timestamptz desc)) as rm
                            from monitor_net t
                           ) tt where tt.rm = 1
        ) mn
    where mgh.hostname = mh.hostname and
        mh.hostname = mc.hostname and
        mc.hostname = mm.hostname and
        mm.hostname = md.hostname and
        md.hostname = mn.hostname;

-- for ADB monitor host page: get cpu, memory, i/o and net info for specific time period.
CREATE OR REPLACE FUNCTION pg_catalog.get_host_history_usage(hostname text, i int)
    RETURNS table
    (
        recordtimes timestamptz,
        cpuuseds numeric,
        memuseds numeric,
        ioreadps numeric,
        iowriteps numeric,
        netinps numeric,
        netoutps numeric
    )
    AS
    $$

    select c.mc_timestamptz as recordtimes,
           round(c.mc_cpu_usage::numeric, 1) as cpuuseds,
           round(m.mm_usage::numeric, 1) as memuseds,
           round((d.md_io_read_bytes/1024.0/1024.0)/(d.md_io_read_time/1000.0), 1) as ioreadps,
           round((d.md_io_write_bytes/1024.0/1024.0)/(d.md_io_write_time/1000.0), 1) as iowriteps,
           round(n.mn_recv/1024.0,1) as netinps,
           round(n.mn_sent/1024.0,1) as netoutps
    from monitor_cpu c left join monitor_mem  m on(c.hostname = m.hostname and c.mc_timestamptz = m.mm_timestamptz)
                       left join monitor_disk d on(c.hostname = d.hostname and c.mc_timestamptz = d.md_timestamptz)
                       left join monitor_net  n on(c.hostname = n.hostname and c.mc_timestamptz = n.mn_timestamptz)
                       left join monitor_host h on(c.hostname = h.hostname and c.mc_timestamptz = h.mh_current_time)
                       left join mgr_host   mgr on(c.hostname = mgr.hostname),

                       (select mh.hostname, mh.mh_current_time
                        from monitor_host mh
                        order by mh.mh_current_time desc
                        limit 1) as temp
    where c.mc_timestamptz  >  temp.mh_current_time - case $2
                                                      when 0 then interval '1 hour'
                                                      when 1 then interval '1 day'
                                                      when 2 then interval '7 day'
                                                      end and
          mgr.hostname = $1
   order by 1 asc;
    $$
    LANGUAGE SQL
    IMMUTABLE
    RETURNS NULL ON NULL INPUT;

-- for ADB monitor host page: get cpu, memory, i/o and net info for specific time period.
CREATE OR REPLACE FUNCTION pg_catalog.get_host_history_usage_by_time_period(hostname text, starttime timestamptz, endtime timestamptz)
    RETURNS table
    (
        recordtimes timestamptz,
        cpuuseds numeric,
        memuseds numeric,
        ioreadps numeric,
        iowriteps numeric,
        netinps numeric,
        netoutps numeric
    )
    AS
    $$

    select c.mc_timestamptz as recordtimes,
           round(c.mc_cpu_usage::numeric, 1) as cpuuseds,
           round(m.mm_usage::numeric, 1) as memuseds,
           round((d.md_io_read_bytes/1024.0/1024.0)/(d.md_io_read_time/1000.0), 1) as ioreadps,
           round((d.md_io_write_bytes/1024.0/1024.0)/(d.md_io_write_time/1000.0), 1) as iowriteps,
           round(n.mn_recv/1024.0,1) as netinps,
           round(n.mn_sent/1024.0,1) as netoutps
    from monitor_cpu c left join monitor_mem  m on(c.hostname = m.hostname and c.mc_timestamptz = m.mm_timestamptz)
                       left join monitor_disk d on(c.hostname = d.hostname and c.mc_timestamptz = d.md_timestamptz)
                       left join monitor_net  n on(c.hostname = n.hostname and c.mc_timestamptz = n.mn_timestamptz)
                       left join monitor_host h on(c.hostname = h.hostname and c.mc_timestamptz = h.mh_current_time)
                       left join mgr_host   mgr on(c.hostname = mgr.hostname),

                       (select mh.hostname, mh.mh_current_time
                        from monitor_host mh
                        order by mh.mh_current_time desc
                        limit 1) as temp
    where c.mc_timestamptz  between $2 and $3
            and mgr.hostname = $1
    order by 1 asc;
    $$
    LANGUAGE SQL
    IMMUTABLE
    RETURNS NULL ON NULL INPUT;

-- for ADB monitor host page: The names of all the nodes on a host
CREATE OR REPLACE FUNCTION pg_catalog.get_all_nodename_in_spec_host(hostname text)
    RETURNS table
	(
		nodename name,
		nodetype text,
		nodesync name
	)
    AS
    $$
    select
	nodename,
		case nodetype
		when 'g' then 'gtm master'
		when 'p' then 'gtm slave'
		when 'c' then 'coordinator'
		when 'd' then 'datanode master'
		when 'b' then 'datanode slave'
		end as nodetype,
		nodesync
    from mgr_node
    where nodehost = (select oid from mgr_host where hostname = $1);
    $$
    LANGUAGE SQL
    IMMUTABLE
    RETURNS NULL ON NULL INPUT;

--make view for 12 hours data for  tps, qps, connectnum, dbsize, indexsize
CREATE VIEW adbmgr.monitor_12hours_tpsqps_v AS
select time, tps,qps from (SELECT monitor_databasetps_time::timestamptz(0) as time, sum(monitor_databasetps_tps) AS tps ,
  sum(monitor_databasetps_qps) AS qps  , row_number() over (PARTITION BY 1) FROM (SELECT
	distinct(monitor_databasetps_dbname),  monitor_databasetps_time, monitor_databasetps_tps,
	monitor_databasetps_qps, monitor_databasetps_runtime FROM monitor_databasetps WHERE
	monitor_databasetps_time > (SELECT now() - interval '12 hour' ) ORDER BY 2 ASC)AS a  GROUP BY
	monitor_databasetps_time) AS a;

CREATE VIEW adbmgr.monitor_12hours_connect_dbsize_indexsize_v AS
select time, connectnum, dbsize, indexsize from (SELECT  monitor_databaseitem_time::timestamptz(0) as time, sum(
	monitor_databaseitem_connectnum) AS connectnum,
	(sum(monitor_databaseitem_dbsize)/1024.0)::numeric(18,2) AS dbsize , (sum(monitor_databaseitem_indexsize)/1024.0)::
	numeric(18,2) AS indexsize, row_number() over (PARTITION BY 1) FROM (SELECT
	distinct(monitor_databaseitem_dbname),  monitor_databaseitem_time, monitor_databaseitem_connectnum,
	monitor_databaseitem_dbsize, monitor_databaseitem_indexsize FROM monitor_databaseitem WHERE monitor_databaseitem_time
	> (SELECT now() - interval '12 hour' ) ORDER BY 2 ASC) AS b  GROUP BY monitor_databaseitem_time ) AS b;

--get first page first line values
CREATE VIEW adbmgr.monitor_cluster_firstline_v
AS
	SELECT * from
	(SELECT monitor_databasetps_time::timestamptz(0) AS time, sum(monitor_databasetps_tps) AS tps , sum(monitor_databasetps_qps) AS qps,
		sum(monitor_databaseitem_connectnum) AS connectnum , (sum(monitor_databaseitem_dbsize)/1024.0)::numeric(18,2) AS dbsize, (sum(monitor_databaseitem_indexsize)/1024.0)::numeric(18,2) AS indexsize, max(monitor_databasetps_runtime) as runtime
	FROM
		(SELECT  tps.monitor_databasetps_time, tps.monitor_databasetps_tps,  tps.monitor_databasetps_qps, tps.monitor_databasetps_runtime,
					b.monitor_databaseitem_connectnum , b.monitor_databaseitem_dbsize, b.monitor_databaseitem_indexsize
				FROM (SELECT * ,(ROW_NUMBER()OVER(PARTITION BY monitor_databasetps_dbname ORDER BY
									monitor_databasetps_time desc ))AS tc
							FROM monitor_databasetps
							)AS tps
			JOIN
			(SELECT monitor_databaseitem_dbname,
					monitor_databaseitem_connectnum,  monitor_databaseitem_dbsize, monitor_databaseitem_indexsize
			 FROM (SELECT * ,(ROW_NUMBER()OVER(PARTITION BY monitor_databaseitem_dbname ORDER BY
									monitor_databaseitem_time desc ))AS tc  FROM monitor_databaseitem)AS item  WHERE item.tc =1
			) AS b
			on tps.monitor_databasetps_dbname = b.monitor_databaseitem_dbname
			WHERE   tps.tc =1
		) AS c
		GROUP BY  monitor_databasetps_time
		ORDER BY monitor_databasetps_time DESC LIMIT 1
	) AS d
	join
	( SELECT (sum(md_total/1024.0/1024)/1024)::numeric(18,2) AS md_total FROM (SELECT  hostname,md_timestamptz, md_total, (ROW_NUMBER()OVER(PARTITION BY hostname  ORDER BY  md_timestamptz desc ))AS tc   from monitor_disk) AS d WHERE tc =1
	) AS e
	on 1=1;


--make function to get tps and qps for given dbname time and time interval
CREATE OR REPLACE FUNCTION pg_catalog.monitor_databasetps_func(in text, in timestamptz, in int)
		RETURNS TABLE
	(
		recordtime timestamptz(0),
		tps bigint,
		qps bigint
	)
	AS
		$$
	SELECT monitor_databasetps_time::timestamptz(0) AS recordtime,
				monitor_databasetps_tps   AS tps,
				monitor_databasetps_qps   AS qps
	FROM
				monitor_databasetps
	WHERE monitor_databasetps_dbname = $1 and monitor_databasetps_time >= $2 and monitor_databasetps_time <= $2 + case $3
				when 0 then interval '1 hour'
				when 1 then interval '24 hour'
				end
	ORDER BY 1;
	$$
		LANGUAGE SQL
	IMMUTABLE
	RETURNS NULL ON NULL INPUT;

--make function to get tps and qps for given dbname time and start time and end time
CREATE OR REPLACE FUNCTION pg_catalog.monitor_databasetps_func_by_time_period(dbname text, starttime timestamptz, endtime timestamptz)
		RETURNS TABLE
	(
		recordtime timestamptz(0),
		tps bigint,
		qps bigint
	)
	AS $$
	SELECT monitor_databasetps_time::timestamptz(0) AS recordtime,
				monitor_databasetps_tps   AS tps,
				monitor_databasetps_qps   AS qps
	FROM
				monitor_databasetps
	WHERE monitor_databasetps_dbname = $1 and
		  monitor_databasetps_time between $2 and $3
	ORDER BY 1;
	$$
		LANGUAGE SQL
	IMMUTABLE
	RETURNS NULL ON NULL INPUT;

--to show all database tps, qps, runtime at current_time
 CREATE VIEW adbmgr.monitor_all_dbname_tps_qps_runtime_v
 AS
 SELECT distinct(monitor_databasetps_dbname) as databasename, monitor_databasetps_tps as tps, monitor_databasetps_qps as qps, monitor_databasetps_runtime as runtime FROM monitor_databasetps WHERE monitor_databasetps_time=(SELECT max(monitor_databasetps_time) FROM monitor_databasetps) ORDER BY 1 ASC;
--show cluster summary at current_time
CREATE VIEW adbmgr.monitor_cluster_summary_v
AS
SELECT (SUM(monitor_databaseitem_dbsize)/1024.0)::numeric(18,2) AS dbsize,
	CASE AVG(monitor_databaseitem_archivemode::int)
	 WHEN 0 THEN false
	 ELSE true
	 END AS archivemode,
	CASE AVG(monitor_databaseitem_autovacuum::int)
	 WHEN 0 THEN false
	 ELSE true
	 END AS autovacuum,
	AVG(monitor_databaseitem_heaphitrate)::numeric(18,2) AS heaphitrate,
	AVG(monitor_databaseitem_commitrate)::numeric(18,2) AS commitrate,
	AVG(monitor_databaseitem_dbage)::int AS dbage,
	SUM(monitor_databaseitem_connectnum) AS connectnum,
	AVG(monitor_databaseitem_standbydelay)::int AS standbydelay,
	SUM(monitor_databaseitem_locksnum) AS locksnum,
	SUM(monitor_databaseitem_longtransnum) AS longtransnum,
	SUM(monitor_databaseitem_idletransnum) AS idletransnum,
	SUM(monitor_databaseitem_preparenum) AS preparenum,
	SUM(monitor_databaseitem_unusedindexnum) AS unusedindexnum
from (SELECT * from monitor_databaseitem where monitor_databaseitem_time=(SELECT MAX(monitor_databaseitem_time) from monitor_databaseitem)) AS a;
--show database summary at current_time for given name
CREATE OR REPLACE FUNCTION pg_catalog.monitor_databasesummary_func(in name)
	RETURNS TABLE
	(
		dbsize 			numeric(18,2),
		archivemode		bool,
		autovacuum		bool,
		heaphitrate		numeric(18,2),
		commitrate		numeric(18,2),
		dbage			bigint,
		connectnum		bigint,
		standbydelay	bigint,
		locksnum		bigint,
		longtransnum	bigint,
		idletransnum	bigint,
		preparenum		bigint,
		unusedindexnum	bigint
	)
	AS
		$$
	SELECT (monitor_databaseitem_dbsize/1024.0)::numeric(18,2) AS dbsize,
			 monitor_databaseitem_archivemode AS archivemode,
			 monitor_databaseitem_autovacuum  AS autovacuum,
			 monitor_databaseitem_heaphitrate::numeric(18,2) AS heaphitrate,
			 monitor_databaseitem_commitrate::numeric(18,2)  AS commitrate,
			 monitor_databaseitem_dbage       AS dbage,
			 monitor_databaseitem_connectnum  AS connectnum,
			 monitor_databaseitem_standbydelay AS standbydelay,
			 monitor_databaseitem_locksnum     AS locksnum,
			 monitor_databaseitem_longtransnum  AS longtransum,
			 monitor_databaseitem_idletransnum  AS idletransum,
			 monitor_databaseitem_preparenum    AS  preparenum,
			 monitor_databaseitem_unusedindexnum  AS unusedindexnum
	from (SELECT * from monitor_databaseitem where monitor_databaseitem_time=(SELECT
		MAX(monitor_databaseitem_time) from monitor_databaseitem) and monitor_databaseitem_dbname = $1) AS a
	$$
		LANGUAGE SQL
	IMMUTABLE
	RETURNS NULL ON NULL INPUT;

CREATE OR REPLACE FUNCTION pg_catalog.monitor_slowlog_count_func(in name, in timestamptz, in timestamptz)
		RETURNS bigint
	AS
		$$
	SELECT count(*)
	FROM
				monitor_slowlog
	WHERE slowlogdbname = $1 and slowlogtime >= $2 and slowlogtime < $3;
	$$
		LANGUAGE SQL
	IMMUTABLE
	RETURNS NULL ON NULL INPUT;

CREATE OR REPLACE FUNCTION pg_catalog.monitor_slowlog_func_page(in name, in timestamptz, in timestamptz, in int, in int)
		RETURNS TABLE
		(
			query text,
			dbuser name,
			singletime float4,
			totalnum int,
			queryplan text
		)
	AS
		$$
	SELECT slowlogquery AS query,
				slowloguser   AS dbuser,
				slowlogsingletime AS singletime,
				slowlogtotalnum   AS totalnum,
				slowlogqueryplan  AS queryplan
	FROM
				monitor_slowlog
	WHERE slowlogdbname = $1 and slowlogtime >= $2 and slowlogtime < $3 order by slowlogtime desc limit $4 offset $5;
	$$
		LANGUAGE SQL
	IMMUTABLE
	RETURNS NULL ON NULL INPUT;
--for cluster warning, monitor_dbthreshold systbl, see "typedef enum DbthresholdObject"
--  in monitor_dbthreshold.c
--heaphitrate
insert into pg_catalog.monitor_host_threshold values(11, 0, 98, 95, 90);
insert into pg_catalog.monitor_host_threshold values(21, 0, 98, 95, 90);
--commitrate
insert into pg_catalog.monitor_host_threshold values(12, 0, 95, 90, 85);
insert into pg_catalog.monitor_host_threshold values(22, 0, 95, 90, 85);
--standbydelay
insert into pg_catalog.monitor_host_threshold values(13, 1, 200, 250, 300);
insert into pg_catalog.monitor_host_threshold values(23, 1, 1000, 2000, 3000);
--locks num
insert into pg_catalog.monitor_host_threshold values(14, 1, 40, 50, 60);
insert into pg_catalog.monitor_host_threshold values(24, 1, 50, 80, 120);
--connect num
insert into pg_catalog.monitor_host_threshold values(15, 1, 400, 500, 800);
insert into pg_catalog.monitor_host_threshold values(25, 1, 500, 800, 1500);
--long transaction, idle transaction
insert into pg_catalog.monitor_host_threshold values(16, 1, 25, 30, 50);
insert into pg_catalog.monitor_host_threshold values(26, 1, 30, 60, 80);
--unused index num
insert into pg_catalog.monitor_host_threshold values(17, 1, 0, 0, 0);
insert into pg_catalog.monitor_host_threshold values(27, 1, 100, 150, 200);
--tps timeinterval
insert into pg_catalog.monitor_host_threshold values(31, 1, 3, 0, 0);
--long transactions min time
insert into pg_catalog.monitor_host_threshold values(32, 1, 100, 0, 0);
--slow query min time
insert into pg_catalog.monitor_host_threshold values(33, 1, 2, 0, 0);
--get limit num slowlog from database cluster once time
insert into pg_catalog.monitor_host_threshold values(34, 1, 100, 0, 0);

-- for ADB monitor the topology in home page : get datanode node topology
CREATE VIEW adbmgr.get_datanode_node_topology AS
    select '{'|| ARRAY_TO_STRING || '}' as datanode_result
    from (
    select ARRAY_TO_STRING(
                            array(
                                    select case f.nodetype
                                           when 'd' then '"master"'
                                           when 'b' then '"slave"'
                                           end
                                           || ':' || '{' || '"node_name"' || ':' || '"' || f.nodename || '"' || ','
                                                         || '"node_port"' || ':' ||        f.nodeport        || ','
                                                         || '"node_ip"'   || ':' || '"' || f.hostaddr || '"' || ','
                                                         || '"sync_state"'|| ':' || '"' || f.nodesync || '"' ||
                                                     '}'
                                    from(
                                            select n.nodename,n.oid,n.nodetype,n.nodesync,n.nodeport,n.nodemasternameoid, h.hostaddr
                                            from mgr_node n, mgr_host h
                                            where n.nodemasternameoid = '0' and
                                                  n.nodename = x.nodename  and
                                                  n.nodetype = 'd' and
                                                  h.oid = n.nodehost and
                                                  n.nodeincluster = true and
                                                  n.nodeinited = true

                                            union all

                                            select t2.nodename,t2.oid,t2.nodetype,t2.nodesync,t2.nodeport,t2.nodemasternameoid,t2.hostaddr
                                            from (
                                                    select n.nodename,n.oid,n.nodetype,n.nodeport,n.nodemasternameoid,h.hostaddr
                                                    from mgr_node n,mgr_host h
                                                    where n.nodemasternameoid = '0' and
                                                          n.nodename = x.nodename and
                                                          n.nodetype = 'd' and
                                                          h.oid = n.nodehost and
                                                          n.nodeincluster = true and
                                                          n.nodeinited = true
                                                ) t1
                                                left join
                                                (
                                                    select n.nodename,n.oid,n.nodetype,n.nodesync,n.nodeport,n.nodemasternameoid,h.hostaddr
                                                    from mgr_node n,mgr_host h
                                                    where h.oid = n.nodehost and
                                                          n.nodeincluster = true and
                                                          n.nodeinited = true
                                                ) t2
                                                on t1.oid = t2.nodemasternameoid and t2.nodetype in ('b','n')
                                        ) as f
                                ), ','
                        ) from (select nodename from mgr_node where nodetype = 'd') x
        ) r;

-- for ADB monitor the topology in home page : get coordinator node topology
CREATE VIEW adbmgr.get_coordinator_node_topology AS
    select n.nodename AS node_name,
        n.nodeport AS node_port,
        h.hostaddr AS node_ip
    from mgr_node n, mgr_host h
    where n.nodeincluster = true and
        n.nodeinited = true and
        n.nodehost = h.oid and
        n.nodetype = 'c';

-- for ADB monitor the topology in home page : get agtm node topology
CREATE VIEW adbmgr.get_agtm_node_topology AS
    select '{'|| ARRAY_TO_STRING || '}' as agtm_result
    from(
    select ARRAY_TO_STRING(
                            array(
                                    select case f.nodetype
                                        when 'g' then '"master"'
                                        when 'p' then '"slave"'
                                        end
                                        || ':' || '{' || '"node_name"' || ':' || '"' || f.nodename || '"' || ','
                                                      || '"node_port"' || ':' ||        f.nodeport        || ','
                                                      || '"node_ip"'   || ':' || '"' || f.hostaddr || '"' || ','
                                                      || '"sync_state"'|| ':' || '"' || f.nodesync || '"' ||
                                                '}'
                                    from(
                                        select n.nodename,n.oid,n.nodetype,n.nodesync,n.nodeport,n.nodemasternameoid, h.hostaddr
                                        from mgr_node n, mgr_host h
                                        where n.nodeincluster = true and
                                              n.nodeinited = true and
                                              n.nodehost = h.oid and
                                              n.nodetype in ('g', 'p', 'e')
                                        ) f
                                ) -- end array
                            , ','
                        ) -- end ARRAY_TO_STRING
        ) r;

-- insert default values into monitor_host_threthold.
insert into pg_catalog.monitor_host_threshold values (1, 1, 90, 95, 99);
insert into pg_catalog.monitor_host_threshold values (2, 1, 85, 90, 95);
insert into pg_catalog.monitor_host_threshold values (3, 1, 80, 85, 90);
insert into pg_catalog.monitor_host_threshold values (4, 1, 2000, 3000, 4000);
insert into pg_catalog.monitor_host_threshold values (5, 1, 2000, 3000, 4000);
insert into pg_catalog.monitor_host_threshold values (6, 1, 3000, 4000, 5000);

-- update threshold value by type
create or replace function pg_catalog.update_threshold_value(type int, value1 int, value2 int, value3 int)
returns  void
as $$
update pg_catalog.monitor_host_threshold
set mt_warning_threshold = $2, mt_critical_threshold = $3, mt_emergency_threshold = $4
where mt_type = $1;
$$ language sql
VOLATILE
returns null on null input;

--get the threshold for specific type
create or replace function pg_catalog.get_threshold_type(type int)
returns text
as $$
    select '{' || row_string || '}' as show_threshold
    from (
           select case $1
                  when 1 then '"cpu_usage"'
                  when 2 then '"mem_usage"'
                  when 3 then '"disk_usage"'
                  when 4 then '"sent_net"'
                  when 5 then '"recv_net"'
                  when 6 then '"IOPS"'
                  when 11 then '"node_heaphit_rate"'
                  when 21 then '"cluster_heaphit_rate"'
                  when 12 then '"node_commit/rollback_rate"'
                  when 22 then '"cluster_commit/rollback_rate"'
                  when 13 then '"node_standy_delay"'
                  when 23 then '"cluster_standy_delay"'
                  when 14 then '"node_wait_locks"'
                  when 24 then '"cluster_wait_locks"'
                  when 15 then '"node_connect"'
                  when 25 then '"cluster_connect"'
                  when 16 then '"node_longtrnas/idletrans"'
                  when 26 then '"cluster_longtrnas/idletrans"'
                  when 27 then '"cluster_unused_indexs"'
                  END
                  || ':' || row_to_json as row_string
           from (

                   select row_to_json(t)
                   from (
                           select
                               mt_warning_threshold AS "warning",
                               mt_critical_threshold AS "critical",
                               mt_emergency_threshold AS "emergency"
                           from monitor_host_threshold
                           where mt_type = $1
                        ) t
               )y
         )s;
    $$
language sql
IMMUTABLE
RETURNS NULL ON NULL INPUT;

--get the threshlod for all type
CREATE VIEW adbmgr.get_threshold_all_type AS
select mt_type as type, mt_direction as direction, mt_warning_threshold as warning, mt_critical_threshold as critical, mt_emergency_threshold as emergency
  from pg_catalog.monitor_host_threshold
	where mt_type in (1,2,3,4,5,6)
	   order by 1 asc;

--get the threshlod for all type
CREATE VIEW adbmgr.get_db_threshold_all_type
as
 select tt1.mt_type as type, tt1.mt_direction as direction, tt1.mt_warning_threshold as node_warning, tt1.mt_critical_threshold as node_critical,
			  tt1.mt_emergency_threshold as node_emergency,tt2.mt_warning_threshold as cluster_warning,
				tt2.mt_critical_threshold as cluster_critical, tt2.mt_emergency_threshold as cluster_emergency
from (select * from pg_catalog.monitor_host_threshold where mt_type in (11,12,13,14,15,16,17))as tt1
   join  (select * from pg_catalog.monitor_host_threshold where mt_type in (21,22,23,24,25,26,27)) tt2 on tt1.mt_type +10 =tt2.mt_type
	 order by 1 asc;

-- get all alarm info (host and DB)
create or replace FUNCTION pg_catalog.get_alarm_info_asc(starttime timestamp ,
                                                        endtime timestamp ,
                                                        search_text text default '',
                                                        alarm_level int default 0,
                                                        alarm_type int default 0,
                                                        alarm_status int default 0,
                                                        alarm_limit int default 20,
                                                        alarm_offset int default 0)
returns TABLE (
                alarm_level smallint,
                alarm_text text,
                alarm_type smallint,
                alarm_id oid,
                alarm_source text,
                alarm_time timestamptz,
                alarm_status smallint,
                alarm_resolve_timetz timestamptz,
                alarm_solution text)
as $$
select a.ma_alarm_level AS alarm_level,
       a.ma_alarm_text AS alarm_text,
       a.ma_alarm_type AS alarm_type,
       a.oid AS alarm_id,
       a.ma_alarm_source AS alarm_source,
       a.ma_alarm_timetz AS alarm_time,
       a.ma_alarm_status AS alarm_status,
       r.mr_resolve_timetz AS alarm_resolve_timetz,
       r.mr_solution AS alarm_solution
from monitor_alarm a left join monitor_resolve r on(a.oid = r.mr_alarm_oid)
where case $4 when 0 then 1::boolean else a.ma_alarm_level = $4 end and
      case $5 when 0 then 1::boolean else a.ma_alarm_type = $5 end and
      case $6 when 0 then 1::boolean else a.ma_alarm_status = $6 end and
      a.ma_alarm_text ~ $3 and
      a.ma_alarm_timetz between $1 and $2
    order by a.ma_alarm_timetz asc limit $7 offset $8

$$ LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

-- get all alarm info (host and DB)
create or replace FUNCTION pg_catalog.get_alarm_info_desc(starttime timestamp ,
                                                        endtime timestamp ,
                                                        search_text text default '',
                                                        alarm_level int default 0,
                                                        alarm_type int default 0,
                                                        alarm_status int default 0,
                                                        alarm_limit int default 20,
                                                        alarm_offset int default 0)
returns TABLE (
                alarm_level smallint,
                alarm_text text,
                alarm_type smallint,
                alarm_id oid,
                alarm_source text,
                alarm_time timestamptz,
                alarm_status smallint,
                alarm_resolve_timetz timestamptz,
                alarm_solution text)
as $$
select a.ma_alarm_level AS alarm_level,
       a.ma_alarm_text AS alarm_text,
       a.ma_alarm_type AS alarm_type,
       a.oid AS alarm_id,
       a.ma_alarm_source AS alarm_source,
       a.ma_alarm_timetz AS alarm_time,
       a.ma_alarm_status AS alarm_status,
       r.mr_resolve_timetz AS alarm_resolve_timetz,
       r.mr_solution AS alarm_solution
from monitor_alarm a left join monitor_resolve r on(a.oid = r.mr_alarm_oid)
where case $4 when 0 then 1::boolean else a.ma_alarm_level = $4 end and
      case $5 when 0 then 1::boolean else a.ma_alarm_type = $5 end and
      case $6 when 0 then 1::boolean else a.ma_alarm_status = $6 end and
      a.ma_alarm_text ~ $3 and
      a.ma_alarm_timetz between $1 and $2
    order by a.ma_alarm_timetz desc limit $7 offset $8

$$ LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

create or replace FUNCTION pg_catalog.get_alarm_info_count(starttime timestamp ,
                                                        endtime timestamp ,
                                                        search_text text default '',
                                                        alarm_level int default 0,
                                                        alarm_type int default 0,
                                                        alarm_status int default 0)
returns bigint
as $$
select count(*)
from monitor_alarm a left join monitor_resolve r on(a.oid = r.mr_alarm_oid)
where case $4 when 0 then 1::boolean else a.ma_alarm_level = $4 end and
      case $5 when 0 then 1::boolean else a.ma_alarm_type = $5 end and
      case $6 when 0 then 1::boolean else a.ma_alarm_status = $6 end and
      a.ma_alarm_text ~ $3 and
      a.ma_alarm_timetz between $1 and $2
$$ LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

-- save resolve alarm log
create or replace function pg_catalog.resolve_alarm(alarm_id int, resolve_time timestamp, resolve_text text)
returns void as
$$
    insert into monitor_resolve (mr_alarm_oid, mr_resolve_timetz, mr_solution)
    values ($1, $2, $3);
    update monitor_alarm set ma_alarm_status = 2 where oideq(oid,$1)
$$
LANGUAGE SQL
VOLATILE
RETURNS NULL ON NULL INPUT;

--insert into monitor_user, as default value: 1: ordinary users, 2: db manager
insert into monitor_user values('adbmonitor', 2, '2016-01-01','2050-01-01', '12345678901',
'userdba@asiainfo.com', '亚信', '数据库', '数据库研发工程师', 'ISMvKXpXpadDiUoOSoAfww==','系统管理员');
--check user name/password
CREATE OR REPLACE  FUNCTION  pg_catalog.monitor_checkuser_func(in Name, in Name)
RETURNS oid AS $$
  select oid from pg_catalog.monitor_user where username=$1 and userpassword=$2;
$$  LANGUAGE sql IMMUTABLE STRICT;

--show user info
 create or replace function pg_catalog.monitor_getuserinfo_func(in int)
returns TABLE
	(
		username			Name,				/*the user name*/
		usertype			int,
		userstarttime		timestamptz,
		userendtime			timestamptz,
		usertel				Name,
		useremail			Name,
		usercompany			Name,
		userdepart			Name,
		usertitle			Name,
		userdesc			text
	)
as
$$
    select username, usertype, userstarttime, userendtime, usertel, useremail
		, usercompany, userdepart, usertitle, userdesc from pg_catalog.monitor_user where oid=$1;
$$
LANGUAGE SQL
IMMUTABLE
RETURNS NULL ON NULL INPUT;

--update user info
create or replace function pg_catalog.monitor_updateuserinfo_func(in int, in Name, in Name, in Name, in Name, in Name, in text)
returns int
as
$$
    update pg_catalog.monitor_user set username=$2, usertel=$3, useremail=$4, usertitle=$5, usercompany=$6, userdesc=$7 where oid=$1 returning 0;
$$
LANGUAGE SQL
VOLATILE
RETURNS NULL ON NULL INPUT;

--check user oldpassword
create or replace function pg_catalog.monitor_checkuserpassword_func(in int, in Name)
returns bigint
as
$$
  select count(*) -1 from pg_catalog.monitor_user where oid=$1 and userpassword=$2;
$$
LANGUAGE SQL
VOLATILE
RETURNS NULL ON NULL INPUT;

--update user password
create or replace function  pg_catalog.monitor_updateuserpassword_func(in int, in Name)
returns int
as
$$
    update pg_catalog.monitor_user set userpassword=$2 where oid=$1  returning 0;
$$
LANGUAGE SQL
VOLATILE
RETURNS NULL ON NULL INPUT;

--for ADB manager command : grant and revoke
grant usage on schema adbmgr to public;

-- clean
revoke execute on function mgr_clean_all() from public;
revoke execute on function mgr_clean_node("any") from public;
revoke execute on function monitor_delete_data_interval_days(int) from public;
-- failover
revoke execute on function mgr_failover_gtm(cstring, bool), mgr_failover_one_dn(cstring, bool) from public;

-- show
revoke execute on function mgr_show_var_param( "any") from public;

-- append
revoke execute on function
mgr_append_dnmaster(cstring),
mgr_append_dnslave(cstring),
mgr_append_coordmaster(cstring),
mgr_append_agtmslave(cstring),
mgr_append_coord_to_coord(cstring,cstring),
mgr_append_activate_coord(cstring)
from public;

-- monitor
revoke execute on function
mgr_monitor_agent_all(),
mgr_monitor_agent_hostlist(text[]),
mgr_monitor_gtm_all(),
mgr_monitor_datanode_all(),
mgr_monitor_nodetype_namelist(bigint, "any"),
mgr_monitor_nodetype_all(bigint),
mgr_monitor_ha()
from public;

--switchover
revoke execute on function
mgr_switchover_func(int, cstring, int)
from public;


revoke execute on function mgr_priv_manage(bigint,text[],text[]) from public;
revoke execute on function mgr_list_acl_all()from public;
revoke execute on function mgr_priv_list_to_all(bigint,text[]) from public;
revoke execute on function mgr_priv_all_to_username(bigint,text[]) from public;

--list
revoke execute on function mgr_list_hba_by_name("any") from public;

--start
revoke execute on function
mgr_start_agent_all(cstring),
mgr_start_agent_hostnamelist(cstring,text[]),
mgr_start_gtm_master("any"),
mgr_start_gtm_slave("any"),
mgr_start_cn_master("any"),
mgr_start_dn_master("any"),
mgr_start_dn_slave("any")
from public;

--add
revoke execute on function
mgr_add_host_func(boolean,cstring,"any"),
mgr_add_node_func(boolean,"char",cstring,cstring,"any")
from public;

--drop
revoke execute on function
mgr_drop_host_func(boolean,"any"),
mgr_drop_node_func("char","any")
from public;

--alter
revoke execute on function
mgr_alter_host_func(boolean, cstring, "any"),
mgr_alter_node_func("char", cstring, "any")
from public;

--set
revoke execute on function
mgr_add_updateparm_func("char", cstring, "char", boolean, "any"),
mgr_set_init_cluster()
from public;

--reset
revoke execute on function
mgr_reset_updateparm_func("char", cstring, "char", boolean, "any")
from public;

--deploy
revoke execute on function
mgr_deploy_all(cstring),
mgr_deploy_hostnamelist(cstring, text[])
from public;

--stop
revoke execute on function
mgr_stop_agent_all(),
mgr_stop_agent_hostnamelist(text[]),
mgr_stop_gtm_master("any"),
mgr_stop_gtm_slave("any"),
mgr_stop_cn_master("any"),
mgr_stop_dn_master("any"),
mgr_stop_dn_slave("any")
from public;

--flush host
revoke execute on function mgr_flush_host() from public;

--add primary key for table:monitor_host_threshold,monitor_user and monitor_job
alter table pg_catalog.monitor_host_threshold add primary key (mt_type);
alter table pg_catalog.monitor_user add primary key (username);
alter table pg_catalog.monitor_job add primary key (name);

--expand
revoke execute on function mgr_expand_dnmaster(cstring, cstring) from public;
revoke execute on function mgr_expand_activate_dnmaster(cstring) from public;

--expand
revoke execute on function
mgr_import_hash_meta(cstring),
mgr_cluster_pgxcnode_init(),
mgr_cluster_slot_init_func(boolean, cstring, "any"),
mgr_expand_show_status(),
mgr_expand_check_status(),
mgr_cluster_pgxcnode_check(),
mgr_cluster_hash_meta_check(),
mgr_checkout_dnslave_status(),
mgr_expand_dnmaster(cstring, cstring),
mgr_expand_recover_backup_fail(cstring, cstring),
mgr_expand_recover_backup_suc(cstring, cstring),
mgr_expand_activate_dnmaster(cstring),
mgr_expand_activate_recover_promote_suc(cstring),
mgr_expand_clean_init(),
mgr_expand_clean_start(),
mgr_expand_clean_end()
from public;
INSERT INTO adbmgr.parm VALUES ('#', 'slot_database_name', 'postgres', 'user', 'string', NULL, NULL, NULL, NULL);

