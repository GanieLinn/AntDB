CREATE EXTENSION adb_doctor;

--
-- These tests don't produce any interesting output.  We're checking that
-- the operations complete without crashing or hanging and that none of their
-- internal sanity tests fail.
--
SELECT adb_doctor_start();
SELECT adb_doctor_stop();
