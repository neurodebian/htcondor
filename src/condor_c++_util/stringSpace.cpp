/***************************************************************
 *
 * Copyright (C) 1990-2007, Condor Team, Computer Sciences Department,
 * University of Wisconsin-Madison, WI.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************/


#include "condor_common.h"
#include "HashTable.h"
#include "stringSpace.h"
#include "condor_debug.h"



StringSpace::
StringSpace (int initial_size)
{
	SSStringEnt filler;
	stringSpace = new HashTable<YourSensitiveString,int>(
		(int) (1.25 * initial_size),
		&YourSensitiveString::hashFunction );

	// initiliaze the string table
	filler.inUse     = false;
	filler.string    = NULL;
	filler.adoptMode = SS_INVALID;
	filler.refCount  = 0;
	strTable.fill (filler);

	// Set up our tracking of the strTable
	first_free_slot = 0;
	highest_used_slot = -1;
	number_of_slots_filled = 0;
	// current = 0; 
	return;
}


StringSpace::
~StringSpace ()
{
	purge ();
	delete stringSpace;
	return;
}


void StringSpace::
purge ()
{
	// free all strings in the string table
	int i;
	for (i = 0; i <= highest_used_slot; i++) 
	{
		if (strTable[i].inUse && strTable[i].string)
		{
			bool did_delete = false;
			switch (strTable[i].adoptMode)
			{
				case SS_DUP:
				case SS_ADOPT_C_STRING:
					free (strTable[i].string);
					did_delete = true;
					break;

				case SS_ADOPT_CPLUS_STRING:
					delete [] (strTable[i].string);
					did_delete = true;
					break;

				case SS_INVALID:
					break;

				default:
					// some problem --- may cause a leak
					break;
			}
			if (did_delete)
			{
				strTable[i].string = NULL;
				strTable[i].inUse = false;
				strTable[i].refCount = 0;
				strTable[i].adoptMode = SS_INVALID;
			}
		}
		strTable[i].adoptMode = SS_INVALID;
	}

	first_free_slot = 0;
	highest_used_slot = -1;
	number_of_slots_filled = 0;

	// clean up the hash table
	stringSpace->clear();
	return;
}

int StringSpace::
getCanonical (char* &str, StringSpaceAdoptionMethod adopt)
{
	// sanity check
	if (!str) return -1;

	YourSensitiveString yourStr(str);
	int index;

	// case 1:  already exists in space
	if (stringSpace->lookup (yourStr, index) == 0) 
	{
		switch (adopt)
        {
            case SS_ADOPT_C_STRING: 
				free(str);  
				str = NULL;
				break;
            case SS_ADOPT_CPLUS_STRING: 
				delete [] (str);  
				str = NULL;
				break;
            case SS_DUP:
            default:
                break;
		}
		// increment reference count
		strTable[index].refCount++;
		return index;
	}

	// case 2:  new string --- add to space
    index = first_free_slot;
	if (adopt == SS_DUP) {
		strTable[index].string  = strdup(str);
	} else {
		strTable[index].string  = str;
		str = NULL;
	}
	strTable[index].inUse       = true;
    strTable[index].refCount    = 1;
    strTable[index].adoptMode   = adopt;
	number_of_slots_filled++;

    // update first_free_slot to reflect the next available slot
	// We may be inserting into a previously empty slot, so 
	// we do a quick little search. 
	while (strTable[first_free_slot].inUse) {
		first_free_slot++;
	}
	// We may have walked past the end of the table, so we update
	// the highest_slot_used to reflect this properly.
	if (first_free_slot >= highest_used_slot) {
		// We know the previous slot is in use, because of the
		// above while loop. 
		highest_used_slot = first_free_slot - 1;
	}

    // insert into hash table
	yourStr = strTable[index].string;
    if (stringSpace->insert (yourStr, index) == 0) return index;

    // some problem
	return -1;
}


int StringSpace::
getCanonical (char* &str, SSString &canonical, 
			  StringSpaceAdoptionMethod adopt)
{
	canonical.context = ((canonical.index=getCanonical(str,adopt)) != -1) ?
							this : 0;
	return canonical.index;
}


int StringSpace::
getCanonical (char* &str, SSString *&canonical, 
			  StringSpaceAdoptionMethod adopt)
{
	if (!(canonical = new SSString)) return -1;
	return getCanonical (str, *canonical, adopt);
}


void StringSpace::
disposeByIndex(int index)
{
	// This works in a slightly non-intuitive way. We simply 
	// construct an SSString, then destroy it. The destructor 
	// calls SSString:dispose, and that does what we want. 
	SSString *str;

	str = new SSString;
	str->context = this;
	str->index = index;
	delete str;
	return;
}


SSString::
SSString ()
{
	context = NULL;
	index = 0;
	return;
}


SSString::
SSString (const SSString &sstr)
{
	context = NULL;
	copy (sstr);
	return;
}


SSString::
~SSString()
{
	dispose();
	return;
}


void
SSString::copy (const SSString &sstr)
{
		// clean up previous reference
	dispose( );

    index = sstr.index;
    context = sstr.context;

    // update reference count
    if (context) {
		context->strTable[index].refCount++;
	}
	return;
}

const SSString& 
SSString::operator=( const SSString &str )
{
	copy( str );
	return( *this );
}

bool operator== (const SSString &s1, const SSString &s2)
{
	bool are_equal;

	if (   s1.context == s2.context
	    && s1.index   == s2.index) {
		are_equal = true;
	} else {
		are_equal = false;
	}
	return are_equal;
}


bool operator!= (const SSString &s1, const SSString &s2)
{
	bool are_not_equal;

	if (   s1.context != s2.context
	    || s1.index   != s2.index) {
		are_not_equal = true;
	} else {
		are_not_equal = false;
	}
	return are_not_equal;
}

void 
SSString::dispose ()
{
    // if it is a valid reference, decrement refcount and check it is zero
    if (context && (--context->strTable[index].refCount) == 0)
    {
		YourSensitiveString str( context->strTable[index].string );
		context->stringSpace->remove( str );

        switch (context->strTable[index].adoptMode)
        {
            case SS_DUP: 
            case SS_ADOPT_C_STRING:
                free (context->strTable[index].string);
				context->strTable[index].string = NULL;
				context->strTable[index].inUse = false;
				context->strTable[index].adoptMode = SS_INVALID;
                break;

            case SS_ADOPT_CPLUS_STRING:
                delete [] (context->strTable[index].string);
				context->strTable[index].string = NULL;
				context->strTable[index].inUse = false;
				context->strTable[index].adoptMode = SS_INVALID;
                break;

            default:
                break;
        }

		// Adjust the variables that track our strTable usage. 
		context->number_of_slots_filled--;
		if (context->number_of_slots_filled < 0) {
			EXCEPT("StringSpace is algorithmically bad: number_of_slots_filled = %d!\n",
				   context->number_of_slots_filled);
		}
		if (context->first_free_slot >= index) {
			context->first_free_slot = index;
		}
		// If we deleted the highest used slot, we look backwards
		// for the next used slot. 
		if (context->highest_used_slot == index) {
			do {
				context->highest_used_slot--;
				if (      context->highest_used_slot >= 0 // not yet -1
						  && context->strTable[context->highest_used_slot].inUse) {
					break;
				} 
			} while (context->highest_used_slot >= -1);
		}
	}

	context = NULL;
	return;
}   
        
void StringSpace::
dump (void)
{
	int number_printed;

	number_printed = 0;
	printf ("String space dump:  %d strings\n", number_of_slots_filled);
	for (int i = 0; i <= highest_used_slot; i++)
	{
		if (strTable[i].inUse) {
			number_printed++;
			printf("#%03d ", i);
			if (strTable[i].string == NULL) {
				printf ("(disposed) (%d)\n", strTable[i].refCount);
			} else {
				printf ("%s (%d)\n", strTable[i].string, strTable[i].refCount);
			}
		}
	}
	if (number_printed != number_of_slots_filled) {
		printf("Number of slots expected (%d) is not accurate--should be %d.\n",
			   number_of_slots_filled, number_printed);
	}
	printf ("\nDone\n");
	return;
}

int 
StringSpace::checkFor (char *str)		// check if string is in the space
{
	int canonical_index;
	YourSensitiveString yourStr(str);
	if (stringSpace->lookup (yourStr, canonical_index) != 0) {
		canonical_index = -1;
	}
	return canonical_index;
}
