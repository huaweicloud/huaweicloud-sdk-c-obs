/*
 * Copyright (c) 2002-2005, Network Appliance, Inc. All rights reserved.
 *
 * This Software is licensed under one of the following licenses:
 * 
 * 1) under the terms of the "Common Public License 1.0" a copy of which is
 *    in the file LICENSE.txt in the root directory. The license is also
 *    available from the Open Source Initiative, see 
 *    http://www.opensource.org/licenses/cpl.php.
 * 
 * 2) under the terms of the "The BSD License" a copy of which is in the file
 *    LICENSE2.txt in the root directory. The license is also available from
 *    the Open Source Initiative, see
 *    http://www.opensource.org/licenses/bsd-license.php.
 * 
 * 3) under the terms of the "GNU General Public License (GPL) Version 2" a
 *    copy of which is in the file LICENSE3.txt in the root directory. The
 *    license is also available from the Open Source Initiative, see
 *    http://www.opensource.org/licenses/gpl-license.php.
 * 
 * Licensee has the right to choose one of the above licenses.
 * 
 * Redistributions of source code must retain the above copyright
 * notice and one of the license notices.
 * 
 * Redistributions in binary form must reproduce both the above copyright
 * notice, one of the license notices in the documentation
 * and/or other materials provided with the distribution.
 */

/****************************************************************
 *
 * HEADER: dat_registry.h
 *
 * PURPOSE: DAT registration API signatures
 *
 * Description: Header file for "DAPL: Direct Access Programming
 *		Library, Version: 1.2"
 *
 * 		Contains registration external reference signatures
 * 		for dat registry functions. This file is *only*
 * 		included by providers, not consumers.
 *
 * Mapping rules:
 * 	All global symbols are prepended with DAT_ or dat_
 * 	All DAT objects have an 'api' tag which, such as 'ep' or 'lmr'
 * 	The method table is in the provider definition structure.
 *
 **********************************************************/
#ifndef _DAT_REGISTRY_H_
#define _DAT_REGISTRY_H_

#if defined(_UDAT_H_)
#include <dat/udat_redirection.h>
#elif defined(_KDAT_H_)
#include <dat/kdat_redirection.h>
#else
#error Must include udat.h or kdat.h
#endif

/*
 * dat registration API.
 *
 * Technically the dat_ia_open is part of the registration API. This
 * is so the registration module can map the device name to a provider
 * structure and then call the provider dat_ia_open function.
 * dat_is_close is also part of the registration API so that the
 * registration code can be aware when an ia is no longer in use.
 *
 */

extern DAT_RETURN dat_registry_add_provider (
	IN  const DAT_PROVIDER *,               /* provider          */
	IN  const DAT_PROVIDER_INFO* );         /* provider info     */

extern DAT_RETURN dat_registry_remove_provider (
	IN  const DAT_PROVIDER *,               /* provider          */
	IN  const DAT_PROVIDER_INFO* );         /* provider info     */

/*
 * Provider initialization APIs.
 *
 * Providers that support being automatically loaded by the Registry must
 * implement these APIs and export them as public symbols.
 */

#define DAT_PROVIDER_INIT_FUNC_NAME  dat_provider_init
#define DAT_PROVIDER_FINI_FUNC_NAME  dat_provider_fini

#define DAT_PROVIDER_INIT_FUNC_STR   "dat_provider_init"
#define DAT_PROVIDER_FINI_FUNC_STR   "dat_provider_fini"

typedef void ( *DAT_PROVIDER_INIT_FUNC) (
	IN const DAT_PROVIDER_INFO *,           /* provider info     */
	IN const char *);                       /* instance data     */

typedef void ( *DAT_PROVIDER_FINI_FUNC) (
	IN const DAT_PROVIDER_INFO *);          /* provider info     */

#endif /* _DAT_REGISTRY_H_ */
