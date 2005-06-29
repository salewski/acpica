
/******************************************************************************
 * 
 * Module Name: nsaccess - Top-level functions for accessing ACPI namespace
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999, Intel Corp.  All rights 
 * reserved.
 *
 * 2. License
 * 
 * 2.1. Intel grants, free of charge, to any person ("Licensee") obtaining a 
 * copy of the source code appearing in this file ("Covered Code") a license 
 * under Intel's copyrights in the base code distributed originally by Intel 
 * ("Original Intel Code") to copy, make derivatives, distribute, use and 
 * display any portion of the Covered Code in any form; and
 *
 * 2.2. Intel grants Licensee a non-exclusive and non-transferable patent 
 * license (without the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell, 
 * offer to sell, and import the Covered Code and derivative works thereof 
 * solely to the minimum extent necessary to exercise the above copyright 
 * license, and in no event shall the patent license extend to any additions to
 * or modifications of the Original Intel Code.  No other license or right is 
 * granted directly or by implication, estoppel or otherwise;
 *
 * the above copyright and patent license is granted only if the following 
 * conditions are met:
 *
 * 3. Conditions 
 *
 * 3.1. Redistribution of source code of any substantial portion of the Covered 
 * Code or modification must include the above Copyright Notice, the above 
 * License, this list of Conditions, and the following Disclaimer and Export 
 * Compliance provision.  In addition, Licensee must cause all Covered Code to 
 * which Licensee contributes to contain a file documenting the changes 
 * Licensee made to create that Covered Code and the date of any change.  
 * Licensee must include in that file the documentation of any changes made by
 * any predecessor Licensee.  Licensee must include a prominent statement that
 * the modification is derived, directly or indirectly, from Original Intel 
 * Code.
 *
 * 3.2. Redistribution in binary form of any substantial portion of the Covered 
 * Code or modification must reproduce the above Copyright Notice, and the 
 * following Disclaimer and Export Compliance provision in the documentation 
 * and/or other materials provided with the distribution.
 *
 * 3.3. Intel retains all right, title, and interest in and to the Original 
 * Intel Code.
 *
 * 3.4. Neither the name Intel nor any other trademark owned or controlled by 
 * Intel shall be used in advertising or otherwise to promote the sale, use or 
 * other dealings in products derived from or relating to the Covered Code 
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED 
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE 
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE, 
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY 
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY 
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A 
 * PARTICULAR PURPOSE. 
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES 
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR 
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT, 
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY 
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL 
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS 
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY 
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this 
 * software or system incorporating such software without first obtaining any 
 * required license or other approval from the U. S. Department of Commerce or 
 * any other agency or department of the United States Government.  In the 
 * event Licensee exports any such software from the United States or re-
 * exports any such software from a foreign destination, Licensee shall ensure
 * that the distribution and export/re-export of the software is in compliance 
 * with all laws, regulations, orders, or other restrictions of the U.S. Export 
 * Administration Regulations. Licensee agrees that neither it nor any of its 
 * subsidiaries will export/re-export any technical data, process, software, or 
 * service, directly or indirectly, to any country for which the United States 
 * government or any agency thereof requires an export license, other 
 * governmental approval, or letter of assurance, without first obtaining such
 * license, approval or letter.
 *
 *****************************************************************************/


#define __NSACCESS_C__

#include <acpi.h>
#include <amlcode.h>
#include <interpreter.h>
#include <namespace.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>


#define _THIS_MODULE        "nsaccess.c"
#define _COMPONENT          NAMESPACE



static ST_KEY_DESC_TABLE KDT[] = {
    {"0000", 'I', "Package stack not empty at method exit", "Package stack not empty at method exit"},
    {"0001", '1', "Method stack not empty at method exit", "Method stack not empty at method exit"},
    {"0002", 'I', "Object stack not empty at method exit", "Object stack not empty at method exit"},
    {"0003", '1', "Descriptor Allocation Failure", "Descriptor Allocation Failure"},
    {"0004", '1', "root name table allocation failure", "root name table allocation failure"},
    {"0005", '1', "Initial value descriptor allocation failure", "Initial value descriptor allocation failure"},
    {"0006", '1', "Initial value string allocation failure", "Initial value string allocation failure"},
    {"0007", '1', "Scope has no parent", "Scope has no parent"},
    {"0008", '1', "name table overflow", "name table overflow"},
    {"0009", '1', "A name was not found in given scope", "A name was not found in given scope"},
    {"0010", 'W', "Type mismatch", "Type mismatch"},
    {"0011", '1', "Name Table allocation failure", "Name Table allocation failure"},
    {"0012", '1', " Name not found", " Name not found"},
    {NULL, 'I', NULL, NULL}
};


/****************************************************************************
 *
 * FUNCTION:    AcpiExecuteRelativeMethod
 *
 * PARAMETERS:  Handle              - Handle of containing object
 *              *MethodName         - Name of method to execute, If NULL, the
 *                                    handle is the object to execute
 *              *ReturnObject       - Where to put method's return value (if 
 *                                    any).  If NULL, no value is returned.
 *              **Params            - List of parameters to pass to
 *                                    method, terminated by NULL.
 *                                    Params itself may be NULL
 *                                    if no parameters are being
 *                                    passed.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Find and execute the requested method using the handle as a
 *              scope
 *
 ****************************************************************************/

ACPI_STATUS
AcpiExecuteRelativeMethod (NsHandle Handle,
                           char * MethodName,
                           OBJECT_DESCRIPTOR *ReturnObject,
                           OBJECT_DESCRIPTOR **Params)
{
    char                NameBuffer[PATHNAME_MAX];
    ACPI_STATUS         Status;
    UINT32              MaxObjectPathLength = PATHNAME_MAX - 1;


    FUNCTION_TRACE ("AcpiExecuteRelativeMethod");

    /*
     *  Must have a valid handle
     */
    if (!Handle) 
    {
        FUNCTION_EXIT;
        return AE_BAD_PARAMETER;
    }

    /*
     *  If the caller specified a method then it must be a path relative to
     *  the object indicated by the handle we need to reserve space in the
     *  buffer to append the CM name later
     */
    if (MethodName) 
    {
        /*
         *  We will append the method name to the device pathname
         */
        MaxObjectPathLength -= (strlen (MethodName) + 1);
    }

    /*
     *  Get the device pathname
     */
    Status = NsHandleToPathname (Handle, MaxObjectPathLength, NameBuffer);
    if (Status != AE_OK) 
    {
        /*
         *  Failed the conversion
         */
        FUNCTION_EXIT;
        return Status;
    }

    /*
     *  If the caller specified a method then it must be a path relative to
     *  the object indicated by the handle
     */
    if (MethodName) 
    {
        /*
         * Append the method name to the device pathname
         * (Path separator is a period) 
         */
        strcat (NameBuffer, ".");
        strcat (NameBuffer, MethodName);
    }

    /*
     *  Execute the method
     */
    Status = AcpiExecuteMethod (NameBuffer, ReturnObject, Params);

    FUNCTION_EXIT;
    return Status;
}


/****************************************************************************
 *
 * FUNCTION:    AcpiExecuteMethod
 *
 * PARAMETERS:  *MethodName         - Name of method to execute
 *              *ReturnObject       - Where to put method's return value (if 
 *                                    any).  If NULL, no value is returned.
 *              **Params            - List of parameters to pass to
 *                                    method, terminated by NULL.
 *                                    Params itself may be NULL
 *                                    if no parameters are being
 *                                    passed.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Find and execute the requested method passing the given
 *              parameters
 *
 ****************************************************************************/

ACPI_STATUS
AcpiExecuteMethod (char * MethodName, OBJECT_DESCRIPTOR *ReturnObject,
                    OBJECT_DESCRIPTOR **Params)
{
    ACPI_STATUS         Status = AE_ERROR;
    nte                 *MethodPtr = NULL;
    OBJECT_DESCRIPTOR   *ObjDesc;




    FUNCTION_TRACE ("AcpiExecuteMethod");


    /* Parameter validation */

    if (!RootObject->Scope)
    {
        /* 
         * If the name space has not been initialized, the Method has surely
         * not been defined and there is nothing to execute.
         */

        DEBUG_PRINT (ACPI_ERROR, ("Name space not initialized ==> method not defined\n"));
        FUNCTION_EXIT;
        return AE_NO_NAMESPACE;
    }

    if (!MethodName)
    {
        DEBUG_PRINT (ACPI_ERROR, ("AcpiExecuteMethod: MethodName is NULL\n"));
        FUNCTION_EXIT;
        return AE_BAD_PARAMETER;
    }


    if (ReturnObject)
    {
        /* Initialize the return value */

        memset (ReturnObject, 0, sizeof (OBJECT_DESCRIPTOR));
    }

    if (RootObject->Scope && MethodName)
    {   
        /*  Root and MethodName valid   */

        if (MethodName[0] != '\\' || MethodName[1] != '/')
        {
            MethodName = NsInternalizeName (MethodName);
        }

        /* See comment near top of file re significance of FETCH_VALUES */

#ifdef FETCH_VALUES
        Status = NsEnter (MethodName, TYPE_Any, MODE_Exec, (NsHandle *) &MethodPtr);
#else 
        Status = NsEnter (MethodName, TYPE_Method, MODE_Exec, (NsHandle *) &MethodPtr);
#endif

        if (Status != AE_OK)
        {
            DEBUG_PRINT (ACPI_INFO, ("Method [%s] was not found, status=%.4X\n",
                            MethodName, Status));
        }
    }

    if (RootObject->Scope && MethodName && (Status == AE_OK))
    {   
        /*  Root, MethodName, and Method valid  */

#ifdef FETCH_VALUES
        if (NsGetType (MethodPtr) == TYPE_Method)
        {
            /* Method points to a method name */
        
            DEBUG_PRINT (ACPI_INFO,
                        ("[%s Method %p Value %p\n",
                        MethodName, MethodPtr, MethodPtr->Value));

            if (MethodPtr->Value)
            {
                DEBUG_PRINT (ACPI_INFO,
                            ("Offset %x Length %lx]\n",
                            ((meth *) MethodPtr->Value)->Offset + 1,
                            ((meth *) MethodPtr->Value)->Length - 1));
            }
        
            else
            {
                DEBUG_PRINT (ACPI_INFO, ("*Undefined*]\n"));
            }
#endif

            /* 
             * Here if not FETCH_VALUES (and hence only Method was looked for), or
             * FETCH_VALUES and the name found was in fact a Method.  In either
             * case, set the current scope to that of the Method, and execute it.
             */

            if (!MethodPtr->Value)
            {
                DEBUG_PRINT (ACPI_ERROR, ("Method is undefined\n"));
            }

            else
            {
                NsDumpPathname (MethodPtr->Scope, "AcpiExecuteMethod: Setting scope to", 
                                TRACE_NAMES, _COMPONENT);

                /* Reset the current scope to the beginning of scope stack */

                CurrentScope = &ScopeStack[0];

                /* Push current scope on scope stack and make Method->Scope current  */

                NsPushCurrentScope (MethodPtr->Scope, TYPE_Method);

                NsDumpPathname (MethodPtr, "AcpiExecuteMethod: Executing", 
                                TRACE_NAMES, _COMPONENT);

                DEBUG_PRINT (TRACE_NAMES, ("At offset %8XH\n",
                                  ((meth *) MethodPtr->Value)->Offset + 1));
        
                AmlClearPkgStack ();
                ObjStackTop = 0;    /* Clear object stack */
                

                /* 
                 * Excecute the method here 
                 */
                Status = AmlExecuteMethod (
                                 ((meth *) MethodPtr->Value)->Offset + 1,
                                 ((meth *) MethodPtr->Value)->Length - 1,
                                 Params);

                if (AmlPackageNested ())
                {
                    /*  Package stack not empty at method exit and should be  */

                    REPORT_INFO (&KDT[0]);
                }

                if (AmlGetMethodDepth () > -1)
                {
                    /*  Method stack not empty at method exit and should be */

                    REPORT_ERROR (&KDT[1]);
                }

                if (ObjStackTop)
                {
                    /* Object stack is not empty at method exit and should be */

                    REPORT_INFO (&KDT[2]);
                    AmlDumpStack (MODE_Exec, "Remaining Object Stack entries", -1, "");
                }

                DEBUG_PRINT (ACPI_INFO, ("*** Completed execution of method %s ***\n",
                                MethodName));
            }

#ifdef FETCH_VALUES
        }
    
        else
        {
            /*
             * Method points to a name that is not a method
             * Here if FETCH_VALUES and the name found was not a Method.
             * Return its value.
             */
            ObjDesc = AllocateObjectDesc (&KDT[3]);
            if (ObjDesc)
            {
                /* Construct a descriptor pointing to the name */
            
                ObjDesc->Lvalue.ValType = (UINT8) TYPE_Lvalue;
                ObjDesc->Lvalue.OpCode  = (UINT8) AML_NameOp;
                ObjDesc->Lvalue.Ref     = (void *) MethodPtr;

                /* 
                 * Put it on the stack, and use AmlGetRvalue() to get the value.
                 * Note that ObjStackTop points to the top valid entry, not to
                 * the first unused position.
                 */

                LocalDeleteObject ((OBJECT_DESCRIPTOR **) &ObjStack[ObjStackTop]);
                ObjStack[ObjStackTop] = (void *) ObjDesc;

                /* This causes ObjDesc (allocated above) to always be deleted */

                Status = AmlGetRvalue ((OBJECT_DESCRIPTOR **) &ObjStack[ObjStackTop]);

                /* 
                 * If AmlGetRvalue() succeeded, treat the top stack entry as
                 * a return value.
                 */

                if (AE_OK == Status)
                {
                    Status = AE_RETURN_VALUE;
                }
            }

            else
            {
                /* Descriptor allocation failure */

                Status = AE_NO_MEMORY;
            }
        }
#endif



        /* TBD: Unecessary mapping? */

        if (AE_AML_ERROR == Status)
        {
            Status = AE_ERROR;
        }
    
        if (AE_RETURN_VALUE == Status)
        {
            /* 
             * If the Method returned a value and the caller provided a place
             * to store a returned value, Copy the returned value to the object
             * descriptor provided by the caller.
             */

            if (ReturnObject)
            {
                (*ReturnObject) = *((OBJECT_DESCRIPTOR *) ObjStack[ObjStackTop]);            
            }
        
            /* 
             * TBD: do we ever want to delete this??? 
             * There are clearly cases that we don't and this will fault
             */

            /* OsdFree (ObjStack[ObjStackTop]); */

            Status = AE_OK;
        }
    }

    FUNCTION_EXIT;
    return Status;
}



/****************************************************************************
 * 
 * FUNCTION:    NsSetup()
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Allocate and initialize the root name table
 *
 ***************************************************************************/

ACPI_STATUS
NsSetup (void)
{
    ACPI_STATUS             Status;
    PREDEFINED_NAMES        *InitVal = NULL;
    NsHandle                handle;


    FUNCTION_TRACE ("NsSetup");


    /* 
     * Root is initially NULL, so a non-NULL value indicates
     * that NsSetup() has already been called; just return.
     */

    if (RootObject->Scope)
    {
        FUNCTION_EXIT;
        return AE_OK;
    }

    /* Allocate a root scope table */

    RootObject->Scope = NsAllocateNteDesc (NS_TABLE_SIZE);
    if (!RootObject->Scope)
    {
        /*  root name table allocation failure  */

        REPORT_ERROR (&KDT[4]);
        FUNCTION_EXIT;
        return AE_NO_MEMORY;
    }

    /* 
     * Init the root scope first entry -- since it is the exemplar of 
     * the scope (Some fields are duplicated to new entries!) 
     */
    NsInitializeTable (RootObject->Scope, NULL, RootObject);

    /* Push the root name table on the scope stack */
    
    ScopeStack[0].Scope = RootObject->Scope;
    ScopeStack[0].Type = TYPE_Any;
    CurrentScope = &ScopeStack[0];

    /* Enter the pre-defined names in the name table */
    
    DEBUG_PRINT (ACPI_INFO, ("Entering predefined name table into namespace\n"));

    for (InitVal = PreDefinedNames; InitVal->Name; InitVal++)
    {
        Status = NsEnter (InitVal->Name, InitVal->Type, MODE_Load, &handle);

        /* 
         * if name entered successfully
         *  && its entry in PreDefinedNames[] specifies an initial value
         */
        
        if ((Status == AE_OK) && handle && InitVal->Val)
        {
            /* Entry requests an initial value, allocate a descriptor for it. */
            
            OBJECT_DESCRIPTOR       *ObjDesc = AllocateObjectDesc (&KDT[5]);

            if (!ObjDesc)
            {
                FUNCTION_EXIT;
                return AE_NO_MEMORY;
            }

            else
            {
                ObjDesc->ValType = (UINT8) InitVal->Type;

                /* 
                 * Convert value string from table entry to internal representation.
                 * Only types actually used for initial values are implemented here.
                 */

                switch (InitVal->Type)
                {
                case TYPE_Number:
                    ObjDesc->Number.Number = (UINT32) atol (InitVal->Val);
                    break;

                case TYPE_String:
                    ObjDesc->String.StrLen = (UINT16) strlen (InitVal->Val);

                    /* 
                     * XXX - if this OsdAllocate() causes a garbage collection pass,
                     * XXX - ObjDesc will get deleted since no NT points to it yet.
                     * XXX - This "should not happen" during initialization.
                     */

                    ObjDesc->String.String = OsdAllocate ((size_t) (ObjDesc->String.StrLen + 1));
                    if (!ObjDesc->String.String)
                    {
                        REPORT_ERROR (&KDT[6]);
                        FUNCTION_EXIT;
                        return AE_NO_MEMORY;
                    }
                    else
                    {
                        strcpy ((char *) ObjDesc->String.String, InitVal->Val);
                    }
                    
                    break;

                default:
                    OsdFree (ObjDesc);
                    ObjDesc = NULL;
                    continue;
                }

                /* Store pointer to value descriptor in nte */
            
                NsSetValue(handle, ObjDesc, ObjDesc->ValType);
            }
        }
    }

#ifdef PLUMBER
    RegisterMarkingFunction (NsMarkNS);
#endif

    FUNCTION_EXIT;
    return AE_OK;
}


/****************************************************************************
 *
 * FUNCTION:    NsEnter
 *
 * PARAMETERS:  Name        - Name to be entered, in internal format
 *                            as represented in the AML stream
 *              Type        - Type associated with name
 *              LoadMode    - MODE_Load => add name if not found
 *              RetHandle   - Where the new handle is placed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Find or enter the passed name in the name space.
 *              Log an error if name not found in Exec mode.
 *
 *
 ***************************************************************************/

ACPI_STATUS
NsEnter (char *Name, NsType Type, OpMode LoadMode, NsHandle *RetHandle)
{
    ACPI_STATUS     Status;
    nte             *EntryToSearch = NULL;
    nte             *ThisEntry = NULL;
    nte             *ScopeToPush = NULL;
    INT32           NumSegments;
    INT32           NullNamePath = FALSE;
    NsType          TypeToCheckFor;              /* Type To Check For */


    FUNCTION_TRACE ("NsEnter");


    if (!RetHandle)
    {
        FUNCTION_EXIT;
        return AE_BAD_PARAMETER;
    }

    *RetHandle = NOTFOUND;
    if (!RootObject->Scope)
    {
        /* 
         * If the name space has not been initialized:
         * -  In Pass1 of Load mode, we need to initialize it
         *    before trying to define a name.
         * -  In Exec mode, there are no names to be found.
         */

        if (MODE_Load1 == LoadMode)
        {
            if ((Status = NsSetup ()) != AE_OK)
            {
                FUNCTION_EXIT;
                return Status;
            }
        }
        else
        {
            FUNCTION_EXIT;
            return AE_NOT_FOUND;
        }
    }

    if (!Name)
    {
        /* Invalid parameter */

        FUNCTION_EXIT;
        return AE_BAD_PARAMETER;
    }

    DEBUG_PRINT (TRACE_NAMES,
                    ("NsEnter: Name[0-5] - %02x %02x %02x %02x %02x %02x \n",
                    Name[0], Name[1], Name[2], Name[3], Name[4], Name[5]));

    CheckTrash ("enter NsEnter");

    /* DefFieldDefn and BankFieldDefn define fields in a Region */
    
    if (TYPE_DefFieldDefn == Type || TYPE_BankFieldDefn == Type)
    {
        TypeToCheckFor = TYPE_Region;
    }
    else
    {
        TypeToCheckFor = Type;
    }

    /* 
     * Check for prefixes.  As represented in the AML stream, a Name consists
     * of an optional scope prefix followed by a segment part.
     *
     * If present, the scope prefix is either a RootPrefix (in which case the
     * name is fully qualified), or one or more ParentPrefixes (in which case
     * the name's scope is relative to the current scope).
     *
     * The segment part consists of:
     *  + a single 4-byte name segment, or
     *  + a DualNamePrefix followed by two 4-byte name segments, or
     *  + a MultiNamePrefixOp, followed by a byte indicating the number of
     *    segments and the segments themselves.
     */

    if (*Name == AML_RootPrefix)
    {
        /* Name is fully qualified, look in root name table */
        
        DEBUG_PRINT (TRACE_NAMES, ("root \n"));
        
        EntryToSearch = RootObject->Scope;
        Name++;                 /* point to segment part */
    }
    
    else
    {
        /* Name is relative to current scope, start there */
        
        EntryToSearch = CurrentScope->Scope;

        while (*Name == AML_ParentPrefix)
        {
            DEBUG_PRINT (TRACE_NAMES, ("parent \n"));
            
            /* point to segment part or next ParentPrefix */

            Name++;     

            /*  Backup to the parent's scope  */
            
            EntryToSearch = EntryToSearch->ParentScope;
            if (!EntryToSearch)
            {
                /* Current scope has no parent scope */
                
                REPORT_ERROR (&KDT[7]);
                CheckTrash ("leave NsEnter NOTFOUND 1");

                FUNCTION_EXIT;
                return AE_NOT_FOUND;
            }
        }
    }

    if (*Name == AML_DualNamePrefix)
    {
        DEBUG_PRINT (TRACE_NAMES, ("Dual Name \n"));

        NumSegments = 2;
        Name++;                             /* point to first segment */
    }
    
    else if (*Name == AML_MultiNamePrefixOp)
    {
        DEBUG_PRINT (TRACE_NAMES, ("Multi Name %d \n", Name[1]));
        
        NumSegments = (INT32)* (UINT8 *) ++Name;
        Name++;                             /* point to first segment */
    }

    else if (!Name)
    {
        /*  8-12-98 ASL Grammar Update supports null NamePath   */

        NullNamePath = TRUE;
        NumSegments = 0;
        ThisEntry = RootObject;
    }

    else
    {
        /* 
         * No Dual or Multi prefix, hence there is only one
         * segment and Name is already pointing to it.
         */
        NumSegments = 1;
    }

    DEBUG_PRINT (TRACE_NAMES, ("Segments = %d \n", NumSegments));

    while (NumSegments-- && EntryToSearch)
    {
        /*  loop through and verify/add each name segment   */
        
        CheckTrash ("before NsSearchTable");
        
        /* 
         * Search for the current segment in the table where it should be.
         * Type is significant only at the last level.
         */

        Status = NsSearchAndEnter (Name, EntryToSearch, LoadMode,
                                    NumSegments == 0 ? Type : TYPE_Any, &ThisEntry);
        CheckTrash ("after NsSearchTable");

        if (Status != AE_OK)
        {
            if (Status == AE_NOT_FOUND)
            {
                /*  name not in ACPI namespace  */

                if (MODE_Load1 == LoadMode || MODE_Load == LoadMode)
                {
                    REPORT_ERROR (&KDT[8]);
                }

                else
                {
                    DEBUG_PRINT (TRACE_NAMES, ("Name not found in this scope\n"));
                }

                CheckTrash ("leave NsEnter NOTFOUND 2");
            }

            FUNCTION_EXIT;
            return Status;
        }

        if (NumSegments         == 0  &&                    /* if last segment                  */
            TypeToCheckFor      != TYPE_Any &&              /* and looking for a specific type  */
            TypeToCheckFor      != TYPE_DefAny &&           /* which is not a phoney type       */
            TypeToCheckFor      != TYPE_Scope &&            /*   "   "   "  "   "     "         */
            TypeToCheckFor      != TYPE_IndexFieldDefn &&   /*   "   "   "  "   "     "         */
            ThisEntry->Type     != TYPE_Any &&              /* and type of entry is known       */
            ThisEntry->Type     != TypeToCheckFor)          /* and entry does not match request */
        {                                                   /* complain.                        */
            /*  complain about type mismatch    */

            REPORT_WARNING (&KDT[10]);
        }

        /*
         * If last segment and not looking for a specific type, but type of
         * found entry is known, use that type to see if it opens a scope.
         */

        if ((0 == NumSegments) && (TYPE_Any == Type))
        {
            Type = ThisEntry->Type;
        }

        if ((NumSegments || NsOpensScope (Type)) &&
            (ThisEntry->Scope == (nte *) 0))
        {
            /* 
             * More segments or the type implies enclosed scope, 
             * and the next scope has not been allocated.
             */

            DEBUG_PRINT (ACPI_INFO, ("Load mode= %d  ThisEntry= %x\n", LoadMode, ThisEntry));

            if ((MODE_Load1 == LoadMode) || (MODE_Load == LoadMode))
            {   
                /*  first or second pass load mode ==> locate the next scope    */
                
                DEBUG_PRINT (TRACE_NAMES, ("Creating/adding a new scope\n"));
                ThisEntry->Scope = NsAllocateNteDesc (NS_TABLE_SIZE);

                if (!ThisEntry->Scope)
                {
                    FUNCTION_EXIT;
                    return AE_NO_MEMORY;
                }
            }

            /* Now complain if there is no next scope */
            
            if (ThisEntry->Scope == (nte *) 0)
            {
                DEBUG_PRINT (ACPI_ERROR, ("No child scope at entry %p\n", ThisEntry));

                if (MODE_Load1 == LoadMode || MODE_Load == LoadMode)
                {
                    REPORT_ERROR (&KDT[11]);
                    FUNCTION_EXIT;
                    return AE_NOT_FOUND;
                }

                REPORT_ERROR (&KDT[12]);
                CheckTrash ("leave NsEnter NOTFOUND 3");
                FUNCTION_EXIT;
                return AE_NOT_FOUND;
            }


            /* Scope table initialization */

            if (MODE_Load1 == LoadMode || MODE_Load == LoadMode)
            {
                /* Initialize the new table */

                NsInitializeTable (ThisEntry->Scope, EntryToSearch, ThisEntry);
            }
        }

        EntryToSearch = ThisEntry->Scope;
        Name += 4;                        /* point to next name segment */
    }

    /* 
     * If entry is a type which opens a scope,
     * push the new scope on the scope stack.
     */

    if (NsOpensScope (TypeToCheckFor))
    {
        /*  8-12-98 ASL Grammar Update supports null NamePath   */

        if (NullNamePath)   
            ScopeToPush = ThisEntry;
        else
            ScopeToPush = ThisEntry->Scope;


        CheckTrash ("before NsPushCurrentScope");
        NsPushCurrentScope (ScopeToPush, Type);
        CheckTrash ("after  NsPushCurrentScope");
    }

    *RetHandle = (NsHandle) ThisEntry;
    FUNCTION_EXIT;
    return AE_OK;
}

