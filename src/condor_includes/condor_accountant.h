/***************************Copyright-DO-NOT-REMOVE-THIS-LINE**
 * CONDOR Copyright Notice
 *
 * See LICENSE.TXT for additional notices and disclaimers.
 *
 * Copyright (c)1990-1998 CONDOR Team, Computer Sciences Department, 
 * University of Wisconsin-Madison, Madison, WI.  All Rights Reserved.  
 * No use of the CONDOR Software Program Source Code is authorized 
 * without the express consent of the CONDOR Team.  For more information 
 * contact: CONDOR Team, Attention: Professor Miron Livny, 
 * 7367 Computer Sciences, 1210 W. Dayton St., Madison, WI 53706-1685, 
 * (608) 262-0856 or miron@cs.wisc.edu.
 *
 * U.S. Government Rights Restrictions: Use, duplication, or disclosure 
 * by the U.S. Government is subject to restrictions as set forth in 
 * subparagraph (c)(1)(ii) of The Rights in Technical Data and Computer 
 * Software clause at DFARS 252.227-7013 or subparagraphs (c)(1) and 
 * (2) of Commercial Computer Software-Restricted Rights at 48 CFR 
 * 52.227-19, as applicable, CONDOR Team, Attention: Professor Miron 
 * Livny, 7367 Computer Sciences, 1210 W. Dayton St., Madison, 
 * WI 53706-1685, (608) 262-0856 or miron@cs.wisc.edu.
****************************Copyright-DO-NOT-REMOVE-THIS-LINE**/
#ifndef _Condor_Accountant_H_
#define _Condor_Accountant_H_

#include <iostream.h>

#include "condor_classad.h"

#include "HashTable.h"
#include "MyString.h"
#include "Set.h"
#include "TimeClass.h"

// this is the required minimum separation between two priorities for them
// to be considered distinct values
static const double PriorityDelta = 0.5;

class Accountant {

public:

  //--------------------------------------------------------
  // User Functions
  //--------------------------------------------------------

  void Initialize();  // Initialize some parameters

  int GetResourcesUsed(const MyString& CustomerName); // get # of used resources
  double GetPriority(const MyString& CustomerName); // get priority for a customer
  void SetPriority(const MyString& CustomerName, double Priority); // set priority for a customer

  int MatchExist(const MyString& CustomerName, const MyString& ResourceName); // check if a mtch exists
  void AddMatch(const MyString& CustomerName, ClassAd* ResourceAd); // Add new match
  void RemoveMatch(const MyString& ResourceName); // remove a match

  void UpdatePriorities(); // update all the priorities

  void CheckMatches(ClassAdList& ResourceList);  // Remove matches that are not claimed
  AttrList* ReportState();

  //--------------------------------------------------------
  // Misc public functions
  //--------------------------------------------------------

  static int HashFunc(const MyString& Key, int TableSize) {
    int count=0;
    int length=Key.Length();
    for(int i=0; i<length; i++) count+=Key[i];
    return (count % TableSize);
  }
  
  Accountant(int MaxCustomers=1024, int MaxResources=1024);
                                                
private:

  //--------------------------------------------------------
  // Private methods Methods
  //--------------------------------------------------------
  
  void AddMatch(const MyString& CustomerName, const MyString& ResourceName, const Time& T); 
  void RemoveMatch(const MyString& ResourceName, const Time& T);
  void UpdatePriorities(const Time& T);

  void LoadState(); // Save to file
  void SaveState(); // Read from file

  //--------------------------------------------------------
  // Configuration variables
  //--------------------------------------------------------

  double MinPriority;        // Minimum priority (if no resources used)
  double NiceUserPriorityFactor;
  double HalfLifePeriod;     // The time in sec in which the priority is halved by aging
  MyString LogFileName;      // Name of Log file

  //--------------------------------------------------------
  // Internal data types
  //--------------------------------------------------------

  struct CustomerRecord {
    double Priority;
    double UnchargedTime;
    Set<MyString> ResourceNames;
    CustomerRecord() { Priority=UnchargedTime=0; }
  };

  struct ResourceRecord {
    MyString CustomerName;
    ClassAd* Ad;
    Time StartTime;
    ResourceRecord() { Ad=NULL; }
    ~ResourceRecord() { if (Ad) delete Ad; }
  };

  //--------------------------------------------------------
  // Data members
  //--------------------------------------------------------

  HashTable<MyString, CustomerRecord*> Customers;
  HashTable<MyString, ResourceRecord*> Resources;

  Time LastUpdateTime;
  int LogEnabled;

  //--------------------------------------------------------
  // Utility functions
  //--------------------------------------------------------

  static MyString GetResourceName(ClassAd* Resource);
  static int IsClaimed(ClassAd* ResourceAd, MyString& CustomerName);
  static int CheckClaimedOrMatched(ClassAd* ResourceAd, const MyString& CustomerName);
  static ClassAd* FindResourceAd(const MyString& ResourceName, ClassAdList& ResourceList);
  void AppendLogEntry(const MyString& Action, const MyString& CustomerName, const MyString& ResourceName, double d);

};

#endif
