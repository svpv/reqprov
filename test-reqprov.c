#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <rpmlib.h>
#include "reqprov.h"

static struct ReqComp {
    const char *token;
    int sense;
} ReqComparisons[] = {
    { "<=", RPMSENSE_LESS | RPMSENSE_EQUAL},
    { "=<", RPMSENSE_LESS | RPMSENSE_EQUAL},
    { "<", RPMSENSE_LESS},
    { "==", RPMSENSE_EQUAL},
    { "=", RPMSENSE_EQUAL},
    { ">=", RPMSENSE_GREATER | RPMSENSE_EQUAL},
    { "=>", RPMSENSE_GREATER | RPMSENSE_EQUAL},
    { ">", RPMSENSE_GREATER},
    { NULL, 0 },
};

static int test(int tag, const char *tagname, int line,
	const char *cmp1, const char *evr1,
	const char *cmp2, const char *evr2,
	int expect)
{
    int sense1 = 0;
    int sense2 = 0;
    struct ReqComp *p;
    for (p = ReqComparisons; p->token; p++) {
	if (strcmp(p->token, cmp1) == 0)
	    sense1 = p->sense;
	if (strcmp(p->token, cmp2) == 0)
	    sense2 = p->sense;
    }
    int rc = compare_version_requirement(tag, evr1, sense1, evr2, sense2);
    if (rc == expect)
	return 0;
    fprintf(stderr,
	    "Test failed for %s (line %d)\n"
	    "    Foo %s %s\n"
	    "    Foo %s %s\n"
	    "	expected %d got %d\n",
		tagname, line,
		cmp1, evr1,
		cmp2, evr2,
		expect, rc);
    return 1;
}

#define Test(tag, args...) test(tag, #tag, __LINE__, ##args)
#define Requires RPMTAG_REQUIREVERSION
#define Provides RPMTAG_PROVIDEVERSION
#define Foo

int main()
{
    int rc = 0;
    /* common Requires tests */
    rc |= Test(Requires,
	    Foo "", "",
	    Foo ">=", "1.0",
	    -1);
    rc |= Test(Requires,
	    Foo ">", "2.0",
	    Foo ">=", "1.0",
	    1);
    /* common Provides tests */
    rc |= Test(Provides,
	    Foo "", "",
	    Foo ">=", "1.0",
	    -1);
    rc |= Test(Provides,
	    Foo ">", "2.0",
	    Foo ">=", "1.0",
	    -1);
    /* test Requires with the same version */
    rc |= Test(Requires,
	    Foo ">=", "1.0",
	    Foo "=", "1.0",
	    -1);
    rc |= Test(Requires,
	    Foo ">=", "1.0",
	    Foo "=", "1.0-1",
	    -1);
    rc |= Test(Requires,
	    Foo ">=", "1.0-1",
	    Foo "=", "1.0",
	    -2);
    /* test Provides with the same version */
    rc |= Test(Provides,
	    Foo ">=", "1.0",
	    Foo "=", "1.0-1",
	    -2);
    rc |= Test(Provides,
	    Foo "=", "1.0",
	    Foo ">=", "1.0-1",
	    -1);
    rc |= Test(Provides,
	    Foo ">=", "1.0",
	    Foo ">=", "1.0-1",
	    -1);
    rc |= Test(Provides,
	    Foo ">=", "1.0-1",
	    Foo "=", "1.0-1",
	    1);
    return rc;
}

/* ex: set ts=8 sts=4 sw=4 noet: */
