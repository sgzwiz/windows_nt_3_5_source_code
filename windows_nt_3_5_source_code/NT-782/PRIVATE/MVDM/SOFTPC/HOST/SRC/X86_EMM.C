/*                      INSIGNIA MODULE SPECIFICATION
                        -----------------------------

MODULE NAME     : 'Lower layer' of Expanded Memory Manager

        THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
        CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
        NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
        AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER        : Simon Frost
DATE            : March '92

PURPOSE         : NT specific code for EMS LIM rev 4.0
                implementation.

The Following Routines are defined:
                1. host_initialise_EM()
                2. host_deinitialise_EM()
                3. host_allocate_storage()
                4. host_free_storage()
                5. host_reallocate_storage()
                6. host_map_page()
                7. host_unmap_page()
                8. host_alloc_page()
                9. host_free_page()
                10. host_copy_con_to_con()
                11. host_copy_con_to_EM()
                12. host_copy_EM_to_con()
                13. host_copy_EM_to_EM()
                14. host_exchg_con_to_con()
                15. host_exchg_con_to_EM()
                16. host_exchg_EM_to_EM()
                17. host_get_access_key()

=========================================================================
*/
#include "nt.h"
#include "ntrtl.h"              // for romdump
#include "insignia.h"
#include "host_def.h"
#include "string.h"
#include "stdlib.h"

IMPORT int DisplayErrorTerm(int, int, char *, int);

#ifdef LIM
#ifdef MONITOR  //x86 specific LIM functions

#include "xt.h"
#include "emm.h"
#include "sas.h"
#include "host_rrr.h"
#include "debug.h"
#include "umb.h"
#include "host_emm.h"

/*      Global Variables                */

/*      Forward Declarations            */
BOOL hold_lim_page(int);

/*      Local Variables                 */

LOCAL   UTINY *EM_pagemap_address = NULL;     /* address of start of pagemap */
LOCAL   LONG    EM_size = 0;

/* pagemap requires 1 bit per 16K page - i.e. 8 bytes per meg   */
LOCAL UTINY EM_pagemap[8*32];

LOCAL VOID *BaseOfLIMMem = NULL;        // start of expanded memory
LOCAL VOID *HolderBlock = NULL;	// start of place holder block

LOCAL HANDLE LIMSectionHandle, LIMHolderHandle;
LOCAL HANDLE processHandle = NULL;

LOCAL ULONG X86NumRoms = 0;

#define PAGE_SEG_SIZE	0x400	/* size of page expressed as segment */

#define CONFIG_DATA_STRING L"Configuration Data"
#define KEY_VALUE_BUFFER_SIZE 2048

#define EMMBASE 0xd0000
#define EMMTOP  0xe0000

#define SECTION_NAME        L"\\BaseNamedObjects\\LIMSection"
#define SECTION_NAME_LEN    sizeof(SECTION_NAME)

typedef struct _BIOS_BLOCK {
    ULONG PhysicalAddress;
    ULONG SizeInByte;
} BIOS_BLOCK;

static sys_addr emm_start = 0;
static unsigned int emm_len = 0;
static word emm_start_seg = 0;

LOCAL HANDLE GapSectionHandle;
LOCAL VOID *BaseOfGapMem = NULL;
LOCAL TINY MapState[4] = { -1, -1, -1, -1 };
/*
Defines are:
        EM_loads(from, to, length), copies length bytes from intel 24 bit
                address from, to host 32 bit address to
        EM_stores(to, from, length), copies length bytes from host 32 bit
                address from to intel 24 bit address to
        EM_moves(from, to, length), copies length bytes from intel 24 bit
                address from to intel 24 bit address to
        EM_memcpy(to, from, length), copies length bytes from host 32 bit
                address from to host 32 bit address to
*/


#define EM_loads(from, to, length) memcpy(to, get_byte_addr(from), length)
#define EM_stores(to, from, length) \
        RtlCopyMemory(get_byte_addr(to), from, length)
#define EM_moves(from,to,length) \
        RtlMoveMemory(get_byte_addr(to), get_byte_addr(from), length)
#define EM_memcpy(to, from, length) \
        RtlMoveMemory(to, from, length)

/*
===========================================================================

FUNCTION        : host_initialise_EM

PURPOSE         : allocates the area of memory that is used for
                expanded memory and sets up an area of memory to be used
                for the logical pagemap allocation table.


RETURNED STATUS : SUCCESS - memory allocated successfully
                  FAILURE - unable to allocate required space

DESCRIPTION     :


=========================================================================
*/
int host_initialise_EM(short size)
/*   IN short   size             size of area required in megabytes     */
{
    UTINY *pagemap_ptr;         /* temp ptr. to logical pagemap */
    int i;                              /* loop counter                 */
    NTSTATUS status;
    OBJECT_ATTRIBUTES objAttribs;
    LARGE_INTEGER secSize;
    ULONG viewSize;
    SAVED UNICODE_STRING LIMSectionName =
    {
        SECTION_NAME_LEN,
        SECTION_NAME_LEN,
        SECTION_NAME
    };

    EM_pagemap_address = &EM_pagemap[0];

    /* initialise pagemap to 0's        */

    pagemap_ptr = EM_pagemap_address;
    for(i = 0; i < 8*32; i++)
        *pagemap_ptr++ = 0;

    EM_size = ((long) size) * 0x100000;

    if (size == 0)
        return(SUCCESS);        //DANGEROUS OR SENSIBLE???

    if (!(processHandle = NtCurrentProcess()))
    {
        assert0(NO, "host_initialise_EM: cant get process handle");
        return(FAILURE);
    }

    // create section for LIM

    /* Fill the fields of the OBJECT_ATTRIBUTES structure. */
    InitializeObjectAttributes(&objAttribs,
                            NULL, // was &LIMSectionName, but null means private
                            OBJ_CASE_INSENSITIVE,
                            NULL,
                            NULL);

    /* Create the section. */
    secSize.LowPart = EM_size;
    secSize.HighPart = 0;
    // improvement - just reserve  & commit as needed...
    status = NtCreateSection(&LIMSectionHandle,
                                    SECTION_MAP_WRITE|SECTION_MAP_EXECUTE,
                                    &objAttribs,
                                    &secSize,
                                    PAGE_EXECUTE_READWRITE,
                                    SEC_COMMIT,
                                    NULL);
    if (!NT_SUCCESS(status))
    {
        assert1(NO, "host_initialise_EM: LIM section creation failed (%x)", status);
        return(FAILURE);
    }

    /* Map the section to the process' address space. */
    BaseOfLIMMem = NULL;
    viewSize = 0;

    status = NtMapViewOfSection(LIMSectionHandle,
                                       processHandle,
                                       (PVOID *) &BaseOfLIMMem,
                                       0,
                                       0,
                                       NULL,
                                       &viewSize,
                                       ViewUnmap,
                                       0,       // do we need mem_top_down??
                                       PAGE_READWRITE);
    if (!NT_SUCCESS(status))
    {
        assert1(NO, "host_initialise_EM: can't map view of LIM section (%x)", status);
        return(FAILURE);
    }

    // create place holder section for holes when LIM mapped out.

    /* Fill the fields of the OBJECT_ATTRIBUTES structure. */
    InitializeObjectAttributes(&objAttribs,
                            NULL, // was &LIMSectionName, but null means private
                            OBJ_CASE_INSENSITIVE,
                            NULL,
                            NULL);

    /* Create the section. */
    secSize.LowPart = 0x10000;	/* 64K section - 4 pages */
    secSize.HighPart = 0;

    status = NtCreateSection(&LIMHolderHandle,
                                    SECTION_MAP_WRITE|SECTION_MAP_EXECUTE,
                                    &objAttribs,
                                    &secSize,
                                    PAGE_EXECUTE_READWRITE,
                                    SEC_COMMIT,
                                    NULL);
    if (!NT_SUCCESS(status))
    {
        assert1(NO, "host_initialise_EM: LIM place holder creation failed (%x)", status);
        return(FAILURE);
    }

    /* Map the section to the process' address space. */
    HolderBlock = NULL;
    viewSize = 0;

    status = NtMapViewOfSection(LIMHolderHandle,
                                       processHandle,
                                       (PVOID *) &HolderBlock,
                                       0,
                                       0,
                                       NULL,
                                       &viewSize,
                                       ViewUnmap,
                                       0,       // do we need mem_top_down??
                                       PAGE_READWRITE);
    if (!NT_SUCCESS(status))
    {
        assert1(NO, "host_initialise_EM: can't map view of LIM holder block (%x)", status);
        return(FAILURE);
    }

    // use the holder block to put memory in holes
    if (!hold_lim_page(0) || !hold_lim_page(1) || !hold_lim_page(2) ||
							!hold_lim_page(3))
	return(FAILURE);

    return(SUCCESS);
}


/*
===========================================================================

FUNCTION        : host_deinitialise_EM

PURPOSE         : frees the area of memory that was used for
                expanded memory and memory  used
                for the logical pagemap allocation table.


RETURNED STATUS : SUCCESS - memory freed successfully
                  FAILURE - error ocurred in freeing memory

DESCRIPTION     :


=========================================================================
*/
int host_deinitialise_EM()
{
    ULONG len = 0x10000;
    NTSTATUS status;

    if (BaseOfLIMMem != NULL && emm_start != 0)
    {
        if (processHandle == NULL)
        {
	    //As shutting down anyway then fail silently
            return(FAILURE);
        }

        // lose section from our memory space
        status = NtUnmapViewOfSection(processHandle, BaseOfLIMMem);
        if (!NT_SUCCESS(status))
        {
	    //As shutting down anyway then fail silently
            return(FAILURE);
        }

        status = NtClose(LIMSectionHandle);     // delete section
        if (!NT_SUCCESS(status))
        {
	    //As shutting down anyway then fail silently
            return(FAILURE);
        }

        // Holder block will look after Lim gap.

        EM_size = 0;

        return(SUCCESS);
    }
}



/*
===========================================================================

FUNCTION        : host_allocate_storage

PURPOSE         : allocates an area of memory of requested size, to be
                used as a general data storage area. The area is
                to zeros.

RETURNED STATUS : storage_ID - (in this case a pointer)
                 NULL - failure to allocate enough space.


DESCRIPTION     : returns memory initialised to zeros.
                The storage ID returned is a value used to later reference
                the storage area allocated. The macro USEBLOCK in
                "host_emm.h" is used by the manager routines to convert
                this ID into a char pointer

=========================================================================
*/
long host_allocate_storage(int no_bytes)
/*   IN  int    no_bytes                        no. of bytes required   */
{
        // should replace this (?) - dissasembling calloc seems to
        // indicate it uses win funx...
        return ((long)calloc(1, no_bytes));
}


/*
===========================================================================

FUNCTION        : host_free_storage

PURPOSE         : frees the area of memory that was used for
                data storage


RETURNED STATUS : SUCCESS - memory freed successfully
                  FAILURE - error ocurred in freeing memory

DESCRIPTION     : In this implementation storage_ID is simply a pointer


=========================================================================
*/
int host_free_storage(long storage_ID)
/*   IN  long   storage_ID                      ptr to area of memory   */
{
        if(storage_ID != (long)NULL)
                free((char *)storage_ID);

        return(SUCCESS);
}


/*
===========================================================================

FUNCTION        : host_reallocate_storage

PURPOSE         : increases the size of memory allocated, maintaining the
                contents of the original memory block


RETURNED STATUS : storage_ID - memory reallocated successfully
                  NULL - error ocurred in reallocating memory

DESCRIPTION     : In this implementation storage_ID is simply a pointer
                Note the value of storage_ID returned may or may not be the
                same as the value given

=========================================================================
*/
long host_reallocate_storage(LONG storage_ID, int size, int new_size)
/*
    IN
long    storage_ID       ptr to area of memory
int     size             original size - not used in this version
        new_size         new size required
*/
{
        return((long)realloc((char *)storage_ID, new_size));
}


/*
===========================================================================

FUNCTION        : hold_lim_page

PURPOSE         : Puts some memory in one of the LIM page gaps in 16 bit
		  memory. Ensures nothing else in the process gets that
		  via malloc.


RETURNED STATUS : TRUE - mapping OK.

DESCRIPTION     : Mapping achieved by mapping correct page from section into
                  Intel memory
=========================================================================
*/
BOOL hold_lim_page(int page)
/*   IN
int page	page (0-3) of LIM gap 
*/

{
    PVOID to;
    LARGE_INTEGER sec_offset;
    ULONG viewSize;
    NTSTATUS status;

    note_trace1(LIM_VERBOSE,"hold page %d", page);

    if (HolderBlock != NULL)
    {
        to = (PVOID)effective_addr((word)(emm_start_seg + (PAGE_SEG_SIZE * page)),
                                   (word)0);

        sec_offset.LowPart = page * EMM_PAGE_SIZE;
        sec_offset.HighPart = 0;

        viewSize = EMM_PAGE_SIZE;

        status = NtMapViewOfSection(LIMHolderHandle,
                                processHandle,
                                &to,
                                0,
                                EMM_PAGE_SIZE,
                                &sec_offset,
                                &viewSize,
                                ViewUnmap,
                                MEM_DOS_LIM,
                                PAGE_EXECUTE_READWRITE);
        if (!NT_SUCCESS(status))
        {
            DisplayErrorTerm(EHS_FUNC_FAILED,status,__FILE__,__LINE__);
            return(FALSE);
        }
        return(TRUE);
    }
    return(FALSE);
}


/*
===========================================================================

FUNCTION        : host_map_page

PURPOSE         : produces mapping from an Expanded Memory page to a
                page in Intel physical address space


RETURNED STATUS : SUCCESS - mapping completed succesfully
                  FAILURE - error ocurred in mapping

DESCRIPTION     : Mapping achieved by mapping correct page from section into
                  Intel memory
=========================================================================
*/
int host_map_page(SHORT EM_page_no, USHORT segment)
/*   IN
short           EM_page_no      Expanded Memory page to be mapped in
unsigned short  segment;        segment in physical address space to map into
*/

{
    LONG offset;
    PVOID to;
    int tstpage;
    LARGE_INTEGER sec_offset;
    ULONG viewSize;
    NTSTATUS status;

    note_trace2(LIM_VERBOSE,"map page %d to segment 0x%4x", EM_page_no, segment);

    if (BaseOfLIMMem != NULL)
    {
        to = (PVOID)effective_addr(segment, 0);

        sec_offset.LowPart = EM_page_no * EMM_PAGE_SIZE;
        sec_offset.HighPart = 0;

        viewSize = EMM_PAGE_SIZE;

        tstpage = (segment - get_base_address()) >> 10;

        /* clear block */
        status = NtUnmapViewOfSection(processHandle, (PVOID)to);
        if (!NT_SUCCESS(status))
        {
            DisplayErrorTerm(EHS_FUNC_FAILED,status,__FILE__,__LINE__);
            return(FALSE);
        }

        if (processHandle == NULL)
        {
            DisplayErrorTerm(EHS_FUNC_FAILED,0,__FILE__,__LINE__);
            return(FAILURE);
        }


        status = NtMapViewOfSection(LIMSectionHandle,
                                    processHandle,
                                    &to,
                                    0,
                                    EMM_PAGE_SIZE,
                                    &sec_offset,
                                    &viewSize,
                                    ViewUnmap,
                                    MEM_DOS_LIM,
                                    PAGE_EXECUTE_READWRITE
                                    );

        if (!NT_SUCCESS(status))
        {
            DisplayErrorTerm(EHS_FUNC_FAILED,status,__FILE__,__LINE__);
            return(FAILURE);
        }
        MapState[tstpage] = (char) EM_page_no;         /* mark as mapped */
        return(SUCCESS);
    }
    return(FAILURE);
}

/*
===========================================================================

FUNCTION        : host_unmap_page

PURPOSE         :unmaps pages from Intel physical address space to an
                Expanded Memory page

RETURNED STATUS : SUCCESS - unmapping completed succesfully
                  FAILURE - error ocurred in mapping

DESCRIPTION     : Unmapping achieved by unampping view of section from
                  Intel memory

=========================================================================
*/
int host_unmap_page(USHORT segment, SHORT EM_page_no)
/*   IN
unsigned short  segment         Segment in physical address space to unmap.
short           EM_page_no      Expanded Memory page currently mapped in
*/
{
        LONG offset;
        PVOID from;
	int tstpage;
        NTSTATUS status;

        note_trace2(LIM_VERBOSE,"unmap page %d from segment 0x%.4x\n",EM_page_no,segment);

	tstpage = (segment - get_base_address()) >> 10;
	if (MapState[tstpage] == -1)
	{
	    note_trace1(LIM_VERBOSE, "Won't unmap page as segment %04x marked unmapped", segment);
	    return(SUCCESS);
	}

        from = (PVOID)effective_addr(segment, 0);

        status = NtUnmapViewOfSection(processHandle, from);
        if (!NT_SUCCESS(status))
        {
            DisplayErrorTerm(EHS_FUNC_FAILED,status,__FILE__,__LINE__);
            return(FAILURE);
        }
	MapState[tstpage] = -1;

	if (!hold_lim_page((segment - emm_start_seg) / PAGE_SEG_SIZE))
	    note_trace1(LIM_VERBOSE, "couldn't hold lim page %d",(segment - emm_start_seg) / EMM_PAGE_SIZE);

        return(SUCCESS);
}

/*
===========================================================================

FUNCTION        : host_alloc_page

PURPOSE         : searches the pagemap looking for a free page, allocates
                that page and returns the EM page no.

RETURNED STATUS :
                  SUCCESS - Always see note below

DESCRIPTION     : Steps through the Expanded memory Pagemap looking for
                a clear bit, which indicates a free page. When found,
                sets that bit and returns the page number.
                For access purposes the pagemap is divided into long
                word(32bit) sections

NOTE            : The middle layer calling routine (alloc_page()) checks
                that all pages have not been allocated and therefore in
                this implementation the returned status will always be
                SUCCESS.
                However alloc_page still checks for a return status of
                SUCCESS, as some implementations may wish to allocate pages
                dynamically and that may fail.
=========================================================================
*/
SHORT host_alloc_page()
{
        SHORT EM_page_no;               /* page number returned         */
        LONG  *ptr;                     /* ptr to 32 bit sections in    */
                                        /* pagemap                      */
        SHORT i;                        /* index into 32 bit section    */

        ptr = (LONG *)EM_pagemap_address;
        i =0;
        EM_page_no = 0;

        while(*ptr & (MSB >> i++))
        {
                EM_page_no++;

                if(i == 32)
                /*
                 * start on next section
                 */
                {
                        ptr++;
                        i = 0;
                }
        }
        /*
         * Set bit to show that page is allocated
         */
        *ptr = *ptr | (MSB >> --i);

        return(EM_page_no);
}


/*
===========================================================================

FUNCTION        : host_free_page

PURPOSE         : marks the page indicated as being free for further
                allocation

RETURNED STATUS :
                SUCCESS - Always - see note below

DESCRIPTION     : clears the relevent bit in the pagemap.

                For access purposes the pagemap is divided into long
                word(32bit) sections.

NOTE            : The middle layer calling routine (free_page()) always
                checks for invalid page numbers so in this implementation
                the routine will always return SUCCESS.
                However free_page() still checks for a return of SUCCESS
                as other implementations may wish to use it.
=========================================================================
*/
int host_free_page(SHORT EM_page_no)
/*   IN SHORT   EM_page_no      page number to be cleared       */
{
        LONG  *ptr;                     /* ptr to 32 bit sections in    */
                                        /* pagemap                      */
        SHORT i;                        /* index into 32 bit section    */

        /*
         * Set pointer to correct 32 bit section and index to correct bit
         */

        ptr = (long *)EM_pagemap_address;
        ptr += (EM_page_no / 32);
        i = EM_page_no % 32;

        /*
         * clear bit
         */
        *ptr = *ptr & ~(MSB >> i);

        return(SUCCESS);
}


/*
===========================================================================

FUNCTION        : host_copy routines
                host_copy_con_to_con()
                host_copy_con_to_EM()
                host_copy_EM_to_con()
                host_copy_EM_to_EM()

PURPOSE         : copies between conventional and expanded memory


RETURNED STATUS :
                SUCCESS - Always - see note below

DESCRIPTION     :
                 The middle layer calling routine always checks for a
                return of SUCCESS as other implementations may
                return FAILURE.
=========================================================================
*/
int host_copy_con_to_con(int length, USHORT src_seg, USHORT src_off, USHORT dst_seg, USHORT dst_off)

/*   IN
int             length          number of bytes to copy

USHORT  src_seg         source segment address
        src_off         source offset address
        dst_seg         destination segment address
        dst_off         destination offset address
*/
{
        sys_addr from, to;      /* pointers used for copying    */

        from = effective_addr(src_seg, src_off);
        to = effective_addr(dst_seg, dst_off);

        EM_moves(from, to, length);

        return(SUCCESS);
}

int host_copy_con_to_EM(int length, USHORT src_seg, USHORT src_off, USHORT dst_page, USHORT dst_off)

/*   IN
int             length   number of bytes to copy

USHORT  src_seg         source segment address
        src_off         source offset address
        dst_page        destination page number
        dst_off         destination offset within page
*/
{
        UTINY *to;
        sys_addr from;

        from = effective_addr(src_seg, src_off);
        to = (char *)BaseOfLIMMem + dst_page * EMM_PAGE_SIZE + dst_off;

        EM_loads(from, to, length);

        return(SUCCESS);
}

int host_copy_EM_to_con(int length, USHORT src_page, USHORT src_off, USHORT dst_seg, USHORT dst_off)
/*   IN
int     length          number of bytes to copy

USHORT  src_page        source page number
        src_off         source offset within page
        dst_seg         destination segment address
        dst_off         destination offset address
*/
{
        UTINY  *from;
        sys_addr to;

        from = (char *)BaseOfLIMMem + (src_page * EMM_PAGE_SIZE + src_off);
        to = effective_addr(dst_seg, dst_off);

        EM_stores(to, from, length);

        return(SUCCESS);
}

int host_copy_EM_to_EM(int length, USHORT src_page, USHORT src_off, USHORT dst_page, USHORT dst_off)
/*   IN
int     length          number of bytes to copy

USHORT  src_page        source page number
        src_off         source offset within page
        dst_page        destination page number
        dst_off         destination offset within page
*/
{
        unsigned char *from, *to;       /* pointers used for copying    */

        from = (char *)BaseOfLIMMem + src_page * EMM_PAGE_SIZE + src_off;
        to = (char *)BaseOfLIMMem + dst_page * EMM_PAGE_SIZE + dst_off;

        EM_memcpy(to, from, length);

        return(SUCCESS);
}


/*
===========================================================================

FUNCTION        : host_exchange routines
                host_exchg_con_to_con()
                host_exchg_con_to_EM()
                host_exchg_EM_to_EM()

PURPOSE         : exchanges data between conventional and expanded memory


RETURNED STATUS :
                SUCCESS - Everything ok
                FAILURE - Memory allocation failure

DESCRIPTION     :

=========================================================================
*/
int host_exchg_con_to_con(int length, USHORT src_seg, USHORT src_off, USHORT dst_seg, USHORT dst_off)
/*   IN
int     length          number of bytes to copy

USHORT  src_seg          source segment address
        src_off          source offset address
        dst_seg          destination segment address
        dst_off          destination offset address
*/
{
        UTINY *temp, *pointer;/* pointers used for copying      */
        sys_addr to, from;

        if (length <= 64*1024)
            temp = sas_scratch_address(length);
        else
            if ((temp = (unsigned char *)host_malloc(length)) == NULL)
                return(FAILURE);

        pointer = temp;

        from = effective_addr(src_seg, src_off);
        to = effective_addr(dst_seg, dst_off);

        EM_loads(from, pointer, length);
        EM_moves(from, to, length);
        EM_stores(to, pointer, length);

        if (length > 64*1024)
            host_free(temp);

        return(SUCCESS);
}

int host_exchg_con_to_EM(int length, USHORT src_seg, USHORT src_off, USHORT dst_page, USHORT dst_off)
/*   IN
int     length          number of bytes to copy

USHORT  src_seg         source segment address
        src_off         source offset address
        dst_page        destination page number
        dst_off         destination offset within page
*/
{
        UTINY *to, *temp, *pointer;/* pointers used for copying */
        sys_addr from;

        //STF - performance improvement: if 4k aligned & >= 4k then can use
        // (un)mapview to do exchange.

        if (length <= 64*1024)
            temp = sas_scratch_address(length);
        else
            if ((temp = (unsigned char *)host_malloc(length)) == NULL)
                return(FAILURE);

        pointer = temp;

        from = effective_addr(src_seg, src_off);
        to = (char *)BaseOfLIMMem + dst_page * EMM_PAGE_SIZE + dst_off;

        EM_loads(from, pointer, length);
        EM_stores(from, to, length);
        EM_memcpy(to, pointer, length);

        if (length > 64*1024)
            host_free(temp);

        return(SUCCESS);
}

int host_exchg_EM_to_EM(int length, USHORT src_page, USHORT src_off, USHORT dst_page, USHORT dst_off)
/*   IN
int     length  number of bytes to copy

USHORT  src_page        source page number
        src_off         source offset within page
        dst_page        destination page number
        dst_off         destination offset within page
*/
{
        UTINY *from, *to, *temp, *pointer; /* pointers used for copying */

        if (length <= 64*1024)
            temp = sas_scratch_address(length);
        else
            if ((temp = (unsigned char *)host_malloc(length)) == NULL)
                return(FAILURE);

        pointer = temp;

        from = (char *)BaseOfLIMMem + src_page * EMM_PAGE_SIZE + src_off;
        to = (char *)BaseOfLIMMem + dst_page * EMM_PAGE_SIZE + dst_off;

        EM_memcpy(pointer, from, length);
        EM_memcpy(from, to, length);
        EM_memcpy(to, pointer, length);

        if (length > 64*1024)
            host_free(temp);

        return(SUCCESS);
}


/*
===========================================================================

FUNCTION        : host_get_access_key

PURPOSE         : produces a random access key for use with LIM function 30
                'Enable/Disable OS/E Function Set Functions'

RETURNED STATUS : none

DESCRIPTION     : Two 16 bit random values are required for the 'access key'
                We use the microsecond field from the get time of day routine
                to provide this.

=========================================================================
*/
void host_get_access_key(USHORT access_key[2])
/*  OUT USHORT  access_key[2]   source segment address          */
{
        // do you think we need to seed the random # gen?
        access_key[0] = rand() & 0xffff;
        access_key[1] = rand() & 0xffff;

        return;
}

/***************************************************************************
 * Function:                                                               *
 *	host_reserve_lim_block						   *
 *                                                                         *
 * Description:                                                            *
 *	Called from bios reset() function to reserve address space for	   *	*
 *	LIM page frame							   *	*
 *                                                                         *
 * Parameters:                                                             *
 *	    NONE							   *
 * Return value:                                                           *
 *	    NONE							   *
 *                                                                         *
 ***************************************************************************/
void host_reserve_lim_block(void)
{
    PVOID   Address;
    ULONG   Size;

    emm_start = 0;
    emm_len = 0;

    Size = 0x10000;
    Address = NULL;
    if (ReserveUMB(UMB_OWNER_EMM, &Address, &Size)) {
	emm_start = (sys_addr) Address;
	emm_len = 0x10000;
    }
}

/***************************************************************************
 * Function:                                                               *
 *      host_get_lim_block                                                 *
 *                                                                         *
 * Description:                                                            *
 *      Called from lim memory manager to get the base and length of the   *
 *      gap we've chosen earlier as suitable for a LIM block (or not).     *
 *                                                                         *
 * Parameters:                                                             *
 *      Addresses of start and length variables to be passed on upwards.   *
 *                                                                         *
 * Return value:                                                           *
 *      SUCCESS or FAILURE                                                 *
 *                                                                         *
 ***************************************************************************/
int host_get_lim_block(word *BaseOfLim, unsigned int *BlockLength)
{
    NTSTATUS Status;
    PVOID   Address;
    ULONG   Size;
    if (emm_len == 0x10000 && GetUMBForEMM()) {
        emm_start_seg = (word)(emm_start >> 4);
        *BaseOfLim = emm_start_seg;
        *BlockLength = emm_len;
        return (SUCCESS);
    }
    else {

#ifndef PROD
        fprintf(trace_file, "NTVDM:host_get_lim_block: returning failed as previous failure\n");
#endif
        *BaseOfLim = 0;
	*BlockLength = 0;
        return(FAILURE);
    }
}
#endif /* MONITOR */
#endif /* LIM */
