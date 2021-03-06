%{
//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  mibyacc.y
//  
//  Copyright 1992 Technology Dynamics, Inc.
//  
//  All Rights Reserved!!!
//  
//	This source code is CONFIDENTIAL and PROPRIETARY to Technology 
//	Dynamics. Unauthorized distribution, adaptation or use may be 
//	subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//  
//  mibyacc.y contains the grammer used by yacc to generate mibyacc.c
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.3  $
//  $Date:   01 Jul 1992 13:28:26  $
//  $Author:   bobo  $
//
//  $Log:   N:/agent/mgmtapi/vcs/mibyacc.y_v  $
//  
//     Rev 1.3   01 Jul 1992 13:28:26   bobo
//  Made error messages go to error_out (which is stdout) instead of stderr.
//  
//     Rev 1.2   14 Jun 1992 21:50:50   bobo
//  Changed rule for DEFVAL so the curlys are always required.
//  
//     Rev 1.1   14 Jun 1992 20:57:42   bobo
//  Improved handling of sub ranges.
//  Added productions for dotted names where just names were expected.
//  Corrected the enumeration production to use the enumList.
//  Added action to return with not successful code if had any number of errors.
//  
//     Rev 1.0   14 Jun 1992 19:15:12   bobo
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/mgmtapi/vcs/mibyacc.y_v  $ $Revision:   1.3  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------
//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "mibcc.h"
#include "mibtree.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

lpTreeNode lpCurrentNode;
unsigned int nNumberOfErrors=0;
unsigned int nNumberOfWarnings=0;

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

#define fYYErrorStopReturn(x) \
   if (nNumberOfErrors>=nStopAfterErrors) { \
      fprintf (error_out, "%i error(s) found.  Compiliation stopped.\n", \
               nNumberOfErrors); \
      return (x); \
   } else

//! NOTE we are defining yyunion to do a structure copy
#define yyunion(a,b)   *(a) = *(b)

//--------------------------- PUBLIC PROCEDURES -----------------------------

extern int yylex(void);	/* resides in miblex.c (generated by flex) */

int yyerror(char *s) {
   fprintf (error_out, "%s on line %i\n", s, lineno);
   nNumberOfErrors++;
   return 1;
}

%}

%start mib

%union {
   unsigned long numval;
   char strval[256];
}

%token	kwACCESS
%token	kwBEGIN
%token	kwCHOICE	kwCOUNTER
%token	kwDEFAULT	kwDEFINITIONS	kwDEFVAL	kwDEPRECATED
%token	kwDESCRIPTION
%token	kwEND		kwENTERPRISE	kwEXPORTS
%token	kwFROM
%token	kwGAUGE
%token	kwIDENTIFIER	kwIMPORTS	kwINDEX		kwINTEGER
%token	kwIPADDRESS	kwIMPLICIT
%token	kwMANDATORY	kwMACRO
%token	kwNETWORKADDRESS		kwNOT_ACCESSIBLE
%token	kwNOTATION	kwNULL
%token	kwOBJECT	kwOBJECT_TYPE	kwOBSOLETE	kwOCTET
%token	kwOF		kwOPAQUE	kwOPTIONAL
%token	kwREAD_ONLY	kwREAD_WRITE	kwREFERENCE
%token	kwSEQUENCE	kwSIZE		kwSTATUS
%token	kwSTRING	kwSYNTAX
%token	kwTIMETICKS	kwTRAP_TYPE	kwTYPE
%token	kwVARIABLES	kwVALUE
%token	kwWRITE_ONLY

%token	tokDOTDOT	/* ".." */
%token	tokIS		/* "::=" */

%token	<strval> tokNAME
%token	<numval> tokNUMBER
%token	tokSTRING
%token	tokBINARYSTR
%token	tokHEXSTR
%token	tokIP

%%   /*start of rules*/
mib		:	moduleList { if (0 != nNumberOfErrors) return 1; }
		;

moduleList	:	module
		|	moduleList module
		;

module		:	moduleHeader optExports optImports
			definitionList
			kwEND
		;

moduleHeader	:	tokNAME optID kwDEFINITIONS tokIS kwBEGIN
		;

optID		:	
		|	'{'
			{ if (warning_level(2))
				fprintf (error_out, "(i%) : warning : optional module object identifier is ignored.", lineno);
			}
			oidValueNoAct '}'
		;

optImports	:
		|	kwIMPORTS importsList ';'
		;

importsList	:	imports
		|	importsList imports
		;

imports		:	importNameList kwFROM tokNAME
		;

importNameList	:	importName
		|	importNameList ',' importName
		;

importName	:	tokNAME
		|	rfc1155_macro
		|	rfc1155_export
		;

rfc1155_macro	:	kwOBJECT_TYPE
		;
		
rfc1155_export	:	kwNETWORKADDRESS
		|	kwIPADDRESS
		|	kwCOUNTER
		|	kwGAUGE
		|	kwTIMETICKS
		|	kwOPAQUE
		;

optExports	:
		|	kwEXPORTS exportNameList ';'
		;

exportNameList	:	exportName
		|	exportNameList exportName
		;

exportName	:	tokNAME
		|	rfc1155_macro
		|	rfc1155_export
		|	exportName ',' tokNAME
		|	exportName ',' rfc1155_macro
		|	exportName ',' rfc1155_export
		;

definitionList	:	definition
		|	definitionList definition
		;

definition	:	defOID
		|	defOTYPE
		|	defTRAPTYPE
		|	defTYPE
		|	defMACRO
		;

defOID		:	tokNAME kwOBJECT kwIDENTIFIER tokIS '{' 
			{ lpCurrentNode = lpMIBRoot; }
			oidValue tokNUMBER '}'
			{ InsertChildNode (lpCurrentNode, NewChildNode ($1, (UINT) $8));
			  fYYErrorStopReturn(1);
			}
		;
		
defOTYPE	:	tokNAME kwOBJECT_TYPE
				kwSYNTAX asn1Type
				kwACCESS objectAccess
				kwSTATUS objectStatus
				optDescription
				optReference
				optIndex
				optDefval
				tokIS '{'
				{ lpCurrentNode = lpMIBRoot; }
				oidValue tokNUMBER '}'
				{ InsertChildNode (lpCurrentNode, NewChildNode ($1, (UINT) $17));
				  fYYErrorStopReturn(1);
				}
		;

defTRAPTYPE	:	tokNAME kwTRAP_TYPE
				kwENTERPRISE dottedNAME
				optVariable
				optDescription
				optReference
				tokIS tokNUMBER
		;

dottedNAME	:	tokNAME
		|	dottedNAME '.' tokNAME

defTYPE		:	tokNAME tokIS optTag asn1Type
		;

optTag		:
		|	'[' tokNAME tokNUMBER']'
		|	'[' tokNAME tokNUMBER']' kwIMPLICIT
		;

/*
tagType		:	kwUNIVERSAL
		|	kwAPPLICATION
		|	kwPRIVATE
		|	tokNAME
		;
*/

defMACRO	:	kwOBJECT_TYPE kwMACRO tokIS kwBEGIN macroSyntax kwEND
		|	tokNAME kwMACRO tokIS kwBEGIN macroSyntax kwEND
		;

macroSyntax	:	typeNotation valueNotation supportSyntax
		;

typeNotation	:	kwTYPE kwNOTATION tokIS typeSyntax
		;

valueNotation	:	kwVALUE kwNOTATION tokIS valueSyntax
		;

typeSyntax	:	macroType
		|	typeSyntax macroType
		;

macroType	:	tokSTRING tokNAME
		|	tokSTRING tokNAME '(' tokNAME ')'
		|	tokSTRING tokNAME '(' kwTYPE tokNAME ')'
		;

/*
optTypeClause	:
		|	'(' tokNAME ')'
		|	'(' kwTYPE tokNAME ')'
		;
*/

valueSyntax	:	tokNAME '(' kwVALUE tokNAME ')'
		;

supportSyntax	:	supportItem
		|	supportSyntax supportItem
		;

supportItem	:	tokNAME tokIS macroTokens
		;

macroTokens	:	tokSTRING
		|	tokSTRING '|' macroTokens
		;

asn1Type	:	kwSEQUENCE kwOF tokNAME
		|	kwSEQUENCE '{' sequence '}'
		|	kwCHOICE '{' choice '}'
		|	kwINTEGER '{' enumList '}' 
		|	simpleSyntax
		;

sequence	:	sequenceItem
		|	sequence ',' sequenceItem
		;

sequenceItem	:	choiceItem optOptional optDefault
		;

choice		:	choiceItem
		|	choice ',' choiceItem
		;
		
choiceItem	:	tokNAME 
		|	tokNAME simpleSyntax
		;

enumList	:	enum
		|	enumList ',' enum
		;

enum		:	tokNAME '(' tokNUMBER ')'
		;

optOptional	:
		|	kwOPTIONAL
		;

optDefault	:
		|	kwDEFAULT defval
		;

simpleSyntax	:	simpleIndex
		|	kwINTEGER '(' signedSubRange ')'
		|	kwNULL
		|	rfc1155_export
		|	tokNAME '(' kwSIZE '(' subRange ')' ')'
		|	tokNAME '(' kwSIZE '(' subRangeValue ')' ')'
		|	tokNAME '(' signedSubRange ')'
		;

objectAccess	:	kwREAD_ONLY
		|	kwREAD_WRITE
		|	kwWRITE_ONLY
		|	kwNOT_ACCESSIBLE
		|	tokNAME
		;
		
objectStatus	:	kwOPTIONAL
		|	kwMANDATORY
		|	kwDEPRECATED
		|	kwOBSOLETE
		|	tokNAME
		;
		
optDescription	:
		|	kwDESCRIPTION tokSTRING
		;

optIndex	:
		|	kwINDEX '{' indexItemList '}'
		;

optReference	:
		|	kwREFERENCE tokSTRING
		;

optVariable	:
		|	kwVARIABLES '{' varList '}'
		;

varList		:	var
		|	varList ',' var
		;

var		:	dottedNAME
		;

indexItemList	:	indexItem
		|	indexItemList ',' indexItem
		;

indexItem	:	simpleIndex
		|	extendedIndex
		;

simpleIndex	:	tokNAME
		|	kwINTEGER
		|	kwINTEGER '(' subRange ')'
		|	kwOCTET kwSTRING
		|	kwOCTET kwSTRING '(' kwSIZE '(' subRangeValue ')' ')'
		|	kwOCTET kwSTRING '(' kwSIZE '(' subRange ')' ')'
		|	kwOBJECT kwIDENTIFIER
		;

extendedIndex	:	kwNETWORKADDRESS
		|	kwIPADDRESS
		;

subRange	:	subRangeValue tokDOTDOT subRangeValue
		;

signedSubRange	:	'-' subRangeValue tokDOTDOT subRangeValue
		|	'-' subRangeValue tokDOTDOT '-' subRangeValue
		|	subRange
		;

subRangeValue	:	tokNUMBER
		|	tokBINARYSTR
		|	tokHEXSTR
		;

optDefval	:
		|	kwDEFVAL '{' defval '}'
		;

defval		:	tokSTRING
		|	tokBINARYSTR
		|	tokHEXSTR
		|	tokNUMBER
		|	'-' tokNUMBER
		|	oidValueNoAct
		|	tokIP
		;

oidValue	:	tokNAME
			{
				lpCurrentNode = FindNodeByName (lpCurrentNode, $1);
				if (NULL == lpCurrentNode) {
				   fprintf (error_out, "error: sub-identifier '%s' not found in tree.  ", $1);
				   yyerror ("Unknown sub-identifier");
				   fYYErrorStopReturn(1);
				}
			}
		|	tokNAME '(' tokNUMBER ')'
			{
				lpCurrentNode = FindNodeAddToTree (lpCurrentNode, $1, (UINT) $3);
				if (NULL == lpCurrentNode) {
				   fprintf (error_out, "error: sub-identifier '%s(%i)' not found in tree.  ", $1, $3);
				   yyerror ("Unknown sub-identifier");
				   fYYErrorStopReturn(1);
				}
			}
		|	tokNUMBER
			{
				lpCurrentNode = FindNodeByNumber (lpCurrentNode, (UINT) $1); 
				if (NULL == lpCurrentNode) {
				   fprintf (error_out, "error: sub-identifier '%s' not found in tree.  ", $1);
				   yyerror ("Unknown sub-identifier");
				   fYYErrorStopReturn(1);
				}
			}
		|	oidValue tokNAME
			{
				lpCurrentNode = FindNodeByName (lpCurrentNode, $2);
				if (NULL == lpCurrentNode) {
				   fprintf (error_out, "error: sub-identifier '%s' not found in tree.  ", $2);
				   yyerror ("Unknown sub-identifier");
				   fYYErrorStopReturn(1);
				}
			}
		|	oidValue tokNAME '(' tokNUMBER ')'
			{
				lpCurrentNode = FindNodeAddToTree (lpCurrentNode, $2, (UINT) $4);
				if (NULL == lpCurrentNode) {
				   fprintf (error_out, "error: sub-identifier '%s(%i)' not found in tree.  ", $2, $4);
				   yyerror ("Unknown sub-identifier");
				   fYYErrorStopReturn(1);
				}
			}
		|	oidValue tokNUMBER
			{
				lpCurrentNode = FindNodeByNumber (lpCurrentNode, (UINT) $2);
				if (NULL == lpCurrentNode) {
				   fprintf (error_out, "error: sub-identifier '%s' not found in tree.  ", $2);
				   yyerror ("Unknown sub-identifier");
				   fYYErrorStopReturn(1);
				}
			}
		;

oidValueNoAct	:	tokNAME
		|	tokNAME '(' tokNUMBER ')'
		|	tokNUMBER
		|	oidValueNoAct tokNAME
		|	oidValueNoAct tokNAME '(' tokNUMBER ')'
		|	oidValueNoAct tokNUMBER
		;

/*--------------- END OF FILE: mibyacc.y ---------------------------------*/

