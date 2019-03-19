/*
 * LEVEE, or Captain Video;  A vi clone
 *
 * Copyright (c) 1982-2007 David L Parsons
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, without or
 * without modification, are permitted provided that the above
 * copyright notice and this paragraph are duplicated in all such
 * forms and that any documentation, advertising materials, and
 * other materials related to such distribution and use acknowledge
 * that the software was developed by David L Parsons (orc@pell.portland.or.us).
 * My name may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.
 */

#include "levee.h"
#include "extern.h"

#include <stdlib.h>

static void
copyright()
{
     dputs("  Copyright (c) 1983-2019 by David Parsons");
}


void
initialize(argc, argv)
int argc;
char **argv;
/* initialize: set up everything I can in levee */
{
    int i;
    int xmode = E_INIT, xquit;
    char *p;

    dinitialize();
    set_input();
    dscreensize(&COLS, &LINES);
    dofscroll = LINES/2;

    if ( lineonly = !CA ) {
        dgotoxy(0, 0);
	dclear_to_eol();
	version(); copyright();
	prints("(line mode)");
    }
    else if ( argc <= 1 ) {
	dgotoxy(LINES-1, 0);
	version(); copyright();
    }


    /* initialize macro table */
    for (i = 0;i < MAXMACROS;i++)
	mbuffer[i].token = 0;
    core[0] = EOL;
	
    yank.size = ERR;		/* no yanks yet */
    
    undo.blockp = undo.ptr = 0;
    
    fillchar(adjcurr, sizeof(adjcurr), 0);
    fillchar(adjendp, sizeof(adjendp), 0);
    
    adjcurr[BTO_WD]	=	/* more practical to just leave dynamic */
    adjcurr[SENT_BACK]	=
    adjendp[BTO_WD]	=
    adjendp[FORWD]	=
    adjendp[MATCHEXPR]	=
    adjendp[PATT_BACK]	=
    adjendp[TO_CHAR]	=
    adjendp[UPTO_CHAR]	=
    adjendp[PAGE_BEGIN]	=
    adjendp[PAGE_MIDDLE]=
    adjendp[PAGE_END]	= TRUE;

    fillchar(contexts, sizeof(contexts), -1);

    os_mktemp(undobuf, "$un");
    os_mktemp(yankbuf, "$ya");
    os_mktemp(undotmp, "$tm");
    
    dgotoxy(LINES-1,0);

    
    if ( p = getenv("LVRC") ) {
	extern char* execstr;
	
	strcpy(instring,p);
	execstr = instring;
	setcmd();
    }
    else if ( p = dotfile() )
	do_file(p, &xmode, &xquit);

    ++argv, --argc;
    
#if SOFT_EOL
    /* USCD pascal-style (and early Macos?) EOL
     */
    if (argc > 0 && strcmp(*argv, "-p") == 0 ) {
	++argv, --argc;
	EOL = '\r';
    }
#endif

    if (argc > 0 && **argv == '+') {
	char *p = *argv;
	strcpy(startcmd, p[1] ? (1+p) : "$");
	++argv, --argc;
    }
    memset(&args, sizeof args, 0);
    while (argc-- > 0)
	os_glob(*argv++, GLOB_APPEND|GLOB_NOMAGIC, &args);
	
    if (args.gl_pathc > 0) {
	strcpy(filenm, args.gl_pathv[0]);
	if (args.gl_pathc > 1)
	    toedit(args.gl_pathc);
	inputf(filenm,TRUE);
    }
    else
	filenm[0] = 0;
}

bool
execmode(emode)
exec_type emode;
{
    bool more,			/* used [more] at end of line */
	 noquit;		/* quit flag for :q, :xit, :wq */
    exec_type mode;

    zotscreen = diddled = FALSE;
    noquit = TRUE;

    if (lineonly)
	println();

    mode=emode;
    do {
	prompt(FALSE,":");
	if (lvgetline(instring))
	    exec(instring, &mode, &noquit);
	indirect = FALSE;
	if (mode == E_VISUAL && zotscreen && noquit) {	/*ask for more*/
	    prints(" [more]");
	    if ((ch=peekc()) == 13 || ch == ' ' || ch == ':')
		readchar();
	    more = (ch != ' ' && ch != 13);
	}
	else
	    more = (mode == E_EDIT);
	if (mode != E_VISUAL && curpos.x > 0)
	    println();
    } while (more && noquit);
    if (zotscreen)
	clrprompt();
    return noquit;
}

int
main(argc,argv)
int argc;
char **argv;
{
    initialize(argc, argv);

    diddled = TRUE;	/* force screen redraw when we enter editcore() */
    if (lineonly)
	while (execmode(E_EDIT))
	    prints("(no visual mode)");
    else
	while (execmode(editcore()))
            /* do nada */;

    os_unlink(undobuf);
    os_unlink(yankbuf);

    if ( curpos.x )
	println();

    reset_input();
    os_restore();
    exit(0);
}
