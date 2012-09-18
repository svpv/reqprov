#ifndef REQPROV_H
#define REQPROV_H

/**
 * Compare sense/version parts of two deps and decide if there is a stronger one.
 * @param tag		RPMTAG_{REQUIRE,PROVIDE,CONFLICTS,OBSOLETE}_VERSION
 * @param evr1		version from dep1
 * @param flags1	flags from dep1
 * @param evr2		version from dep2
 * @param flags2	flags from dep2
 * @return
 * 	+1 if dep1 is stronger
 *	 0 if dep1 and dep2 are equal
 *	-1 if dep2 is stronger
 *	-2 if dep1 and dep2 are essesntially different
 */
int compare_version_requirement(int tag,
	const char *evr1, int flags1,
	const char *evr2, int flags2);

#endif

/* ex: set ts=8 sts=4 sw=4 noet: */
