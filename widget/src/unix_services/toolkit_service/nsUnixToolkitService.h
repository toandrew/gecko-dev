/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsIUnixToolkitService.h"

class nsUnixToolkitService : public nsIUnixToolkitService
{
 public:
  nsUnixToolkitService();
  virtual ~nsUnixToolkitService();

  NS_DECL_ISUPPORTS

  NS_IMETHOD SetToolkitName(const nsCString & aToolkitName);
  NS_IMETHOD SetWidgetToolkitName(const nsCString & aToolkitName);
  NS_IMETHOD SetGfxToolkitName(const nsCString & aToolkitName);

  NS_IMETHOD IsValidToolkit(const nsCString & aToolkitName,
                            PRBool * aResultOut);
  NS_IMETHOD IsValidWidgetToolkit(const nsCString & aToolkitName,
                                  PRBool * aResultOut);
  NS_IMETHOD IsValidGfxToolkit(const nsCString & aToolkitName,
                               PRBool * aResultOut);

  NS_IMETHOD GetToolkitName(nsCString & aToolkitNameOut);
  NS_IMETHOD GetWidgetToolkitName(nsCString & aToolkitNameOut);
  NS_IMETHOD GetGfxToolkitName(nsCString & aToolkitNameOut);

  NS_IMETHOD GetWidgetDllName(nsCString & aWidgetDllNameOut);
  NS_IMETHOD GetGfxDllName(nsCString & aGfxDllNameOut);

  NS_IMETHOD GetTimerCID(nsCID ** aTimerCIDOut);

private:

  static nsresult GlobalGetWidgetToolkitName(nsCString & aStringOut);
  static nsresult GlobalGetGfxToolkitName(nsCString & aStringOut);

  static nsresult EnsureWidgetToolkitName();
  static nsresult EnsureGfxToolkitName();

  static nsresult Cleanup();

  static nsCString *     sWidgetToolkitName;
  static nsCString *     sGfxToolkitName;
  static nsCString *     sWidgetDllName;
  static nsCString *     sGfxDllName;
  static const nsCID *  sTimerCID;

  static const char * ksDefaultToolkit;
  static const char * ksDllSuffix;
  static const char * ksDllPrefix;
  static const char * ksWidgetName;
  static const char * ksGfxName;
};
