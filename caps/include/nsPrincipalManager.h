/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
/*creates, registers, and performs logical operations on principals*/
#ifndef _NS_PRINCIPAL_MANAGER_H_
#define _NS_PRINCIPAL_MANAGER_H_

#include "nsIPrincipalManager.h"
#include "nsHashtable.h"

#define NS_PRINCIPALMANAGER_CID \
{ 0x7ee2a4c0, 0x4b91, 0x11d3, \
{ 0xba, 0x18, 0x00, 0x60, 0xb0, 0xf1, 0x99, 0xa2 }}

class nsPrincipalManager : public nsIPrincipalManager {

public:

  NS_DEFINE_STATIC_CID_ACCESSOR(NS_PRINCIPALMANAGER_CID)

	NS_DECL_ISUPPORTS
    NS_DECL_NSIPRINCIPALMANAGER

	static nsresult
	GetPrincipalManager(nsPrincipalManager * * prinMan);

	virtual ~nsPrincipalManager(void);

	NS_IMETHOD
	CreateCertificatePrincipal(const unsigned char * * certChain, PRUint32 * certChainLengths, PRUint32 noOfCerts, nsIPrincipal * * prin);

	void 
	RegisterSystemPrincipal(nsIPrincipal * principal);

	static nsIPrincipalArray *
	GetMyPrincipals(PRInt32 callerDepth);

	static nsIPrincipalArray *
	GetMyPrincipals(nsIScriptContext * context, PRInt32 callerDepth);

	nsIPrincipal * 
	GetPrincipalFromString(char * prinName);
	
	static nsIPrincipal * 
	GetSystemPrincipal(void);

	static PRBool 
	HasSystemPrincipal(nsIPrincipalArray * prinArray);

	static nsIPrincipal * 
	GetUnsignedPrincipal(void);

	static nsIPrincipal * 
	GetUnknownPrincipal(void);

	const char * 
	GetAllPrincipalsString(void);

	void 
	AddToPrincipalNameToPrincipalTable(nsIPrincipal * prin);

	void
	RemoveFromPrincipalNameToPrincipalTable(nsIPrincipal * prin);

	nsIPrincipalArray * 
	GetClassPrincipalsFromStack(PRInt32 callerDepth);

	nsIPrincipalArray * 
	GetClassPrincipalsFromStack(nsIScriptContext * context, PRInt32 callerDepth);


private:
  nsPrincipalManager(void);
  NS_IMETHODIMP
  Init();
	nsHashtable * itsPrinNameToPrincipalTable;
};

#endif /* _NS_PRINCIPAL_MANAGER_H_*/
