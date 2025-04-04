PG_CONFIG = pg_config
PKG_CONFIG = pkg-config

EXTENSION = emailaddr
MODULE_big = emailaddr
OBJS = emailaddr.o
DATA = emailaddr--1.sql

REGRESS = init test
REGRESS_OPTS = --inputdir=test

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
