#include <string.h>
#include <assert.h>
#include <rpmlib.h>

#if 1
#define xstrdup strdup
int rpmsetcmp(const char *str1, const char *str2);
#endif

/* Compares two EVR triples. */
static int compare_evr(char *evr1, char *evr2, int *rcmp)
{
    const char *e1, *v1, *r1;
    const char *e2, *v2, *r2;
    parseEVR(evr1, &e1, &v1, &r1);
    parseEVR(evr2, &e2, &v2, &r2);
    /* compare epoch */
    int ecmp = 0;
    if ((e1 && *e1) && (e2 && *e2))
	ecmp = rpmvercmp(e1, e2);
    else if ((e1 && *e1) || (e2 && *e2)) {
	/* only one side has epoch */
	return -2;
    }
    /* compare version */
    int vcmp = rpmvercmp(v1, v2);
    /* version comparison must be consistent with epoch */
    if (vcmp > 0 && ecmp < 0)
	return -2;
    if (vcmp < 0 && ecmp > 0)
	return -2;
    if (vcmp == 0 && ecmp != 0)
	return -2;
    /* handle release */
    if (vcmp == 0) {
	/* when both releases are present, they affect main result */
	if ((r1 && *r1) && (r2 && *r2))
	    vcmp = rpmvercmp(r1, r2);
	/* otherwise, we keep vcmp = 0 and set rcmp instead */
	else if (r1 && *r1)
	    *rcmp = 1;
	else if (r2 && *r2)
	    *rcmp = -1;
    }
    return vcmp;
}

/* Match Requires or Provides version comparison with sense flags */
static int compare_sense(int prov, int vcmp, int rcmp, int sense1, int sense2)
{
    if (vcmp == 0) {
	if (sense1 == sense2) {
	    /*
	     * Example:
	     *   Requires: foo >= 1.0
	     * + Requires: foo >= 1.0-1
	     *
	     * Example:
	     *   Provides: foo >= 1.0
	     *     (less specific Provides)
	     * + Provides: foo >= 1.0-1
	     *     (more specific Provides, can substitute for less specific)
	     */
	    if (sense1 & RPMSENSE_EQUAL)
		return rcmp;
	    /*
	     * Example:
	     * + Requires: foo > 1.0
	     *     (can match only higher version - a stronger requirement)
	     *   Requires: foo > 1.0-1
	     *     (can match version 1.0 with higher release)
	     * 
	     * Example:
	     *   Provides: foo > 1.0
	     *     (cannot satisfy Requires with version 1.0)
	     * + Provides: foo > 1.0-1
	     *     (can satisfy e.g. Requires: foo >= 1.0-2 - a wider range)
	     */
	    return prov ? rcmp : -rcmp;
	}
	else if ((sense1 & sense2) == sense1) {
	    if (sense1 == RPMSENSE_EQUAL) {
		/*
		 * Example:
		 *   Requires: foo = 1.0
		 *   Requires: foo >= 1.0-1
		 *     (cannot optimize)
		 *
		 * Example:
		 *   Provides: foo = 1.0
		 * + Provides: foo >= 1.0-1
		 *
		 * Example:
		 * + Requires: foo = 1.0-1
		 *   Requires: foo >= 1.0
		 *
		 * Example:
		 *   Provides: foo = 1.0-1
		 *   Provides: foo >= 1.0
		 *     (cannot optimize?)
		 */
		if (prov) {
		    if (rcmp <= 0)
			return -1;
		}
		else {
		    if (rcmp >= 0)
			return 1;
		}
	    }
	    else {
		assert(sense1 == RPMSENSE_GREATER || sense1 == RPMSENSE_LESS);
		/*
		 * Example:
		 * + Requires: foo > 1.0
		 *   Requires: foo >= 1.0-1
		 *
		 * Example:
		 *   Provides: foo > 1.0
		 * + Provides: foo >= 1.0-1
		 * 
		 * Example:
		 * + Requires: foo > 1.0-1
		 *   Requires: foo >= 1.0
		 *
		 * Example:
		 *   Provides: foo > 1.0-1
		 *   Provides: foo >= 1.0
		 *     (cannot optimize?)
		 */
		return prov ? -1 : 1;
	    }
	}
	else if ((sense1 & sense2) == sense2) {
	    /* reverse logic */
	    if (sense2 == RPMSENSE_EQUAL) {
		if (prov) {
		    if (rcmp >= 0)
			return 1;
		}
		else {
		    if (rcmp <= 0)
			return -1;
		}
	    }
	    else {
		assert(sense2 == RPMSENSE_GREATER || sense2 == RPMSENSE_LESS);
		return prov ? 1 : -1;
	    }
	}
    }
    else if (vcmp > 0) {
	/*
	 * Example:
	 * + Requires: foo = 2.0
	 *   Requires: foo
	 */
	if (sense2 == 0)
	    return 1;
	/*
	 * Example:
	 * + Requires: foo = 2.0
	 *   Requires: foo >= 1.0
	 *
	 * Example:
	 *   Provides: foo = 2.0
	 * + Provides: foo >= 1.0
	 *
	 * Example:
	 *   Requires: foo >= 2.0
	 *   Requires: foo = 1.0
	 * (cannot optimize)
	 * 
	 * Note that sense2 must have GT flag to cover evr1.
	 */
	if (!(sense1 & RPMSENSE_LESS) && (sense2 & RPMSENSE_GREATER))
	    return prov ? -1 : 1;
	/*
	 * Example:
	 *   Requires: foo <= 2.0
	 * + Requires: foo = 1.0
	 *
	 * Example:
	 * + Provides: foo <= 2.0
	 *   Provides: foo = 1.0
	 *
	 * Example:
	 *   Requires: foo = 2.0
	 *   Requires: foo <= 1.0
	 * (cannot optimize)
	 *
	 * Note that sense1 must have LT flag to cover evr2.
	 */
	if (!(sense2 & RPMSENSE_GREATER) && (sense1 & RPMSENSE_LESS))
	    return prov ? 1 : -1;
    }
    else {
	assert(vcmp < 0);
	/* reverse args to execute (vcmp > 0) branch */
	return -compare_sense(prov, -vcmp, -rcmp, sense2, sense1);
    }
    return -2;
}

/* API routine */
int compare_version_requirement(int tag,
	const char *evr1, int flags1,
	const char *evr2, int flags2)
{
    /* vcmp is main version comparison result */
    int vcmp = 0;
    /* rcmp indicates that vcmp = 0 but only one side has a release */
    int rcmp = 0;
    /* there must be a sense iff evr comes along */
    int sense1 = flags1 & RPMSENSE_SENSEMASK;
    int sense2 = flags2 & RPMSENSE_SENSEMASK;
    assert((sense1 == 0) ^ (evr1 && *evr1));
    assert((sense2 == 0) ^ (evr2 && *evr2));

    /* compare evr1 and evr2 */
    if (sense1 && sense2) {
	int set1 = strncmp(evr1, "set:", 4) == 0;
	int set2 = strncmp(evr2, "set:", 4) == 0;
	if (set1 && set2) {
	    /* check for subset */
	    vcmp = rpmsetcmp(evr1 + 4, evr2 + 4);
	    if (vcmp < -1)
		return -2;
	}
	else if (set1 || set2) {
	    /* no match between a set and a non-set */
	    return -2;
	}
	else {
	    char *evr1x = xstrdup(evr1);
	    char *evr2x = xstrdup(evr2);
	    vcmp = compare_evr(evr1x, evr2x, &rcmp);
	    evr1x = _free(evr1x);
	    evr2x = _free(evr2x);
	    if (vcmp == -2)
		return -2;
	}
    }
    else if (sense1)
	vcmp = 1;
    else if (sense2)
	vcmp = -1;

    /* interpret version comparison with sense */
    switch (tag) {
    case RPMTAG_REQUIREVERSION:
	return compare_sense(0, vcmp, rcmp, sense1, sense2);
    case RPMTAG_PROVIDEVERSION:
	return compare_sense(1, vcmp, rcmp, sense1, sense2);
    case RPMTAG_CONFLICTVERSION:
    case RPMTAG_OBSOLETEVERSION:
	/*
	 * Conflicts and Obsoletes are the exact opposite of Requires: we must
	 * reverse to a "weaker" version condition which covers a wider range.
	 * Also, Conflicts without a version conflicts with any version.
	 */
	return -compare_sense(0, vcmp, rcmp, sense1, sense2);
    default:
	assert(!"unknown version tag");
	return -2;
    }
}

/* ex: set ts=8 sts=4 sw=4 noet: */
