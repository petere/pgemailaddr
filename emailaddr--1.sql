SET client_min_messages = warning;


CREATE TYPE emailaddr;

CREATE FUNCTION emailaddr_in(cstring) RETURNS emailaddr
    IMMUTABLE
    STRICT
    LANGUAGE C
    AS '$libdir/emailaddr';

CREATE FUNCTION emailaddr_out(emailaddr) RETURNS cstring
    IMMUTABLE
    STRICT
    LANGUAGE C
    AS '$libdir/emailaddr';

CREATE FUNCTION emailaddr_user(emailaddr) RETURNS cstring
    IMMUTABLE
    STRICT
    LANGUAGE C
    AS '$libdir/emailaddr';

CREATE FUNCTION emailaddr_host(emailaddr) RETURNS cstring
    IMMUTABLE
    STRICT
    LANGUAGE C
    AS '$libdir/emailaddr';

CREATE TYPE emailaddr (
    INTERNALLENGTH = -1,
    INPUT = emailaddr_in,
    OUTPUT = emailaddr_out
);


CREATE CAST (emailaddr AS text) WITH INOUT AS ASSIGNMENT;
CREATE CAST (text AS emailaddr) WITH INOUT AS ASSIGNMENT;


CREATE FUNCTION emailaddr_lt(emailaddr, emailaddr) RETURNS boolean
    IMMUTABLE
    STRICT
    LANGUAGE C
    AS '$libdir/emailaddr';

CREATE FUNCTION emailaddr_le(emailaddr, emailaddr) RETURNS boolean
    IMMUTABLE
    STRICT
    LANGUAGE C
    AS '$libdir/emailaddr';

CREATE FUNCTION emailaddr_eq(emailaddr, emailaddr) RETURNS boolean
    IMMUTABLE
    STRICT
    LANGUAGE C
    AS '$libdir/emailaddr';

CREATE FUNCTION emailaddr_ne(emailaddr, emailaddr) RETURNS boolean
    IMMUTABLE
    STRICT
    LANGUAGE C
    AS '$libdir/emailaddr';

CREATE FUNCTION emailaddr_ge(emailaddr, emailaddr) RETURNS boolean
    IMMUTABLE
    STRICT
    LANGUAGE C
    AS '$libdir/emailaddr';

CREATE FUNCTION emailaddr_gt(emailaddr, emailaddr) RETURNS boolean
    IMMUTABLE
    STRICT
    LANGUAGE C
    AS '$libdir/emailaddr';

CREATE FUNCTION emailaddr_cmp(emailaddr, emailaddr) RETURNS integer
    IMMUTABLE
    STRICT
    LANGUAGE C
    AS '$libdir/emailaddr';

CREATE OPERATOR < (
    LEFTARG = emailaddr,
    RIGHTARG = emailaddr,
    COMMUTATOR = >,
    NEGATOR = >=,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel,
    PROCEDURE = emailaddr_lt
);

CREATE OPERATOR <= (
    LEFTARG = emailaddr,
    RIGHTARG = emailaddr,
    COMMUTATOR = >=,
    NEGATOR = >,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel,
    PROCEDURE = emailaddr_le
);

CREATE OPERATOR = (
    LEFTARG = emailaddr,
    RIGHTARG = emailaddr,
    COMMUTATOR = =,
    NEGATOR = <>,
    RESTRICT = eqsel,
    JOIN = eqjoinsel,
    HASHES,
    MERGES,
    PROCEDURE = emailaddr_eq
);

CREATE OPERATOR <> (
    LEFTARG = emailaddr,
    RIGHTARG = emailaddr,
    COMMUTATOR = <>,
    NEGATOR = =,
    RESTRICT = neqsel,
    JOIN = neqjoinsel,
    PROCEDURE = emailaddr_ne
);

CREATE OPERATOR >= (
    LEFTARG = emailaddr,
    RIGHTARG = emailaddr,
    COMMUTATOR = <=,
    NEGATOR = <,
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel,
    PROCEDURE = emailaddr_ge
);

CREATE OPERATOR > (
    LEFTARG = emailaddr,
    RIGHTARG = emailaddr,
    COMMUTATOR = <,
    NEGATOR = <=,
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel,
    PROCEDURE = emailaddr_gt
);

CREATE OPERATOR CLASS emailaddr_ops
    DEFAULT FOR TYPE emailaddr USING btree AS
        OPERATOR        1       < ,
        OPERATOR        2       <= ,
        OPERATOR        3       = ,
        OPERATOR        4       >= ,
        OPERATOR        5       > ,
        FUNCTION        1       emailaddr_cmp(emailaddr, emailaddr);
