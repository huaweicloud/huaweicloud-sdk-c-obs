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


/*
 * Fabric Utilities Project
 *
 * Hierarchical Systems Definition DataModel
 *
 */

#ifndef IBDM_SYSDEF_H
#define IBDM_SYSDEF_H


#include "Fabric.h"

typedef map< string, class IBSysInstPort *, strless > map_str_pinstport;
typedef map< string, class IBSysInst *, strless > map_str_psysinsts;
typedef map< string, class IBSysPortDef *, strless > map_str_psysportdef;
typedef map< string, class IBSysDef *, strless > map_str_psysdef;
typedef list< string > list_str;

// Ports are only connected to other SysInst at the same level.
// We track upper level connections only in the IBPortDef object.
class IBSysInstPort {
  string               name;         // name of this port
  string               remInstName;
  string               remPortName;
  IBLinkWidth          width;
  IBLinkSpeed          speed;

 public:
  IBSysInstPort(string n, const char *toNode, const char *toPort, IBLinkWidth w, IBLinkSpeed s) {
    name = n; remInstName = toNode; remPortName = toPort; width = w; speed = s;
  };

  friend class IBSysInst;
  friend class IBSystemsCollection;

};

class IBSysInst {
  string      name;         // name of this inst
  map_str_str SubInstMods;  // Modifier to apply to each sub inst by hier name
  map_str_pinstport InstPorts;  // Any inst to inst connections
  string      master;       // the master system name (without any modifier)
  int         nodeNumPorts; // only valid for node types.
  IBNodeType  nodeType;     // node tyep SW/CA
  int         isNode;       // if non zero makes this instance a node instance.

 public:
  // we use two constructors - one for Node and one for SubSys
  IBSysInst(string n, string m, int np, IBNodeType t) {
    name = n; isNode = 1; master = m; nodeNumPorts = np; nodeType = t;
  };

  IBSysInst(string n, string m) {
    name = n; isNode = 0; master = m; nodeNumPorts = 0;
    nodeType = IB_UNKNOWN_NODE_TYPE;
  };

  inline int addPort(IBSysInstPort* p_port) {
    InstPorts[p_port->name] = p_port;
    return(0);
  };

  inline int addInstMod(char *subInstName, char *instMod) {
    SubInstMods[subInstName] = instMod;
    return(0);
  };

  inline string getName() { return name; };

  friend class IBSysDef;
  friend class IBSystemsCollection;
};

class IBSysPortDef {
  string        name;         // name of this port
  string        instName;     // instance connected to the port
  string        instPortName; // inst port name connected to this sysport.
  IBLinkWidth   width;
  IBLinkSpeed   speed;
 public:
  IBSysPortDef(string n, string i, string p, IBLinkWidth w, IBLinkSpeed s) {
    name = n; instName = i; instPortName = p; width = w; speed = s;
  };

  friend class IBSysDef;
  friend class IBSystemsCollection;
};

class IBSysDef {
  string               fileName;          // The file the system was defined by
  map_str_psysinsts    SystemsInstByName; // all sub instances by name
  map_str_psysportdef  SysPortsDefs;      // all sys ports defs

  map_str_str     SubInstAtts;  // Map of node attributes by hier node name
                                // We attach these attributes only after smash

 public:

  IBSysDef(string fn) {fileName = fn; };

  // add sub instance attribute
  inline void setSubInstAttr(string hierInstName, string attrStr) {
    map_str_str::iterator iaI = SubInstAtts.find(hierInstName);
    if (iaI == SubInstAtts.end()) {
      SubInstAtts[hierInstName] = attrStr;
    } else {
      (*iaI).second += "," + attrStr;
    }
  };

  inline int addInst (IBSysInst *p_inst) {
    SystemsInstByName[p_inst->name] = p_inst;
    return 0;
  };

  inline int addSysPort(IBSysPortDef *p_sysPort) {
    SysPortsDefs[p_sysPort->name] = p_sysPort;
    return 0;
  };

  string getName() {
    return fileName;
  };

  friend class IBSystemsCollection;
};

// we collect all top level systems into this
class IBSystemsCollection {
  map_str_psysdef SysDefByName; // Map of sub instances by name
 public:

  // Initialize the system collection with all available
  // ibnl files in the given directory path list.
  int parseSysDefsFromDirs(list< string > dirs);

  // Parse a system definition netlist
  int parseIBSysdef(string fileName);

  // Add a new system def ..
  inline int addSysDef(string sname, IBSysDef *p_sysDef) {
    // TODO - check for doubles...
    SysDefByName[sname] = p_sysDef;
    return 0;
  };

  // given a system type and optionaly a modifier
  // look it up
  inline IBSysDef* getSysDef(string sname) {
    // The modifier is simply added to the system name
    map_str_psysdef::iterator sI = SysDefByName.find(sname);
    if (sI != SysDefByName.end()) {
      return (*sI).second;
    }
    return NULL;
  };

  // Given the name and type of the system - build its system by applying any
  // modifiers by hierarchical instance name.
  IBSystem *makeSystem(
    IBFabric *p_fabric, string name, string master, map_str_str mods);

 private:
  // build all nodes recursively
  int
    makeSysNodes(
      IBFabric *p_fabric,        // the fabric we belong to
      IBSystem *p_system,        // the system we build
      IBSysDef *p_parSysDef,     // the sysdef of the parent
      string    parHierName,     // the hier name of the parent "" for top
      map_str_str &mods          // hier name based modifiers list
      );

  // Get a system definition for an instance applying any modifiers
  IBSysDef *
    getInstSysDef(
      IBSysDef    *p_sysDef,    // parent system def
      IBSysInst   *p_inst,      // the instance
      string       hierInstName,// instance hierarchical name
      map_str_str &mods         // hier name based modifiers list
      );

  // find a system port def gven the instance, port name, hierName and
  IBSysPortDef *
    getSysPortDefByInstPortName(
      IBSysDef     *p_sysDef,     // the system definition the port is on
      IBSysInst    *p_inst,       // the instance
      string        instPortName, // the port name
      string        hierInstName, // the hier name of the instance
      map_str_str  &mods          // hier name based modifiers list
      );

  IBPort *
    makeNodePortByInstAndPortName(
      IBSystem     *p_system,    // the system we build the node port in
      IBSysDef     *p_sysDef,     // the system definition holding the inst
      IBSysInst    *p_inst,       // the instance
      string        instPortName, // the port name
      string        hierInstName, // the hier name of the instance
      map_str_str  &mods          // hier name based modifiers list
      );

  // find the lowest point connection of this port and make it if a node port
  // assumes the nodes were already created for the
  IBPort *
    makeNodePortBySysPortDef(
      IBSystem      *p_system,    // the system we build the node port in
      IBSysDef      *p_sysDef,    // the system definition the port is on
      IBSysPortDef  *p_sysPortDef,// system port definition
      string         parHierName, // the hier name of the parent "" for top
      map_str_str   &mods         // hier name based modifiers list
      );

  // find the lowest point connection of this sub sysport and make a node port
  // assumes the nodes were already created for the
  IBPort *
    makeNodePortBySubSysInstPortName(
      IBSystem      *p_system,    // the system we build the node port in
      IBSysDef      *p_sysDef,    // the system definition the inst is in
      string         instName,    // Name of the instance
      string         instPortName,// Name of instance port
      string         parHierName, // the hier name of the parent "" for top
      map_str_str   &mods         // hier name based modifiers list
      );

  //  DFS from top on each level connect all connected SysInst ports
  int
    makeSubSystemToSubSystemConns(
      IBSystem      *p_system,    // the system we build the node port in
      IBSysDef      *p_sysDef,    // the system definition the port is on
      string         parHierName, // the hier name of the parent "" for top
      map_str_str   &mods         // hier name based modifiers list
      );

  // dump out names of all defined systems
  void dump();
};

// we use a singleton system repository
IBSystemsCollection *theSysDefsCollection();

#endif // IBDM_SYSDEF_H
