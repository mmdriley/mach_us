#
# genrom.awk
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.4 $
# $Date: 1993/02/02 00:23:48 $
#

#
# generates an 'initRom' routine from an existing ROM file which
# initializes the 'rom' array as if the ROM file had been read at
# runtime.  This is mainly for in-kernel use.


BEGIN {
	fmtString = "\trom[%d][%d] = %s;\n";

#	printf("\n#include \"platform.h\"\n");
#	printf("\n#include \"xk_debug.h\"\n");
	printf("\nvoid\ninitRom()\n{\n");
}

{ 
	for ( i=1; i <= NF; i++ ) {
		code = code sprintf(fmtString, NR-1, i-1, "\"" $i "\"");
	}
	if ( maxFields < (NF-1) ) maxFields = NF-1;
	code = code sprintf(fmtString, NR-1, NF, "0");
}

END {
	printf("\tif ( ROM_MAX_LINES < %d ) {\n", NR);
	printf("\t\txError(\"xkernel ROM array too big\");\n");
	printf("\t\treturn;\n");
	printf("\t}\n");

	printf("\tif ( ROM_MAX_FIELDS < %d ) {\n", maxFields);
	printf("\t\txError(\"xkernel ROM -- too many fields\");\n");
	printf("\t\treturn;\n");
	printf("\t}\n");
	printf code;
	printf(fmtString, NR, 0, "0");
	printf("}\n");
}
