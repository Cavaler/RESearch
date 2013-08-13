@echo off
perl MigrateFar2.pl >Migrate.sql
sqlite3 F250C12A-78E2-4ABC-A784-3FDD3156E415.db <Migrate.sql
