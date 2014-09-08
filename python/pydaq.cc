//
//  pydaq module
//
//  Defines one python class: pdsdaq
//

//#define DBUG

#include <Python.h>
#include <structmember.h>

#include "pdsapp/python/pydaq.hh"
#include "pdsapp/control/RemoteSeqCmd.hh"
#include "pds/config/ControlConfigType.hh"
#include "pdsdata/psddl/control.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
using Pds::ControlData::PVControl;
using Pds::ControlData::PVMonitor;
using Pds::ControlData::PVLabel;
#include "pdsapp/control/RemotePartition.hh"
using Pds::RemotePartition;
using Pds::RemoteNode;

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sstream>
using std::ostringstream;

#include <list>
using std::list;

static const int MaxPathSize   = 0x100;
static const int MaxAliasSize   = 0x100;
static const int MaxConfigSize = 0x100000;
static const int Control_Port  = 10130;

enum PydaqState { Disconnected, Connected, Configured, Running };

//
//  pdsdaq class methods
//
static void      pdsdaq_dealloc(pdsdaq* self);
static PyObject* pdsdaq_new    (PyTypeObject* type, PyObject* args, PyObject* kwds);
static int       pdsdaq_init   (pdsdaq* self, PyObject* args, PyObject* kwds);
static PyObject* pdsdaq_dbpath   (PyObject* self);
static PyObject* pdsdaq_dbalias  (PyObject* self);
static PyObject* pdsdaq_dbkey    (PyObject* self);
static PyObject* pdsdaq_partition(PyObject* self);
static PyObject* pdsdaq_runnum   (PyObject* self);
static PyObject* pdsdaq_expt     (PyObject* self);
static PyObject* pdsdaq_detectors(PyObject* self);
static PyObject* pdsdaq_devices  (PyObject* self);
static PyObject* pdsdaq_disconnect(PyObject* self);
static PyObject* pdsdaq_types    (PyObject* self);
static PyObject* pdsdaq_connect  (PyObject* self);
static PyObject* pdsdaq_configure(PyObject* self, PyObject* args, PyObject* kwds);
static PyObject* pdsdaq_begin    (PyObject* self, PyObject* args, PyObject* kwds);
static PyObject* pdsdaq_end      (PyObject* self);
static PyObject* pdsdaq_stop     (PyObject* self);
static PyObject* pdsdaq_eventnum (PyObject* self);
static PyObject* pdsdaq_rcv      (PyObject* self);

static PyMethodDef pdsdaq_methods[] = {
  {"dbpath"    , (PyCFunction)pdsdaq_dbpath    , METH_NOARGS  , "Get database path"},
  {"dbalias"   , (PyCFunction)pdsdaq_dbalias   , METH_NOARGS  , "Get database alias"},
  {"dbkey"     , (PyCFunction)pdsdaq_dbkey     , METH_NOARGS  , "Get database key"},
  {"partition" , (PyCFunction)pdsdaq_partition , METH_NOARGS  , "Get partition"},
  {"runnumber" , (PyCFunction)pdsdaq_runnum    , METH_NOARGS  , "Get run number"},
  {"experiment", (PyCFunction)pdsdaq_expt      , METH_NOARGS  , "Get experiment number"},
  {"detectors" , (PyCFunction)pdsdaq_detectors , METH_NOARGS  , "Get the detector names"},
  {"devices"   , (PyCFunction)pdsdaq_devices   , METH_NOARGS  , "Get the device names"},
  {"types"     , (PyCFunction)pdsdaq_types     , METH_NOARGS  , "Get the type names"},
  {"configure" , (PyCFunction)pdsdaq_configure , METH_KEYWORDS, "Configure the scan"},
  {"begin"     , (PyCFunction)pdsdaq_begin     , METH_KEYWORDS, "Configure the cycle"},
  {"end"       , (PyCFunction)pdsdaq_end       , METH_NOARGS  , "Wait for the cycle end"},
  {"stop"      , (PyCFunction)pdsdaq_stop      , METH_NOARGS  , "End the current cycle"},
  {"eventnum"  , (PyCFunction)pdsdaq_eventnum  , METH_NOARGS  , "Get current event number"},
  {"connect"   , (PyCFunction)pdsdaq_connect   , METH_NOARGS  , "Connect to control"},
  {"disconnect", (PyCFunction)pdsdaq_disconnect, METH_NOARGS  , "Disconnect from control"},
  {NULL},
};

//
//  Register pdsdaq members
//
static PyMemberDef pdsdaq_members[] = {
  {NULL}
};

static PyTypeObject pdsdaq_type = {
  PyObject_HEAD_INIT(NULL)
    0,                          /* ob_size */
    "pydaq.Control",            /* tp_name */
    sizeof(pdsdaq),             /* tp_basicsize */
    0,                          /* tp_itemsize */
    (destructor)pdsdaq_dealloc, /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare*/
    0,                          /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "pydaq Control objects",    /* tp_doc */
    0,                    /* tp_traverse */
    0,                    /* tp_clear */
    0,                    /* tp_richcompare */
    0,                    /* tp_weaklistoffset */
    0,                    /* tp_iter */
    0,                    /* tp_iternext */
    pdsdaq_methods,             /* tp_methods */
    pdsdaq_members,             /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    (initproc)pdsdaq_init,      /* tp_init */
    0,                          /* tp_alloc */
    pdsdaq_new,                 /* tp_new */
};

//
//  pdsdaq class functions
//

void pdsdaq_dealloc(pdsdaq* self)
{
  if (self->socket >= 0) {
    ::close(self->socket);
  }

  if (self->buffer) {
    delete[] self->dbpath;
    delete[] self->dbalias;
    delete   self->partition;
    delete[] self->buffer;
  }

  self->ob_type->tp_free((PyObject*)self);
}

PyObject* pdsdaq_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  pdsdaq* self;

  self = (pdsdaq*)type->tp_alloc(type,0);
  if (self != NULL) {
    self->addr     = 0;
    self->platform = 0;
    self->socket   = -1;
    self->state    = Disconnected;
    self->dbpath   = new char[MaxPathSize];
    self->dbalias  = new char[MaxAliasSize];
    self->dbkey    = 0;
    self->partition= new RemotePartition;
    self->buffer   = new char[MaxConfigSize];
    self->runinfo  = 0;
  }

  return (PyObject*)self;
}

int pdsdaq_init(pdsdaq* self, PyObject* args, PyObject* kwds)
{
  char* kwlist[] = {"host","platform",NULL};
  unsigned    addr  = 0;
  const char* host  = 0;
  unsigned    platform = 0;

  while(1) {
    if (PyArg_ParseTupleAndKeywords(args,kwds,"s|I",kwlist,
                                    &host,&platform)) {

      hostent* entries = gethostbyname(host);
      if (entries) {
        addr = htonl(*(in_addr_t*)entries->h_addr_list[0]);
        break;
      }
    }
    if (PyArg_ParseTupleAndKeywords(args,kwds,"I|i",kwlist,
                                    &addr,&platform)) {
      break;
    }

    return -1;
  }

  PyErr_Clear();

  self->addr     = addr;
  self->platform = platform;
  self->socket   = -1;
  return 0;
}

PyObject* pdsdaq_disconnect(PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  if (daq->socket >= 0) {
    ::close(daq->socket);
    daq->socket = -1;
  }
  daq->state = Disconnected;

  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* pdsdaq_connect(PyObject* self)
{
  Py_DECREF(pdsdaq_disconnect(self));

  pdsdaq* daq = (pdsdaq*)self;

  int s = ::socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0)
    return NULL;

  sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(daq->addr);
  sa.sin_port        = htons(Control_Port+daq->platform);

  if (::connect(s, (sockaddr*)&sa, sizeof(sa)) < 0) {
    ::close(s);
    PyErr_SetString(PyExc_RuntimeError,"Connect failed");
    return NULL;
  }

  daq->socket = s;

  while(1) {

    uint32_t len=0;
    if (::recv(s, &len, sizeof(len), MSG_WAITALL) < 0)
    {
      printf("pdsdaq_connect(): get dbpath len failed.  Is DAQ allocated?\n");
      break;
    }

    /*
     * Get dbpath
     */
    if (len==0)
    {
      printf("pdsdaq_connect(): get dbpath len = 0\n");
      break;
    }

    char buff[256];
    if (::recv(s, buff, len, MSG_WAITALL) != len)
    {
      printf("pdsdaq_connect(): get dbpath string failed\n");
      break;
    }

    buff[len] = 0;
//    *strrchr(buff,'/') = 0;
    strcpy(daq->dbpath,buff);

#ifdef DBUG
    printf("pdsdaq_connect received dbpath %s\n",daq->dbpath);
#endif

    /*
     * Get dbkey
     */
    uint32_t key;
    if (::recv(s, &key, sizeof(key), MSG_WAITALL) < 0)
    {
      printf("pdsdaq_connect(): get dbkey failed\n");
      break;
    }

    daq->dbkey  = key;

#ifdef DBUG
    printf("pdsdaq_connect received dbkey %d\n",daq->dbkey);
#endif

    /*
     * Get dbalias
     */
    len=0;
    if (::recv(s, &len, sizeof(len), MSG_WAITALL) < 0)
    {
      printf("pdsdaq_connect(): get alias len failed\n");
      break;
    }

    if (len==0)
    {
      printf("pdsdaq_connect(): get alias len = 0\n");
      break;
    }

    if (::recv(s, buff, len, MSG_WAITALL) != len)
    {
      printf("pdsdaq_connect(): get alias string failed\n");
      break;
    }

    buff[len] = 0;
    strcpy(daq->dbalias,buff);

#ifdef DBUG
    printf("pdsdaq_connect received dbalias %s\n",daq->dbalias);
#endif

    /*
    * Get partition selection
    */
    if (::recv(s, &len, sizeof(len), MSG_WAITALL) < 0)
    {
      printf("pdsdaq_connect(): get partition len failed\n");
      break;
    }

    if (len==0)
    {
      printf("pdsdaq_connect(): get partition len = 0\n");
      break;
    }

    if (::recv(s, daq->partition, len, MSG_WAITALL) != len)
    {
      printf("pdsdaq_connect(): get partition failed\n");
      break;
    }

#ifdef DBUG
    printf("pdsdaq_connect received partition %p\n",daq->partition);
    for(unsigned j=0; j<daq->partition->nodes(); j++) {
      const RemoteNode& node = *daq->partition->node(j);
      printf("\t%s : %x : %s : %s\n",
       node.name(),
       node.phy(),
       node.readout() ? "Readout":"NoReadout",
       node.record () ? "Record":"NoRecord");
    }
#endif

    daq->state  = Connected;

    Py_INCREF(Py_None);
    return Py_None;
  }

  Py_DECREF(pdsdaq_disconnect(self));
  PyErr_SetString(PyExc_RuntimeError,"Initial query failed");
  return NULL;

}

PyObject* pdsdaq_dbpath   (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  return PyString_FromString(daq->dbpath);
}

PyObject* pdsdaq_dbalias   (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  return PyString_FromString(daq->dbalias);
}

PyObject* pdsdaq_dbkey    (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  return PyLong_FromLong(daq->dbkey);
}

PyObject* pdsdaq_partition(PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
#ifdef DBUG
  printf("pdsdaq_partition %p nnodes %d\n",daq->partition,daq->partition->nodes());
#endif
  //  Translate Partition to dictionary and return
  PyObject* o = PyList_New(daq->partition->nodes());
  for(unsigned j=0; j<daq->partition->nodes(); j++) {
    const RemoteNode& node = *daq->partition->node(j);
    PyObject* n = PyDict_New();
    PyDict_SetItemString(n, "id" , PyString_FromString(node.name()));
    PyDict_SetItemString(n, "phy", PyLong_FromUnsignedLong(node.phy()));
    PyDict_SetItemString(n, "readout", PyBool_FromLong(node.readout()));
    PyDict_SetItemString(n, "record" , PyBool_FromLong(node.record ()));
    PyList_SetItem(o, j, n);
  }
  return o;
}

PyObject* pdsdaq_runnum   (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  return PyLong_FromLong(daq->runinfo & 0xffff);
}

PyObject* pdsdaq_expt     (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  return PyLong_FromLong(daq->runinfo >> 16);
}

PyObject* pdsdaq_detectors (PyObject* self)
{
  Pds::DetInfo  d;
  unsigned i = (unsigned)Pds::DetInfo::NoDetector;
  PyObject* o = PyList_New(Pds::DetInfo::NumDetector);
  while (i<Pds::DetInfo::NumDetector)
  {
    PyList_SetItem(o, i, PyString_FromString(d.name((Pds::DetInfo::Detector)i)));
    i += 1;
  }
  return o;
}

PyObject* pdsdaq_devices (PyObject* self)
{
  Pds::DetInfo d;
  unsigned i = (unsigned) Pds::DetInfo::NoDevice;
  PyObject* o = PyList_New(Pds::DetInfo::NumDevice);
  while (i<(unsigned)Pds::DetInfo::NumDevice)
  {
    PyList_SetItem(o, i, PyString_FromString(d.name((Pds::DetInfo::Device)i)));
    i += 1;
  }
  return o;
}

PyObject* pdsdaq_types (PyObject* self)
{
  Pds::TypeId t;
  unsigned i = (unsigned) Pds::TypeId::Any;
  PyObject* o = PyList_New(Pds::TypeId::NumberOf);
  while (i<(unsigned)Pds::TypeId::NumberOf)
  {
    PyList_SetItem(o, i, PyString_FromString(t.name((Pds::TypeId::Type)i)));
    i += 1;
  }
  return o;
}

static bool ParseInt(PyObject* obj, int& var, const char* name)
{
  if (PyInt_Check(obj))
    var = PyInt_AsLong(obj);
  else if (PyLong_Check(obj))
    var = PyLong_AsLong(obj);
  else {
    ostringstream o;
    o << name << " is not of type Int";
    PyErr_SetString(PyExc_TypeError,o.str().c_str());
    return false;
  }
  return true;
}

PyObject* pdsdaq_configure(PyObject* self, PyObject* args, PyObject* kwds)
{
  pdsdaq*   daq      = (pdsdaq*)self;

  if (daq->state != Connected) {
    Py_DECREF(pdsdaq_disconnect(self));
    PyObject* o = pdsdaq_connect(self);
    if (o == NULL) return o;
    Py_DECREF(o);
  }

  int       key      = daq->dbkey;
  int       l1t_events = -1;
  int       l3t_events = -1;
  PyObject* record   = 0;
  PyObject* duration = 0;
  PyObject* controls = 0;
  PyObject* monitors = 0;
  PyObject* labels   = 0;
  PyObject* partition= 0;

  PyObject* keys = PyDict_Keys  (kwds);
  PyObject* vals = PyDict_Values(kwds);
  for(int i=0; i<PyList_Size(keys); i++) {
    const char* name = PyString_AsString(PyList_GetItem(keys,i));
    PyObject* obj = PyList_GetItem(vals,i);
    if (strcmp("key"     ,name)==0) {
      if (!ParseInt (obj,key   ,"key"   )) return NULL;
    }
    else if (strcmp("events"  ,name)==0) {
      if (!ParseInt (obj,l1t_events,"events")) return NULL;
    }
    else if (strcmp("l1t_events"  ,name)==0) {
      if (!ParseInt (obj,l1t_events,"l1t_events")) return NULL;
    }
    else if (strcmp("l3t_events"  ,name)==0) {
      if (!ParseInt (obj,l3t_events,"l3t_events")) return NULL;
    }
    else if (strcmp("record"  ,name)==0)  record  =obj;
    else if (strcmp("duration",name)==0)  duration=obj;
    else if (strcmp("controls",name)==0)  controls=obj;
    else if (strcmp("monitors",name)==0)  monitors=obj;
    else if (strcmp("labels"  ,name)==0)  labels  =obj;
    else if (strcmp("partition",name)==0) partition=obj;
    else {
      ostringstream o;
      o << name << " is not a valid keyword";
      PyErr_SetString(PyExc_TypeError,o.str().c_str());
      return NULL;
    }
  }

  daq->dbkey    = (key & DbKeyMask);

  uint32_t urecord = 0;
  if (record) {
    urecord |= RecordSetMask;
    if (record==Py_True)
      urecord |= RecordValMask;
  }

  uint32_t upartition = 0;
  if (partition)
    upartition |= ModifyPartition;

  uint32_t ukey = daq->dbkey | urecord | upartition;
  ::write(daq->socket, &ukey, sizeof(ukey));

  if (partition) {
    // Translate partition dict to Allocation and write
    if (PyList_Size(partition)!=daq->partition->nodes()) {
      printf("partition list size (%zd) does not match original size (%d).\nPartition unchanged.\n",
       PyList_Size(partition),daq->partition->nodes());
      PyErr_SetString(PyExc_RuntimeError,"Partition size changed.");
      return NULL;
    }
    else {
      for(unsigned j=0; j<daq->partition->nodes(); j++) {
  PyObject* n = PyList_GetItem(partition,j);
  daq->partition->node(j)->readout( PyDict_GetItemString(n,"readout")==Py_True );
  daq->partition->node(j)->record ( PyDict_GetItemString(n,"record" )==Py_True );
      }
      ::write(daq->socket, daq->partition, sizeof(*daq->partition));
    }
  }

  list<PVControl> clist;
  if (controls)
    for(unsigned i=0; i<PyList_Size(controls); i++) {
      PyObject* item = PyList_GetItem(controls,i);
      clist.push_back(PVControl(PyString_AsString(PyTuple_GetItem(item,0)),
                                PVControl::NoArray,
                                PyFloat_AsDouble (PyTuple_GetItem(item,1))));
    }

  list<PVMonitor> mlist;
  if (monitors)
    for(unsigned i=0; i<PyList_Size(monitors); i++) {
      PyObject* item = PyList_GetItem(monitors,i);
      mlist.push_back(PVMonitor(PyString_AsString(PyTuple_GetItem(item,0)),
                                PVMonitor::NoArray,
                                PyFloat_AsDouble (PyTuple_GetItem(item,1)),
                                PyFloat_AsDouble (PyTuple_GetItem(item,2))));
    }

  list<PVLabel> llist;
  if (labels)
    for(unsigned i=0; i<PyList_Size(labels); i++) {
      PyObject* item = PyList_GetItem(labels,i);
      llist.push_back(PVLabel  (PyString_AsString(PyTuple_GetItem(item,0)),
                                PyString_AsString(PyTuple_GetItem(item,1))));
    }

  ControlConfigType* cfg;

  if (duration) {
    Pds::ClockTime dur(PyLong_AsUnsignedLong(PyList_GetItem(duration,0)),
                       PyLong_AsUnsignedLong(PyList_GetItem(duration,1)));
    cfg = Pds::ControlConfig::_new(daq->buffer,clist,mlist,llist,dur);
  }
  else if (l1t_events>=0) {
    cfg = Pds::ControlConfig::_new(daq->buffer,clist,mlist,llist,l1t_events);
  }
  else if (l3t_events>=0) {
    cfg = Pds::ControlConfig::_new(daq->buffer,clist,mlist,llist,
                                   Pds::ControlConfig::L3TEvents(l3t_events));
  }
  else {
    PyErr_SetString(PyExc_RuntimeError,"Configuration lacks duration or events input.");
    return NULL;
  }

  daq->state = Configured;

  ::write(daq->socket,daq->buffer,cfg->_sizeof());

  return pdsdaq_rcv(self);
}

PyObject* pdsdaq_begin    (PyObject* self, PyObject* args, PyObject* kwds)
{
  pdsdaq*   daq      = (pdsdaq*)self;

  if (daq->state >= Running) {
    PyObject* o = pdsdaq_end(self);
    if (o == NULL) return o;
    Py_DECREF(o);
  }

  if (daq->state != Configured) {
    PyErr_SetString(PyExc_RuntimeError,"Not configured");
    return NULL;
  }

  int       l1t_events = -1;
  int       l3t_events = -1;
  PyObject* duration = 0;
  PyObject* controls = 0;
  PyObject* monitors = 0;
  PyObject* labels   = 0;

  PyObject* keys = PyDict_Keys  (kwds);
  PyObject* vals = PyDict_Values(kwds);
  for(int i=0; i<PyList_Size(keys); i++) {
    const char* name = PyString_AsString(PyList_GetItem(keys,i));
    PyObject* obj = PyList_GetItem(vals,i);
    if (strcmp("events"  ,name)==0) {
      if (!ParseInt (obj,l1t_events,"events")) return NULL;
    }
    else if (strcmp("l1t_events",name)==0) {
      if (!ParseInt (obj,l1t_events,"l1t_events")) return NULL;
    }
    else if (strcmp("l3t_events",name)==0) {
      if (!ParseInt (obj,l3t_events,"l3t_events")) return NULL;
    }
    else if (strcmp("duration",name)==0)  duration=obj;
    else if (strcmp("controls",name)==0)  controls=obj;
    else if (strcmp("monitors",name)==0)  monitors=obj;
    else if (strcmp("labels"  ,name)==0)  labels  =obj;
    else {
      ostringstream o;
      o << name << " is not a valid keyword";
      PyErr_SetString(PyExc_TypeError,o.str().c_str());
      return NULL;
    }
  }

  if (duration && (l1t_events!=-1 || l3t_events!=-1)) {
    PyErr_SetString(PyExc_TypeError,"Cannot specify both events and duration");
    return NULL;
  }

  if (l1t_events!=-1 && l3t_events!=-1) {
    PyErr_SetString(PyExc_TypeError,"Cannot specify both events (L1 and L3)");
    return NULL;
  }

  PyErr_Clear();

  ControlConfigType* cfg = reinterpret_cast<ControlConfigType*>(daq->buffer);

  list<PVControl> clist;
  { for(unsigned i=0; i<cfg->npvControls(); i++)
      clist.push_back(cfg->pvControls()[i]); }

  if (controls) {
    for(unsigned i=0; i<PyList_Size(controls); i++) {
      PyObject* item = PyList_GetItem(controls,i);
      const char* name = PyString_AsString(PyTuple_GetItem(item,0));
      list<PVControl>::iterator it=clist.begin();
      while(it!=clist.end()) {
        if (strcmp(name,it->name())==0) {
          (*it) = PVControl(name,PVControl::NoArray,PyFloat_AsDouble (PyTuple_GetItem(item,1)));
          break;
        }
        ++it;
      }
      if (it == clist.end()) {
        ostringstream o;
        o << "Control " << name << " not present in Configure";
        PyErr_SetString(PyExc_TypeError,o.str().c_str());
        return NULL;
      }
    }
  }

  list<PVMonitor> mlist;
  { for(unsigned i=0; i<cfg->npvMonitors(); i++)
      mlist.push_back(cfg->pvMonitors()[i]); }

  if (monitors) {
    for(unsigned i=0; i<PyList_Size(monitors); i++) {
      PyObject* item = PyList_GetItem(monitors,i);
      const char* name = PyString_AsString(PyTuple_GetItem(item,0));
      list<PVMonitor>::iterator it=mlist.begin();
      while(it!=mlist.end()) {
        if (strcmp(name,it->name())==0) {
          (*it) = PVMonitor(name,
                            PVMonitor::NoArray,
                            PyFloat_AsDouble (PyTuple_GetItem(item,1)),
                            PyFloat_AsDouble (PyTuple_GetItem(item,2)));
          break;
        }
        ++it;
      }
      if (it == mlist.end()) {
        ostringstream o;
        o << "Monitor " << name << " not present in Configure";
        PyErr_SetString(PyExc_TypeError,o.str().c_str());
        return NULL;
      }
    }
  }

  list<PVLabel> llist;
  { for(unsigned i=0; i<cfg->npvLabels(); i++)
      llist.push_back(cfg->pvLabels()[i]); }

  if (labels) {
    for(unsigned i=0; i<PyList_Size(labels); i++) {
      PyObject* item = PyList_GetItem(labels,i);
      const char* name = PyString_AsString(PyTuple_GetItem(item,0));
      list<PVLabel>::iterator it=llist.begin();
      while(it!=llist.end()) {
        if (strcmp(name,it->name())==0) {
          (*it) = PVLabel(name, PyString_AsString(PyTuple_GetItem(item,1)));
          break;
        }
        ++it;
      }
      if (it == llist.end()) {
        ostringstream o;
        o << "Label " << name << " not present in Configure";
        PyErr_SetString(PyExc_TypeError,o.str().c_str());
        return NULL;
      }
    }
  }

  if (duration) {
    Pds::ClockTime dur(PyLong_AsUnsignedLong(PyList_GetItem(duration,0)),
                       PyLong_AsUnsignedLong(PyList_GetItem(duration,1)));
    cfg = Pds::ControlConfig::_new(daq->buffer,clist,mlist,llist,dur);
  }
  else if (l1t_events>=0) {
    cfg = Pds::ControlConfig::_new(daq->buffer,clist,mlist,llist,l1t_events);
  }
  else if (l3t_events>=0) {
    cfg = Pds::ControlConfig::_new(daq->buffer,clist,mlist,llist,
                                   Pds::ControlConfig::L3TEvents(l3t_events));
  }
  else if (cfg->uses_duration()) {
    Pds::ClockTime dur(cfg->duration());
    cfg = Pds::ControlConfig::_new(daq->buffer,clist,mlist,llist,dur);
  }
  else if (cfg->uses_events()) {
    l1t_events = cfg->events();
    cfg = Pds::ControlConfig::_new(daq->buffer,clist,mlist,llist,l1t_events);
  }
  else if (cfg->uses_l3t_events()) {
    l3t_events = cfg->events();
    cfg = Pds::ControlConfig::_new(daq->buffer,clist,mlist,llist,
                                   Pds::ControlConfig::L3TEvents(l3t_events));
  }
  else {
    PyErr_SetString(PyExc_RuntimeError,"Begin lacks duration or events input.");
    return NULL;
  }

  ::write(daq->socket,daq->buffer,cfg->_sizeof());

  daq->state = Running;

  return pdsdaq_rcv(self);
}

PyObject* pdsdaq_end      (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  if (daq->state != Running) {
    PyErr_SetString(PyExc_RuntimeError,"Not running(begin)");
    return NULL;
  }

  daq->state = Configured;
  return pdsdaq_rcv(self);
}

PyObject* pdsdaq_stop     (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  if (daq->state >= Configured) {
    char* buff = new char[MaxConfigSize];
    ControlConfigType* cfg = Pds::ControlConfig::_new(buff,
                                                      list<PVControl>(),
                                                      list<PVMonitor>(),
                                                      list<PVLabel  >(),
                                                      Pds::ClockTime(0,0));
    ::write(daq->socket, buff, cfg->_sizeof());
    delete[] buff;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* pdsdaq_eventnum(PyObject* self)
{
  int64_t iEventNum = -1;

  pdsdaq* daq = (pdsdaq*)self;
  if (daq->state >= Configured) {
    Pds::RemoteSeqCmd cmd(Pds::RemoteSeqCmd::CMD_GET_CUR_EVENT_NUM);
    ::write(daq->socket, &cmd, sizeof(cmd));

    if (::recv(daq->socket,&iEventNum,sizeof(iEventNum),MSG_WAITALL) < 0)
    {
      ostringstream o;
      o << "pdsdap_eventnum(): recv() failed... disconnecting.";
      PyErr_SetString(PyExc_RuntimeError,o.str().c_str());
      Py_DECREF(pdsdaq_disconnect(self));
    }
  }

  return PyLong_FromLong(iEventNum);
}

PyObject* pdsdaq_rcv      (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  int32_t result = -1;
  if (::recv(daq->socket,&result,sizeof(result),MSG_WAITALL) < 0 ||
      result < 0) {
    ostringstream o;
    o << "Remote DAQ failed transition " << -result << "... disconnecting.";
    PyErr_SetString(PyExc_RuntimeError,o.str().c_str());
    Py_DECREF(pdsdaq_disconnect(self));
    return NULL;
  }

  daq->runinfo = result;

  Py_INCREF(Py_None);
  return Py_None;
}

//
//  Module methods
//
//

static PyMethodDef PydaqMethods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

//
//  Module initialization
//
PyMODINIT_FUNC
initpydaq(void)
{
  if (PyType_Ready(&pdsdaq_type) < 0)
    return;

  PyObject *m = Py_InitModule("pydaq", PydaqMethods);
  if (m == NULL)
    return;

  Py_INCREF(&pdsdaq_type);
  PyModule_AddObject(m, "Control" , (PyObject*)&pdsdaq_type);
}
