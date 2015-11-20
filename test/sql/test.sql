\pset null _null_

SET client_min_messages = warning;

CREATE TABLE test (a serial, b emailaddr);

INSERT INTO test (b)
VALUES ('peter@eisentraut.org'),
       ('pgsql-hackers@postgresql.org'),
       ('pgsql-docs@postgresql.org'),
       ('root@[127.0.0.1]');

SELECT * FROM test ORDER BY a ASC;

SELECT * FROM test ORDER BY b ASC;

-- error cases
SELECT emailaddr 'foo';
SELECT emailaddr 'foo,,@bar.org';
SELECT emailaddr 'foo@[123';
