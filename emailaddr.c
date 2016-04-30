#include <postgres.h>
#include <fmgr.h>
#include <utils/builtins.h>


PG_MODULE_MAGIC;


/*
 * Internal storage format for email addresses: The data field
 * contains the domain followed by the local part.  (The '@' is not
 * stored.)  The local_offset field records where the local part
 * starts.  So for example, 'pgsql-hackers@postgresql.org' is stored
 * as { 14, 'postgresql.orgpgsql-hackers' }.
 *
 * This takes the same amount of space as the original but gives us a
 * preparsed form that is easier for sorting and processing.  The
 * local_offset only needs to be 1 byte because domain names are
 * restricted to 255 bytes in length (RFC 1034, RFC 1035, RFC 1123).
 */
typedef struct emailadr {
	char	vl_len_[4];
	uint8	local_offset;
	char	data[FLEXIBLE_ARRAY_MEMBER];
} emailaddr;


#define DatumGetEmailAddrP(X)   ((emailaddr *) PG_DETOAST_DATUM(X))
#define DatumGetEmailAddrPP(X)  ((emailaddr *) PG_DETOAST_DATUM_PACKED(X))
#define EmailAddrPGetDatum(X)   PointerGetDatum(X)

#define PG_GETARG_EMAILADDR_P(n)  DatumGetEmailAddrP(PG_GETARG_DATUM(n))
#define PG_GETARG_EMAILADDR_PP(n) DatumGetEmailAddrPP(PG_GETARG_DATUM(n))
#define PG_RETURN_EMAILADDR_P(x)  PG_RETURN_POINTER(x)


static bool
is_atext(char c)
{
	return ((c >= 'a' && c <= 'z')
			|| (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9')
			|| c == '!' || c == '#'
			|| c == '$' || c == '%'
			|| c == '&' || c == '\''
			|| c == '*' || c == '+'
			|| c == '-' || c == '/'
			|| c == '=' || c == '?'
			|| c == '^' || c == '_'
			|| c == '`' || c == '{'
			|| c == '|' || c == '}'
			|| c == '~'
		);
}

static void
parse_dot_atom(const char *s, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		if (!is_atext(s[i]) && s[i] != '.')
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type emailaddr: invalid character \"%c\"", s[i])));
}

static void
parse_domain_literal(const char *s, size_t len)
{
	size_t i;

	for (i = 1; i < len - 1; i++)
		switch (s[i])
		{
			case '[':
			case ']':
			case '\\':
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type emailaddr: invalid character \"%c\"", s[i])));
		}

	if (s[len - 1] != ']')
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
			 errmsg("invalid input syntax for type emailaddr: missing matching \"]\"")));
}

PG_FUNCTION_INFO_V1(emailaddr_in);
Datum
emailaddr_in(PG_FUNCTION_ARGS)
{
	char *s = PG_GETARG_CSTRING(0);
	emailaddr *result;
	char *p;
	size_t len;
	int32 result_len;
	size_t domain_len;

	p = strchr(s, '@');
	if (!p)
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
			 errmsg("invalid input syntax for type emailaddr: missing \"@\"")));

	len =  strlen(s);
	domain_len = len - (p - s) - 1;
	if (domain_len > 255)
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
			 errmsg("invalid input syntax for type emailaddr: domain too long")));

	parse_dot_atom(s, (p - s));
	if (*(p + 1) == '[')
		parse_domain_literal(p + 1, domain_len);
	else
		parse_dot_atom(p + 1, domain_len);

	result_len = offsetof(emailaddr, data) + len - 1;
	result = palloc(result_len);
	SET_VARSIZE(result, result_len);
	result->local_offset = domain_len;
	memcpy(result->data, p + 1, domain_len);
	memcpy(result->data + domain_len, s, (p - s));

	PG_RETURN_EMAILADDR_P(result);
}

PG_FUNCTION_INFO_V1(emailaddr_out);
Datum
emailaddr_out(PG_FUNCTION_ARGS)
{
	emailaddr *arg = PG_GETARG_EMAILADDR_P(0);
	char *result;
	size_t result_len;
	size_t local_len;

	result_len = VARSIZE(arg) - offsetof(emailaddr, data) + 1 + 1;
	result = palloc(result_len);
	local_len = VARSIZE(arg) - offsetof(emailaddr, data) - arg->local_offset;
	memcpy(result, arg->data + arg->local_offset, local_len);
	result[local_len] = '@';
	memcpy(result + local_len + 1, arg->data, arg->local_offset);
	result[result_len - 1] = '\0';

	PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(emailaddr_user);
Datum
emailaddr_user(PG_FUNCTION_ARGS)
{
	emailaddr *arg = PG_GETARG_EMAILADDR_P(0);
	char *result;
	size_t result_len;
	size_t local_len;

	result_len = VARSIZE(arg) - offsetof(emailaddr, data) - arg->local_offset + 1;
	result = palloc(result_len);

	local_len = VARSIZE(arg) - offsetof(emailaddr, data) - arg->local_offset;
	memcpy(result, arg->data + arg->local_offset, local_len);
  result[result_len - 1] = '\0'; // artisinally terminated strings

	PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(emailaddr_host);
Datum
emailaddr_host(PG_FUNCTION_ARGS)
{
	emailaddr *arg = PG_GETARG_EMAILADDR_P(0);
	char *result;
	size_t result_len;
	size_t local_len;

	result_len = arg->local_offset + 1;
	result = palloc(result_len);

	memcpy(result, arg->data, arg->local_offset);
  result[result_len - 1] = '\0'; // artisinally terminated strings

	PG_RETURN_CSTRING(result);
}

static int
strnncmp(const char *s1, size_t n1, const char *s2, size_t n2)
{
	int res;

	res = strncmp(s1, s2, Min(n1, n2));
	if (res != 0)
		return res;

	res = n1 - n2;
	return res;
}

static int
_emailaddr_cmp(emailaddr *a, emailaddr *b)
{
	int res;

	res = strnncmp(a->data, a->local_offset, b->data, b->local_offset);
	if (res != 0)
		return res;

	res = strnncmp(a->data + a->local_offset, VARSIZE(a) - offsetof(emailaddr, data) - a->local_offset,
				   b->data + b->local_offset, VARSIZE(b) - offsetof(emailaddr, data) - b->local_offset);

	return res;
}

PG_FUNCTION_INFO_V1(emailaddr_lt);
Datum
emailaddr_lt(PG_FUNCTION_ARGS)
{
	emailaddr *arg1 = PG_GETARG_EMAILADDR_PP(0);
	emailaddr *arg2 = PG_GETARG_EMAILADDR_PP(1);

	PG_RETURN_BOOL(_emailaddr_cmp(arg1, arg2) < 0);
}

PG_FUNCTION_INFO_V1(emailaddr_le);
Datum
emailaddr_le(PG_FUNCTION_ARGS)
{
	emailaddr *arg1 = PG_GETARG_EMAILADDR_PP(0);
	emailaddr *arg2 = PG_GETARG_EMAILADDR_PP(1);

	PG_RETURN_BOOL(_emailaddr_cmp(arg1, arg2) <= 0);
}

PG_FUNCTION_INFO_V1(emailaddr_eq);
Datum
emailaddr_eq(PG_FUNCTION_ARGS)
{
	emailaddr *arg1 = PG_GETARG_EMAILADDR_PP(0);
	emailaddr *arg2 = PG_GETARG_EMAILADDR_PP(1);

	PG_RETURN_BOOL(_emailaddr_cmp(arg1, arg2) == 0);
}

PG_FUNCTION_INFO_V1(emailaddr_ne);
Datum
emailaddr_ne(PG_FUNCTION_ARGS)
{
	emailaddr *arg1 = PG_GETARG_EMAILADDR_PP(0);
	emailaddr *arg2 = PG_GETARG_EMAILADDR_PP(1);

	PG_RETURN_BOOL(_emailaddr_cmp(arg1, arg2) != 0);
}

PG_FUNCTION_INFO_V1(emailaddr_ge);
Datum
emailaddr_ge(PG_FUNCTION_ARGS)
{
	emailaddr *arg1 = PG_GETARG_EMAILADDR_PP(0);
	emailaddr *arg2 = PG_GETARG_EMAILADDR_PP(1);

	PG_RETURN_BOOL(_emailaddr_cmp(arg1, arg2) >= 0);
}

PG_FUNCTION_INFO_V1(emailaddr_gt);
Datum
emailaddr_gt(PG_FUNCTION_ARGS)
{
	emailaddr *arg1 = PG_GETARG_EMAILADDR_PP(0);
	emailaddr *arg2 = PG_GETARG_EMAILADDR_PP(1);

	PG_RETURN_BOOL(_emailaddr_cmp(arg1, arg2) > 0);
}

PG_FUNCTION_INFO_V1(emailaddr_cmp);
Datum
emailaddr_cmp(PG_FUNCTION_ARGS)
{
	emailaddr *arg1 = PG_GETARG_EMAILADDR_PP(0);
	emailaddr *arg2 = PG_GETARG_EMAILADDR_PP(1);

	PG_RETURN_INT32(_emailaddr_cmp(arg1, arg2));
}
