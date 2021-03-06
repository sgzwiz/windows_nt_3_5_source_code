#include "cmd.h"
#include "cmdproto.h"

extern   DWORD DosErr ;

/* The following are definitions of the debugging group and level bits
 * for the code in this file.
 */

#define MMGRP	0x1000	/* Memory Manager				   */
#define MALVL	0x0001	/* Memory allocators				   */
#define LMLVL	0x0002	/* List managers				   */
#define SMLVL	0x0004	/* Segment manipulators 			   */


/* Data Stack - a stack of pointers to memory that has been allocated *M005*/

typedef struct _DSTACK {
    ULONG cb ;               /* malloc's length value (M011)            */
    struct _DSTACK *pdstkPrev; /* Pointer to the previous list element    */
    CHAR data ;             /* The data block                          */
} DSTACK, *PDSTACK;

#define PTRSIZE FIELD_OFFSET(DSTACK, data) /* Size of element header*/

PDSTACK DHead = NULL ;   /* Head of the data list           */
ULONG DCount = 0 ;       /* Number of elements in the list  */

PVOID BigBufHandle = 0 ;   /* Handle/segment of buffer used by type & copy */


#if DBG




/***	MemChk1 - Sanity check on one element of the data stack
 *
 *  Purpose:
 *	Verifies the integrity and length of a single data element
 *
 *  int MemChk1(PDSTACK s)
 *
 *  Args:
 *	s - Pointer to the data stack element to check on
 *
 *  Returns:
 *	0 - If element is intact and okay
 *	1 - If element size or integrity is off
 *
 */

MemChk1(
    IN  PDSTACK pdstk
    )
{
        if (pdstk->cb != _msize(pdstk)) {
            cmd_printf (TEXT("len = %d"), pdstk->cb) ;
		return(1) ;
        } else {
		return(0) ;
        } ;
}




/***	MemChkBk - Sanity check on data stack elements from here back
 *
 *  Purpose:
 *	Verifies the integrity of the CMD data stack from a single
 *	point back to the beginning.
 *
 *  int MemChkBk(PDSTACK s)
 *
 *  Args:
 *	s - Pointer to the data stack element to start with
 *
 *  Returns:
 *	0 - If elements are intact and okay
 *	1 - If elements' size or integrity are off
 *
 */

MemChkBk(
    IN  PDSTACK pdstk
    )
{
        ULONG   cnt ;           // Element counter
        PDSTACK pdstkCur;   // Element pointer

	cnt = DCount ;

        for (pdstkCur = DHead, cnt = DCount ; pdstkCur ; pdstkCur = (PDSTACK)pdstkCur->pdstkPrev, cnt--) {
                if (pdstkCur == pdstk) {
			break ;
		} ;
	} ;

        while (pdstkCur) {
                if (MemChk1(pdstkCur)) {
                        cmd_printf(TEXT("Memory Element %d @ %04x contaminated!"), cnt, pdstkCur) ;
			abort() ;
		} ;
                pdstkCur = (PDSTACK)pdstkCur->pdstkPrev ;
		--cnt ;
	} ;

	return(0) ;
}




/***	MemChkAll - Sanity check on one element of the data stack
 *
 *  Purpose:
 *	Checks the entire data stack for integrity.
 *
 *  int MemChkAll()
 *
 *  Args:
 *
 *  Returns:
 *	0 - If elements are intact and okay
 *	1 - If elements' size or integrity are off
 *
 */

MemChkAll()
{
	return(MemChkBk(DHead)) ;
}

#endif


/***	FreeBigBuf - free the buffer used by the type and copy commands
 *
 *  Purpose:
 *	If BigBufHandle contains a handle, unlock it and free it.
 *
 *  FreeBigBuf()
 *
 * ***	NOTE:	This routine manipulates Command's Buffer handle, and	***
 * ***		should be called with signal processing postponed.	***
 */

void FreeBigBuf()
{
    if (BigBufHandle) {
        DEBUG((MMGRP, LMLVL, "    FREEBIGBUF: Freeing bigbufhandle = 0x%04x", BigBufHandle)) ;

        VirtualFree(BigBufHandle,0,MEM_RELEASE) ;
        BigBufHandle = 0 ;
    } ;
}




/***	FreeStack - free the memory on the data stack
 *
 *  Purpose:
 *	Free the memory pointed to by all but the first n elements of the
 *	data stack and free BigBufHandle if it is nonzero.
 *
 *  FreeStack(int n)
 *
 *  Args:
 *	n - the number of elements to leave on the stack
 *
 *				W A R N I N G
 *	!!! THIS ROUTINE CAUSES AN ABORT IF DATA STACK CONTAMINATED !!!
 */

void FreeStack(
    IN ULONG n
    )
{
    PDSTACK pdstkPtr ;

    DEBUG((MMGRP, LMLVL, "    FREESTACK: n = %d  DCount = %d", n, DCount)) ;

    while (DCount > n && (pdstkPtr = DHead)) {
        /* Free the top item in the data stack and pop the stack */

        DHead = (PDSTACK)DHead->pdstkPrev ;
        -- DCount ;
        free((PTCHAR)pdstkPtr) ;
    } ;

#if DBG

	MemChkAll() ;		/* CAUSES abort() IF CONTAMINATED	   */

#endif
	FreeBigBuf() ;

	DEBUG((MMGRP, LMLVL, "    FREESTACK: n = %d, DCount = %d", n, DCount)) ;
}

void
FreeStr(
    IN  PTCHAR   pbFree
    )

{

    PDSTACK pdstkCur;
    PDSTACK pdstkPtr, pdstkLast ;
             ULONG   cdstk;

    DEBUG((MMGRP, LMLVL, "    FreeStr: pbFree = %x DCount = %d", pbFree, DCount)) ;

    if ((pbFree == NULL) || (DHead == NULL)) {

        return;

    }

    pdstkPtr = (PDSTACK)((CHAR*)pbFree - PTRSIZE);

    for(pdstkCur = DHead, cdstk = DCount; cdstk; cdstk--) {

        if (pdstkCur == pdstkPtr) {

            //
            // remove from chain
            //
            DEBUG((MMGRP, LMLVL, "    FreeStr: Prev %x, Cur %x, DCount %d",
                   pdstkLast, pdstkCur, DCount)) ;
            if (pdstkCur == DHead) {
                DHead = (PDSTACK)pdstkCur->pdstkPrev;
            } else {
                pdstkLast->pdstkPrev = pdstkCur->pdstkPrev;
            }
            free(pdstkCur);
            DCount--;
#if DBG

            MemChkAll() ;

#endif

            return;
        }
        pdstkLast = pdstkCur;
        pdstkCur = (PDSTACK)pdstkCur->pdstkPrev;

    }

#if DBG

    MemChkAll() ;

#endif

}


/***	GetBigBuf -  allocate a large buffer
 *
 *  Purpose:
 *	To allocate a buffer for data transferrals.
 *	The buffer will be as large as possible, up to MAXBUFSIZE bytes,
 *	but no smaller than MINBUFSIZE bytes.
 *
 *  TCHAR *GetBigBuf(unsigned *blen)
 *
 *  Args:
 *	blen = the variable pointed to by blen will be assigned the size of
 *	    the buffer
 *
 *  Returns:
 *	A TCHAR pointer containing segment:0.
 *	Returns 0L if unable to allocate a reasonable length buffer
 *
 */

PVOID

GetBigBuf(
    IN	ULONG	CbMaxToAllocate,
    IN	ULONG	CbMinToAllocate,
    OUT unsigned int *CbAllocated
    )


/*++

Routine Description:

    To allocate a buffer for data transferrals.

Arguments:

    CbMinToAllocate - Fail if can't allocate this number
    CbMaxToAllocate - Initial try and allocation will use this number
    CbAllocated - Number of bytes allocated


Return Value:

    Return: NULL - if failed to allocate anything
	    pointer to allocated buffer if success
--*/

{
    ULONG   cbToDecrease;
    PVOID   handle ;

    DEBUG((MMGRP, MALVL, "GETBIGBUF: MinToAlloc %d, MaxToAlloc %d", CbMinToAllocate, CbMaxToAllocate)) ;

    cbToDecrease = CbMaxToAllocate;
    //bytesdecrease = CbMaxToAllocate ;

    while (!(handle = VirtualAlloc(NULL, CbMaxToAllocate,MEM_COMMIT,PAGE_READWRITE))) {

	//
	// Decrease the desired buffer size by CbToDecrease
	// If the decrease is too large, make it smaller
	//
	if ( cbToDecrease >= CbMaxToAllocate ) {
	    cbToDecrease = ((CbMaxToAllocate >> 2) & 0xFE00) + 0x200;
	}

	if ( cbToDecrease < CbMinToAllocate ) {
	    cbToDecrease = CbMinToAllocate ;
	}

	CbMaxToAllocate -= cbToDecrease ;

	if ( CbMaxToAllocate < CbMinToAllocate ) {

	    //
	    // Unable to allocate a reasonable buffer
	    //
	    *CbAllocated = 0 ;
	    PutStdErr(ERROR_NOT_ENOUGH_MEMORY, NOARGS);
	    return ( NULL ) ;
	}
    }

    *CbAllocated = CbMaxToAllocate ;

    FreeBigBuf() ;
    BigBufHandle = handle ;

    DEBUG((MMGRP, MALVL, " GETBIGBUF: Bytes Allocated = %d  Handle = 0x%04x", *CbAllocated, BigBufHandle)) ;

    return(handle) ;
}




/***	mknode - allocata a parse tree node
 *
 *  Purpose:
 *	To allocate space for a new parse tree node.  Grow the data segment
 *	if necessary.
 *
 *  struct node *mknode()
 *
 *  Returns:
 *	A pointer to the node that was just allocated.
 *
 *  Notes:
 *	This routine must always use calloc().	Many other parts of Command
 *	depend on the fact that the fields in these nodes are initialized to 0.
 *
 *	THIS ROUTINE RETURNS `NULL' IF THE C RUN-TIME CANNOT ALLOCATE MEMORY
 */

struct node *mknode()
{
	DEBUG((MMGRP, MALVL, "    MKNODE: Entered")) ;
	return((struct node *) mkstr(sizeof(struct node))) ;
}




/***	mkstr -  allocate space for a string
 *
 *  Purpose:
 *	To allocate space for a new string.  Grow the data segment if necessary.
 *
 *  TCHAR *mkstr(size)
 *
 *  Args:
 *	size - size of the string to be allocated
 *
 *  Returns:
 *	A pointer to the string that was just allocated.
 *
 *  Notes:
 *	This routine must always use calloc().	Many other parts of Command
 *	depend on the fact that memory that is allocated is initialized to 0.
 *
 *    - M005 * The piece of memory allocated is large enough to include
 *	a pointer at the beginning.  This pointer is part of the list of
 *	allocated memory.  The routine calling mkstr() receives the address
 *	of the first byte after that pointer.  resize() knows about this,
 *	and so must any other routines which directly modify memory
 *	allocation.
 *    - M011 * This function is the same as mentioned above except that the
 *	pointer is now preceeded by a header consisting of two signature
 *	bytes and the length of the memory allocated.  This was added for
 *	sanity checks.
 *
 *	THIS ROUTINE RETURNS `NULL' IF THE C RUN-TIME CANNOT ALLOCATE MEMORY
 *
 *				W A R N I N G
 *	!!! THIS ROUTINE CAUSES AN ABORT IF DATA STACK CONTAMINATED !!!
 */

void*
mkstr(
    IN  int  cbNew
    )
{
    PDSTACK pdstkCur ;  // Ptr to the memory being allocated

    DEBUG((MMGRP, MALVL, "    MKSTR: Entered.")) ;

#if DBG

	MemChkAll() ;		/* CAUSES abort() IF CONTAMINATED	   */

#endif

    if ((pdstkCur = (PDSTACK)(calloc(1, cbNew + PTRSIZE + 4))) == NULL) {
            PutStdErr(ERROR_NOT_ENOUGH_MEMORY, NOARGS);
            return(0) ;
    } ;

    DEBUG((MMGRP, MALVL, "    MKSTR: Adding to stack")) ;

    pdstkCur->cb   = cbNew + PTRSIZE + 4 ;
    pdstkCur->pdstkPrev = (PDSTACK)DHead ;
    DHead = pdstkCur ;
    DCount++ ;

    DEBUG((MMGRP, MALVL, "    MKSTR: ptr = %04x  cbNew = %04x  DCount = %d",
           pdstkCur, cbNew, DCount)) ;

#if DBG

    MemChkBk(pdstkCur) ;           /* CAUSES abort() IF CONTAMINATED          */

#endif

    return(&(pdstkCur->data)) ;    /*M005*/
}




/***	gmkstr - allocate a piece of memory, with no return on failure
 *
 *  Purpose:
 *	Same as "mkstr" except that if memory cannot be allocated, this
 *	routine will jump out to code which will clean things up and
 *	return to the top level of command.
 *
 */

void*
gmkstr(
    IN  int   cbNew
    )

{
        PTCHAR pbNew ;

        if (!(pbNew = (PTCHAR)mkstr(cbNew)))
                Abort() ;

        return(pbNew) ;
}




/***	resize - resize a piece of memory
 *
 *  Purpose:
 *	Change the size of a previously allocated piece of memory.  Grow the
 *	data segment if necessary.  If a new different pointer is returned by
 *      realloc(0), search the dstk for the pointer to the old piece and
 *	update that pointer to point to the new piece.
 *
 *  TCHAR *resize(TCHAR *ptr, unsigned size)
 *
 *  Args:
 *	ptr - pointer to the memory to be resized
 *	size - the new size for the block of memory
 *
 *  Returns:
 *	A pointer to the new piece of memory.
 *
 *    - M005 * Modified for the new scheme for keeping a list of allocated
 *	blocks
 *    - M011 * Modified to use and check new header.
 *
 *	THIS ROUTINE RETURNS `NULL' IF THE C RUN-TIME CANNOT ALLOCATE MEMORY
 *
 *				W A R N I N G
 *	!!! THIS ROUTINE CAUSES AN ABORT IF DATA STACK CONTAMINATED !!!
 */

void*
resize (
    IN  void* pv,
    IN  unsigned int cbNew
    )

{
    PDSTACK pdstkCur ;
             PDSTACK pdstkNew, pdstkOld;
    CHAR* pbOld = pv;

    DEBUG((MMGRP, MALVL, "    RESIZE: Entered.")) ;

    pbOld -= PTRSIZE ;
    pdstkOld = (PDSTACK)pbOld ;

#if DBG

    if (MemChk1(pdstkOld)) {

        cmd_printf(TEXT("Memory Element @ %04x contaminated!"), pdstkOld) ;
        abort() ;

    } ;

#endif

    if (!(pdstkNew = (PDSTACK)realloc(pbOld, cbNew + PTRSIZE + 4))) {
            PutStdErr(ERROR_NOT_ENOUGH_MEMORY, NOARGS);
            return(0) ;
    } ;

    pdstkNew->cb = cbNew + PTRSIZE + 4 ;   // Update to new length

    //
    // revise Data Stack information, updating chain of pdstk's with
    // new pointer
    //
    if (pdstkNew != pdstkOld) {
        if (DHead == pdstkOld) {        // Is head of List
            DHead = pdstkNew ;
        } else {                        // Is in middle of list
            for (pdstkCur = DHead ; pdstkCur ; pdstkCur = (PDSTACK)(pdstkCur->pdstkPrev)) {
                if ((PDSTACK)(pdstkCur->pdstkPrev) == pdstkOld) {

                    pdstkCur->pdstkPrev = (PDSTACK)pdstkNew ;
                    break ;

                }
            }
        }
    }

#if DBG

    MemChkBk(pdstkOld) ;  // CAUSES abort() IF CONTAMINATED

#endif

    DEBUG((MMGRP, MALVL, "    RESIZE: pbOld = %04x  cbNew = %04x",&(pdstkNew->data),cbNew)) ;

    return(&(pdstkNew->data)) ;
}
