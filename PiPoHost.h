/**
 
@file PiPoHost.h
@author Norbert.Schnell@ircam.fr
 
@brief Plugin Interface for Processing Objects host class
 
A PiPo host is built around the class PiPoChain that represents a sequence of PiPo modules piping data into each other. 
See details of the steps there.

Copyright (C) 2012-2014 by IRCAM – Centre Pompidou, Paris, France.
All rights reserved.

*/

#ifndef _PIPO_HOST_
#define _PIPO_HOST_

#include "PiPoSequence.h"
#include <string>
#include <vector>


/** abstract base class for a container of a pipo module for PiPoModuleFactory
 */
class PiPoModule
{ 
public:
  PiPoModule(void) { };
  ~PiPoModule(void) { };
};

class PiPoModuleFactory
{ 
public:
  PiPoModuleFactory(void) { };
  ~PiPoModuleFactory(void) { };
  
  virtual PiPo *create(unsigned int index, const std::string &pipoName, const std::string &instanceName, PiPoModule *&module) = 0;
};

/** element of pipo chain, points to pipo pipo
 */
class PiPoOp
{ 
  unsigned int index;
  std::string pipoName; /// pipo class name
  std::string instanceName;
  PiPo *pipo;
  PiPoModule *module;
  
public:
  PiPoOp(unsigned int index = 0) : pipoName(), instanceName()
  {
    this->index = index;
    this->pipo = NULL;
    this->module = NULL;
  };
  
//  PiPoOp(const PiPoOp &other) : pipoName(), instanceName()
//  { 
//    this->index = other.index;
//    this->pipoName = other.pipoName;
//    this->instanceName = other.instanceName;
//    this->pipo = other.pipo;
//    this->module = other.module;
//  };
  
  ~PiPoOp(void) { };

  void setParent(PiPo::Parent *parent)
  {
    if(this->pipo != NULL)
      this->pipo->setParent(parent);
  };
  
  void clear(void)
  {
    if(this->module != NULL)
      delete this->module;
    
    this->module = NULL;
    
    if(this->pipo != NULL)
      delete pipo;
    
    this->pipo = NULL;
  };
  
  /** parse one pipo name, and optional instance name in '(' ')'
   */
  void parse(std::string str, size_t &pos)
  {
    this->clear();
    
    size_t end = str.find_first_of(':', pos);
    size_t open = str.find_first_of('(', pos);
    size_t closed = str.find_first_of(')', pos);
    
    if(open < std::string::npos && closed < std::string::npos)
    {
      this->pipoName = str.substr(pos, open - pos);
      this->instanceName = str.substr(open + 1, closed - open - 1);
    }
    else
    {
      this->pipoName = str.substr(pos, end - pos);
      this->instanceName = this->pipoName;
    }
    
    if(end < std::string::npos)
      pos = end + 1;
    else
      pos = std::string::npos;
  };
  
  bool instantiate(PiPo::Parent *parent, PiPoModuleFactory *moduleFactory)
  {
    this->pipo = NULL;
    this->module = NULL;
    
    if(moduleFactory != NULL)
      this->pipo = moduleFactory->create(index, this->pipoName, this->instanceName, this->module);
    
    if(this->pipo != NULL)
    {
      this->pipo->setParent(parent);
      return true;
    }
    
    return false;
  };

  void set(unsigned int index, PiPo::Parent *parent, PiPoModuleFactory *moduleFactory, const PiPoOp &other)
  {
    this->index = index;
    this->pipoName = other.pipoName;
    this->instanceName = other.instanceName;
    
    this->instantiate(parent, moduleFactory);
    
    if(this->pipo != NULL)
      this->pipo->cloneAttrs(other.pipo);
  };
  
  PiPo *getPiPo() { return this->pipo; };
  const char *getInstanceName() { return this->instanceName.c_str(); };
  bool isInstanceName(const char *str) { return (this->instanceName.compare(str) == 0); };
};

/** 

A PiPo host is built around the class PiPoChain that represents a sequence of PiPo modules piping data into each other. 

The PiPoChain is setup with the following steps:
 
1. parse chain definition string by calling parse()
2. instantiate each PiPoModule in the chain by calling instantiate(), using a PiPoModuleFactory
3. connect PiPos by calling PiPoSequence::connect()
 
A PiPoChain is itself also a PiPo module, i.e. data processing works the same as for a simple module:
- the PiPoChain is prepared for processing by calling streamAttributes() on it.
- data is piped into the PiPoChain with the frames() method.

The host registers as the receiver for the last PiPo module in the PiPoChain.  
*/

class PiPoChain : public PiPoSequence  //TODO: shouldn't PiPoChain just contain a member seq_ for the sequence to build, instead of inheriting?
{
  std::vector<PiPoOp> ops;
  //PiPo::Parent *parent;	//FIXME: remove? pipo has a parent member already!
  PiPoModuleFactory *moduleFactory;

public:
  PiPoChain(PiPo::Parent *parent, PiPoModuleFactory *moduleFactory = NULL)
  : PiPoSequence(parent), ops()
  {
    this->parent = parent;
    this->moduleFactory = moduleFactory;
  };  
  
  PiPoChain(const PiPoChain &other)
  : PiPoSequence(other), ops()
  { 
    this->moduleFactory = NULL;
    this->connect(NULL);
  };  
  
  const PiPoChain& operator=(const PiPoChain &other)
  {
    PiPoSequence::operator= (other);	// call assignment operator of base class PiPoSequence

    this->parent = other.parent;
    this->moduleFactory = other.moduleFactory;
    
    unsigned int numOps = other.ops.size();
    this->ops.clear();
    this->ops.resize(numOps);
    
    for(unsigned int i = 0; i < numOps; i++)
      this->ops[i].set(i, this->parent, this->moduleFactory, other.ops[i]);
    
    this->connect(NULL);
    this->moduleFactory = NULL;

    return *this;
  };
  
  ~PiPoChain(void) 
  { 
    this->clear();
  };
  

  /** @name PiPoChain setup methods */
  /** @{ */
  
  void clear()
  {
    PiPoSequence::clear();
    for(unsigned int i = 0; i < this->ops.size(); i++)
      this->ops[i].clear();

    this->ops.clear();
  };
  

  /** parse pipo chain specification from string 
      
      creates list of PiPoOp in member ops
   */
  size_t parse(const char *str)
  {
    std::string pipoChainStr = str;
    size_t pos = 0;
    
    this->clear();
    
    for(unsigned int i = 0; pos < std::string::npos; i++)
    {
      this->ops.resize(i + 1, i); 
      this->ops[i].parse(pipoChainStr, pos);
    }
    
    return this->ops.size();
  };
  
  /** go through list of PiPoOp in ops and instantiate PiPoModule using PiPoModuleFactory
   */
  bool instantiate(void)
  {
    if(this->ops.size() > 0)
    {
      for(unsigned int i = 0; i < this->ops.size(); i++)
      {
        if(this->ops[i].instantiate(this->parent, this->moduleFactory))
	{ // build sequence by appending modules
	  this->add(this->ops[i].getPiPo());
	}
	else
        {
          this->clear();
          return false; 
        }
      }
      
      return true;
    }
    
    return false;
  };  

  /** @} PiPoChain setup methods */

  /** @name PiPoChain query methods */
  /** @{ */

  int getSize()
  {
    return this->ops.size();
  };
  
  int getIndex(const char *instanceName)
  {
    for(unsigned int i = 0; i < this->ops.size(); i++)
    {
      if(this->ops[i].isInstanceName(instanceName))
        return i;
    }
    
    return -1;
  };  

  using PiPoSequence::getPiPo;
  
  PiPo *getPiPo(const char *instanceName)
  {
    int index = getIndex(instanceName);
    
    if(index >= 0)
      return getPiPo(index);
    
    return NULL;
  };  
  
  const char *getInstanceName(unsigned int index) 
  { 
    if(index < this->ops.size())
      return this->ops[index].getInstanceName(); 
    
    return NULL;
  };

  /** @} PiPoChain query methods */
    
  /** @name overloaded PiPo methods */
  /** @{ */
  /** @name all preparation and processing methods are inherited from PiPoSequence:
      streamAttributes(), reset(), frames(), finalize() */
  /** @} end of overloaded PiPo methods */
};

/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */

#endif
