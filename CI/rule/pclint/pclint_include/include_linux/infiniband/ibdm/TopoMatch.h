/*
 * Copyright (c) 2004-2010 Mellanox Technologies LTD. All rights reserved.
 *
 * This software is available to you under the terms of the
 * OpenIB.org BSD license included below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */


#include "Fabric.h"


// Return 0 if fabrics match 1 otherwise.
// fill in the messages char array with diagnostics..
int
TopoMatchFabrics(
        IBFabric   *p_sFabric,          // The specification fabric
        IBFabric   *p_dFabric,          // The discovered fabric
        const char *anchorNodeName,     // The name of the node to be the anchor point - in the spec fabric
        int         anchorPortNum,      // The port number of the anchor port - in the spec fabric
        uint64_t    anchorPortGuid,     // Guid of the anchor port - in the discovered fabric
        char **messages,
		int compareNodesNames);			// Check that node's name match


// Build a merged fabric from a matched discovered and spec fabrics:
// * Every node from the discovered fabric must appear
// * We sued matched nodes and system names.
int
TopoMergeDiscAndSpecFabrics(
        IBFabric  *p_sFabric,
        IBFabric  *p_dFabric,
        IBFabric  *p_mFabric);


// Return 0 if fabrics match 1 otherwise.
// The detailed compare messages are returned in messages
int
TopoMatchFabricsFromEdge(
        IBFabric *p_sFabric,            // The specification fabric
        IBFabric *p_dFabric,            // The discovered fabric
        char **messages);

