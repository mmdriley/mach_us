#!/bin/sh
# 
# Mach Operating System
# Copyright (c) 1993,1990 Carnegie Mellon University
# All Rights Reserved.
# 
# Permission to use, copy, modify and distribute this software and its
# documentation is hereby granted, provided that both the copyright
# notice and this permission notice appear in all copies of the
# software, derivative works or modified versions, and any portions
# thereof, and that both notices appear in supporting documentation.
# 
# CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
# CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
# ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
# 
# Carnegie Mellon requests users of this software to return to
# 
#  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
#  School of Computer Science
#  Carnegie Mellon University
#  Pittsburgh PA 15213-3890
# 
# any improvements or extensions that they make and grant Carnegie Mellon 
# the rights to redistribute these changes.
#  
#
#
# HISTORY
# $Log:	mallck.sh,v $
# Revision 2.3  94/07/14  15:57:05  mrt
# 	Added copyright
# 
# Revision 2.2  90/10/29  17:33:48  dpj
# 	Created.
# 	[90/10/27  18:01:39  dpj]
# 
# 	First working version.
# 	[90/10/21  21:33:23  dpj]
# 
#
# mallck
# Mark Brader, SoftQuad Inc., 1987
# Modified by Daniel P. Julin, Carnegie Mellon University, 1990
#
# NOT copyright by SoftQuad.  - msb, 1988
#
# sccsid @(#)mallck	1.4 88/08/24
#
# 
# Checks a malloc trace file as produced by modified forms of malloc,
# free, and realloc -- locally in /usr/lib/malloctrace.a -- for any
# incorrectly paired allocations and frees, and any other problems.
# Also computes a few statistics from the data.
#
# Usage:
#	mallck [a.out] [${MALLOCTRACE-malloc.out}]
# or
#	mallck nm.out  [${MALLOCTRACE-malloc.out}]
#
# Both arguments optional, defaults as indicated.  nm.out is the result of
# running nm on the program; only the T lines are significant.
#

# The contents of the malloc trace file look like this:
#
#	malloc of 71 gets 104 at 143692
#	caller 000020e8
#	caller 0000204c
#	
#	free of 44 at 143500
#	caller 000020f8
#	caller 0000204c
#	

# First, three awk programs.  Unfortunately, while it's most convenient
# to put them in sh variables, it runs sh near its limits, so we use sed
# to pack the programs a bit first...


# Look up locations in symbol table.  This program reads the "caller"
# entries in the malloc trace file, and the output of nm, and produces
# output of this form:
#
#	0000204c     204c start 44
#	000020b8     20b8 _main 24
#	000020c8     20c8 _main 40
#	000020d8     20d8 _main 56

SYMTAB=/tmp/symtab.$$
FIND=/tmp/find.$$
REPORT=/tmp/report.$$

cat > $SYMTAB <<'Eot'
BEGIN {
	digit["a"] = 10		# Hex into decimal without scanf
	digit["b"] = 11
	digit["c"] = 12
	digit["d"] = 13
	digit["e"] = 14
	digit["f"] = 15
	for (i = 0; i <= 9; ++i) digit[i] = i
	func = "0"
	funaddr = 0
}
/./ {
	addr = 0
	for (i = 1; i <= 8; ++i) addr = 16*addr + digit[substr($1,i,1)]
	if (addr + 1 == addr) {
		printf " --- **** WARNING: conversion of hex %s to decimal" \
			" %d may be inaccurate!\n", $1, addr
			# Single precision floating point!
	}
	if ($2 == "T") {
		func = $3
		funaddr = addr
	} else \
		printf "%8.8x %8x %s %d\n", addr, addr, func, addr - funaddr
}
Eot

# Examine calls to malloc et al, and locate any problems.
# This program reads the malloc trace file again, and produces output of
# the following form, which is then sorted and filtered through uniq -c.
# The part before the "---" is a complete call stack traceback.
# There are also summary lines.
#
#	 00002110 0000204c --- malloc - size 21 - never freed
#	 00002124 0000204c --- realloc-to - size 21 - never freed
#
# The sizes shown are the requested sizes, not the actual ones including
# overhead.  This is so that duplicate entries can be reduced accurately
# by uniq -c.  However, when errors are reported, actual sizes are shown too.
#

cat > $FIND <<'Eot'
BEGIN {
		# So null input is harmless ...
	alact[""] = totalloc = maxalloc = maxloc = 0

		# for waking up recipients of important errors
	bang = " *****************"
}

/of/ {
	act = $1
	reqsize = $3
	caller = ""
	if ($4 == "gets") {
		gotsize = $5
		loc = $7
	} else {
		gotsize = reqsize
		loc = $5
	}
}

/caller/ {
	caller = caller " " $2		# concatenate traceback together
}

/^$/ {
	if (act == "malloc" || act == "realloc-to") {

		if (loc == 0) {
			printf "%s --- %s - size %s - ALLOCATION FAILED%s\n", \
				caller, act, reqsize, bang

		} else {
				# just take note for later reference
			alcall[loc] = caller
			alreq [loc] = reqsize
			algot [loc] = gotsize
			alact [loc] = act
			totreq += reqsize
			totgot += gotsize
			if (maxreq < totreq) maxreq = totreq
			if (maxgot < totgot) maxgot = totgot
			if (maxloc < gotsize + loc) maxloc = gotsize + loc
		}

	} else {
				# Must be free or realloc - check the data

		if (alact[loc]) {
			if (algot[loc] != gotsize) {
				printf "%s --- %s - size %s - actual %s -" \
					" WRONG SIZE LATER FREED%s\n", \
					alcall[loc], alact[loc], alreq[loc], \
					algot[loc], bang
				printf "%s --- %s - size %s -" \
					" ACTUAL ALLOCATED SIZE WAS %s%s\n", \
					caller, act, gotsize, algot[loc], bang
			}
			alact[loc] = ""
			totgot -= gotsize
			totreq -= alreq[loc]

		} else {
			printf "%s --- %s - size %s - UNALLOCATED%s\n", \
				caller, act, gotsize, bang
		}
	}
}

END {
				# check for never-freed space
	for (loc in alact)
		if (alact[loc])
			printf "%s --- %s - size %s -" \
				" never freed\n", \
				alcall[loc], alact[loc], alreq[loc]

		# The following statistics will print at the top of the
		# final output, by a little trickery -- their beginnings
		# are chosen to sort before the other records (and to
		# leave them in the desired order after sorting)

	print " --- ****** ALLOCATION STATISTICS (excluding most stdio)"
	printf " --- ****** Greatest allocated address: %d (hex %x)\n", \
			maxloc - 1, maxloc - 1
	print " --- ****** High-water allocation level: " \
			maxreq " bytes (+ " maxgot-maxreq " overhead)"
	print " --- ****** Total of memory never freed: " \
			totreq " bytes (+ " totgot-totreq " overhead)"
}
Eot

# Format the output of the 2nd awk program using the output
# of the 1st to display symbolic locations

cat > $REPORT <<'Eot'
! / --- / {
	func[$1] = $3
	offs[$1] = $4
	nozeros[$1] = $2
	if (maxlen < length($3)) maxlen = length($3)
	if (maxdig < length($4)) maxdig = length($4)
}

/ --- / {
	for (punct = 1; $punct != "---"; ++punct) {;} # Where in line is "---"?

	if (punct > 2) printf "\n\n"
	if ($1 > 1) printf "occurring %d times - ", $1	# Bless uniq -c!

	for (i = punct + 1; i <= NF; ++i) printf "%s ", $i
	printf "\n"

	if (punct > 2) {
		for (i = 2; i < punct; ++i) {
			if (i == 2) printf "\n\tcalled"; else printf "\t"
			printf "\tfrom %8s [%" maxlen "s + %" maxdig "d]\n", \
				nozeros[$i], func[$i], offs[$i]
		}
	}
}

Eot


# Default arguments:

BIN=${1-a.out}
RAWTRACE=${2-${MALLOCTRACE-malloc.out}}

TRACE=/tmp/malloctrace.$$

# And do it...

sed -n -e "/.*MALLOC_TRACE: /s///p" < $RAWTRACE > $TRACE

(
	(
		(sed -n '/caller /s///p' $TRACE || exit) \
			| sort -u
		(nm -gp $BIN 2>/dev/null || cat $BIN) \
			| grep ' T '
	) \
		| sort | awk -f $SYMTAB
	awk -f $FIND $TRACE | sort | uniq -c
) \
| awk -f $REPORT

rm -f $SYMTAB $FIND $REPORT $TRACE
