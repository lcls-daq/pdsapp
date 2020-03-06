//
//  pydaq module
//
//  Defines one python class: pdsdaq
//

//#define DBUG

#include <Python.h>
#include <structmember.h>
#include "p3compat.h"
#include "pdsapp/python/pydaq.hh"
#include "pdsapp/control/RemoteSeqCmd.hh"
#include "pdsapp/control/RemoteSeqResponse.hh"
#include "pds/config/ControlConfigType.hh"
#include "pdsdata/psddl/control.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
using Pds::ControlData::PVControl;
using Pds::ControlData::PVMonitor;
using Pds::ControlData::PVLabel;
#include "pdsapp/control/RemotePartition.hh"
using Pds::RemoteSeqResponse;
using Pds::RemotePartition;
using Pds::RemoteNode;

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <poll.h>
#include <sstream>
using std::ostringstream;

#include <list>
using std::list;

static const int MaxPathSize   = 0x100;
static const int MaxAliasSize  = 0x100;
static const int MaxConfigSize = 0x100000;
static const int Control_Port  = 10130;

enum PydaqState { Disconnected, Connected, Configured, Open, Running };

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
static PyObject* pdsdaq_record   (PyObject* self);
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
static PyObject* pdsdaq_wait     (PyObject* self);
static PyObject* pdsdaq_stop     (PyObject* self);
static PyObject* pdsdaq_endrun   (PyObject* self);
static PyObject* pdsdaq_eventnum (PyObject* self);
static PyObject* pdsdaq_l3eventnum(PyObject* self);
static PyObject* pdsdaq_state    (PyObject* self);
static PyObject* pdsdaq_rcv      (PyObject* self, int state, bool interrupt=false);
static PyObject* pdsdaq_clear    (PyObject* self);
#ifdef WITH_THREAD
static PyObject* pdsdaq_blocking (PyObject* self);
#endif
static bool      pdsdaq_lock     (PyObject* self, bool interrupt=true);
static void      pdsdaq_unlock   (PyObject* self);
//
//  pdsdaq class methods (thread safe versions)
//
#define WRAP_FUNC(function)                                       \
static PyObject* function ## _wrap (PyObject* self)               \
{                                                                 \
  if (pdsdaq_lock(self)) {                                        \
    PyObject* o = function(self);                                 \
    pdsdaq_unlock(self);                                          \
    return o;                                                     \
  } else {                                                        \
    PyErr_SetString(PyExc_RuntimeError,"Unable to obtain lock!"); \
    return NULL;                                                  \
  }                                                               \
}

#define WRAP_FUNC_WAIT(function)                                  \
static PyObject* function ## _wrap (PyObject* self)               \
{                                                                 \
  pdsdaq* daq = (pdsdaq*)self;                                    \
  if (pdsdaq_lock(self, false)) {                                 \
    daq->waiting++;                                               \
    PyObject* o = function(self);                                 \
    if (--daq->waiting < 0)                                       \
      daq->waiting = 0;                                           \
    pdsdaq_unlock(self);                                          \
    return o;                                                     \
  } else {                                                        \
    PyErr_SetString(PyExc_RuntimeError,"Unable to obtain lock!"); \
    return NULL;                                                  \
  }                                                               \
}

#define WRAP_FUNC_ARGS(function)                                                    \
static PyObject* function ## _wrap (PyObject* self, PyObject* args, PyObject* kwds) \
{                                                                                   \
  if (pdsdaq_lock(self)) {                                                          \
    PyObject* o = function(self, args, kwds);                                       \
    pdsdaq_unlock(self);                                                            \
    return o;                                                                       \
  } else {                                                                          \
    PyErr_SetString(PyExc_RuntimeError,"Unable to obtain lock!");                   \
    return NULL;                                                                    \
  }                                                                                 \
}

WRAP_FUNC_ARGS(pdsdaq_configure);
WRAP_FUNC_ARGS(pdsdaq_begin);
WRAP_FUNC_WAIT(pdsdaq_end);
WRAP_FUNC_WAIT(pdsdaq_wait);
WRAP_FUNC(pdsdaq_stop);
WRAP_FUNC(pdsdaq_endrun);
WRAP_FUNC(pdsdaq_eventnum);
WRAP_FUNC(pdsdaq_l3eventnum);
WRAP_FUNC(pdsdaq_connect);
WRAP_FUNC(pdsdaq_disconnect);
WRAP_FUNC(pdsdaq_state);
#undef WRAP_FUNC
#undef WRAP_FUNC_WAIT
#undef WRAP_FUNC_ARGS

static PyMethodDef pdsdaq_methods[] = {
  {"dbpath"    , (PyCFunction)pdsdaq_dbpath         , METH_NOARGS  , "Get database path"},
  {"dbalias"   , (PyCFunction)pdsdaq_dbalias        , METH_NOARGS  , "Get database alias"},
  {"dbkey"     , (PyCFunction)pdsdaq_dbkey          , METH_NOARGS  , "Get database key"},
  {"partition" , (PyCFunction)pdsdaq_partition      , METH_NOARGS  , "Get partition"},
  {"record"    , (PyCFunction)pdsdaq_record         , METH_NOARGS  , "Get record status"},
  {"runnumber" , (PyCFunction)pdsdaq_runnum         , METH_NOARGS  , "Get run number"},
  {"experiment", (PyCFunction)pdsdaq_expt           , METH_NOARGS  , "Get experiment name"},
  {"detectors" , (PyCFunction)pdsdaq_detectors      , METH_NOARGS  , "Get the detector names"},
  {"devices"   , (PyCFunction)pdsdaq_devices        , METH_NOARGS  , "Get the device names"},
  {"types"     , (PyCFunction)pdsdaq_types          , METH_NOARGS  , "Get the type names"},
  {"configure" , (PyCFunction)pdsdaq_configure_wrap , METH_VARARGS|METH_KEYWORDS, "Configure the scan"},
  {"begin"     , (PyCFunction)pdsdaq_begin_wrap     , METH_VARARGS|METH_KEYWORDS, "Configure the cycle"},
  {"end"       , (PyCFunction)pdsdaq_end_wrap       , METH_NOARGS  , "Wait for the cycle end"},
  {"wait"      , (PyCFunction)pdsdaq_wait_wrap      , METH_NOARGS  , "Wait for the cycle end"},
  {"stop"      , (PyCFunction)pdsdaq_stop_wrap      , METH_NOARGS  , "End the current cycle"},
  {"endrun"    , (PyCFunction)pdsdaq_endrun_wrap    , METH_NOARGS  , "End the current run"},
  {"eventnum"  , (PyCFunction)pdsdaq_eventnum_wrap  , METH_NOARGS  , "Get current event number"},
  {"l3eventnum", (PyCFunction)pdsdaq_l3eventnum_wrap, METH_NOARGS  , "Get current l3pass event number"},
  {"connect"   , (PyCFunction)pdsdaq_connect_wrap   , METH_NOARGS  , "Connect to control"},
  {"disconnect", (PyCFunction)pdsdaq_disconnect_wrap, METH_NOARGS  , "Disconnect from control"},
  {"state"     , (PyCFunction)pdsdaq_state_wrap     , METH_NOARGS  , "Get the control state"},
#ifdef WITH_THREAD
  {"blocking"  , (PyCFunction)pdsdaq_blocking       , METH_NOARGS , "Get blocking enabled status"},
#endif
  {NULL},
};

//
//  Register pdsdaq members
//
static PyMemberDef pdsdaq_members[] = {
  {NULL}
};

static PyTypeObject pdsdaq_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
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

  for (unsigned i=0; i<(sizeof(self->signal)/sizeof(int)); i++) {
    if (self->signal[i] >= 0) {
      ::close(self->signal[i]);
    }
  }

  if (self->buffer) {
    delete[] self->dbpath;
    delete[] self->dbalias;
    delete   self->partition;
    delete[] self->buffer;
    delete[] self->exptname;
  }

#ifdef WITH_THREAD
  if (self->lock) {
    PyThread_free_lock(self->lock);
    self->lock = NULL;
  }
#endif

  Py_TYPE(self)->tp_free((PyObject*)self);
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
    self->record   = false;
    self->partition= new RemotePartition;
    self->buffer   = new char[MaxConfigSize];
    self->exptname = new char[RemoteSeqResponse::MaxExpName];
    self->runnum   = -1;
    self->waiting  = 0;
    self->pending  = 0;
    for (unsigned i=0; i<(sizeof(self->signal)/sizeof(int)); i++) {
      self->signal[i] = -1;
    }
    /*
     * Initialize char arrays to zero
     */
    memset(self->dbpath, 0, MaxPathSize);
    memset(self->dbalias, 0, MaxAliasSize);
    memset(self->exptname, 0, RemoteSeqResponse::MaxExpName);
#ifdef WITH_THREAD
    self->blocking = true;
    self->lock     = NULL;
#endif
  }

  return (PyObject*)self;
}

int pdsdaq_init(pdsdaq* self, PyObject* args, PyObject* kwds)
{
  const char* kwlist[] = {"host","platform","blocking",NULL};
  unsigned    addr  = 0;
  const char* host  = 0;
  unsigned    platform = 0;
  PyObject*   blocking = NULL;

  while(1) {
    if (PyArg_ParseTupleAndKeywords(args,kwds,"s|IO",const_cast<char**>(kwlist),
                                    &host,&platform,&blocking)) {

      hostent* entries = gethostbyname(host);
      if (entries) {
        addr = htonl(*(in_addr_t*)entries->h_addr_list[0]);
        break;
      }
    }
    if (PyArg_ParseTupleAndKeywords(args,kwds,"I|iO",const_cast<char**>(kwlist),
                                    &addr,&platform,&blocking)) {
      break;
    }

    return -1;
  }

  PyErr_Clear();

  self->addr     = addr;
  self->platform = platform;
  self->socket   = -1;

  if (::pipe(self->signal) < 0) {
    PyErr_SetFromErrno(PyExc_OSError);
    return -1;
  }

#ifdef WITH_THREAD
  if (blocking) {
    self->blocking = PyObject_IsTrue(blocking);
  }
  self->lock = PyThread_allocate_lock();
  if (self->lock == NULL) {
    PyErr_SetString(PyExc_MemoryError, "Unable to allocate thread lock.");
    return -1;
  }
#endif

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

  int nb;
  char buff[256];
  uint32_t len=0;
  uint32_t key;

  Py_BEGIN_ALLOW_THREADS
  nb = ::connect(s, (sockaddr*)&sa, sizeof(sa));
  Py_END_ALLOW_THREADS

  if (nb < 0) {
    ::close(s);
    PyErr_SetString(PyExc_RuntimeError,"Connect failed");
    return NULL;
  }

  while(1) {

    Py_BEGIN_ALLOW_THREADS
    nb = ::recv(s, &len, sizeof(len), MSG_WAITALL);
    Py_END_ALLOW_THREADS

    if (nb < 0)
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

    Py_BEGIN_ALLOW_THREADS
    nb = ::recv(s, buff, len, MSG_WAITALL);
    Py_END_ALLOW_THREADS

    if (nb != int(len))
    {
      printf("pdsdaq_connect(): get dbpath string failed\n");
      break;
    }

    buff[len] = 0;
    strcpy(daq->dbpath,buff);

#ifdef DBUG
    printf("pdsdaq_connect received dbpath %s\n",daq->dbpath);
#endif

    /*
     * Get dbkey
     */

    Py_BEGIN_ALLOW_THREADS
    nb = ::recv(s, &key, sizeof(key), MSG_WAITALL);
    Py_END_ALLOW_THREADS

    if (nb < 0)
    {
      printf("pdsdaq_connect(): get dbkey failed\n");
      break;
    }

    daq->dbkey  = key&DbKeyMask;
    daq->record = key&RecordValMask;

#ifdef DBUG
    printf("pdsdaq_connect received dbkey %d\n",daq->dbkey);
#endif

    /*
     * Get dbalias
     */
    len=0;
    Py_BEGIN_ALLOW_THREADS
    nb = ::recv(s, &len, sizeof(len), MSG_WAITALL);
    Py_END_ALLOW_THREADS

    if (nb < 0)
    {
      printf("pdsdaq_connect(): get alias len failed\n");
      break;
    }

    if (len==0)
    {
      printf("pdsdaq_connect(): get alias len = 0\n");
      break;
    }

    Py_BEGIN_ALLOW_THREADS
    nb = ::recv(s, buff, len, MSG_WAITALL);
    Py_END_ALLOW_THREADS

    if (nb != int(len))
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
    Py_BEGIN_ALLOW_THREADS
    nb = ::recv(s, &len, sizeof(len), MSG_WAITALL);
    Py_END_ALLOW_THREADS

    if (nb < 0)
    {
      printf("pdsdaq_connect(): get partition len failed\n");
      break;
    }

    if (len==0)
    {
      printf("pdsdaq_connect(): get partition len = 0\n");
      break;
    }

    RemotePartition partition;

    Py_BEGIN_ALLOW_THREADS
    nb = ::recv(s, &partition, len, MSG_WAITALL);
    Py_END_ALLOW_THREADS

    if (nb != int(len))
    {
      printf("pdsdaq_connect(): get partition failed\n");
      break;
    }

    daq->socket = s;
    *daq->partition = partition;
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

  daq->socket = s;
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

PyObject* pdsdaq_record   (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  return PyBool_FromLong(daq->record);
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
  PyObject* p = PyDict_New();
  PyDict_SetItemString(p, "l3tag" , PyBool_FromLong    (daq->partition->l3tag()));
  PyDict_SetItemString(p, "l3veto", PyBool_FromLong    (daq->partition->l3veto()));
  PyDict_SetItemString(p, "l3unbiasedFraction", PyFloat_FromDouble(daq->partition->l3uf()));
  PyDict_SetItemString(p, "l3path", PyString_FromString(daq->partition->l3path()));
  PyDict_SetItemString(p, "nodes", o);

  return p;
}

PyObject* pdsdaq_runnum   (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  return PyLong_FromLong(daq->runnum);
}

PyObject* pdsdaq_expt     (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  return PyString_FromString(daq->exptname);
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

  if (kwds) { // No kwargs is null instead of 'empty' dict
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
  }

  daq->dbkey    = (key & DbKeyMask);

  uint32_t urecord = 0;
  if (record) {
    urecord |= RecordSetMask;
    if (record==Py_True) {
      urecord |= RecordValMask;
      daq->record = true;
    }
    else
      daq->record = false;
  }

  uint32_t upartition = 0;
  if (partition)
    upartition |= ModifyPartition;

  uint32_t ukey = daq->dbkey | urecord | upartition;
  ::write(daq->socket, &ukey, sizeof(ukey));

  if (partition) {
    // Translate partition dict to Allocation and write
    if (PyDict_GetItemString(partition,"l3tag")==Py_True) {
      daq->partition->set_l3t( PyString_AsString(PyDict_GetItemString(partition,"l3path")),
                               PyDict_GetItemString(partition,"l3veto")==Py_True,
                               PyFloat_AsDouble (PyDict_GetItemString(partition,"l3unbiasedFraction")) );
    }
    else
      daq->partition->clear_l3t();

    PyObject* nodes = PyDict_GetItemString(partition,"nodes");
    if (PyList_Size(nodes)!=daq->partition->nodes()) {
      printf("partition list size (%zd) does not match original size (%d).\nPartition unchanged.\n",
       PyList_Size(nodes),daq->partition->nodes());
      PyErr_SetString(PyExc_RuntimeError,"Partition size changed.");
      // Half configuring leaves the DAQ in a strange state so disconnect to go to unconfigured!
      Py_DECREF(pdsdaq_disconnect(self));
      return NULL;
    }
    else {
      for(unsigned j=0; j<daq->partition->nodes(); j++) {
        PyObject* n = PyList_GetItem(nodes,j);
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
    // Half configuring leaves the DAQ in a strange state so disconnect to go to unconfigured!
    Py_DECREF(pdsdaq_disconnect(self));
    return NULL;
  }

  ::write(daq->socket,daq->buffer,cfg->_sizeof());

  return pdsdaq_rcv(self, Configured);
}

PyObject* pdsdaq_begin    (PyObject* self, PyObject* args, PyObject* kwds)
{
  pdsdaq*   daq      = (pdsdaq*)self;

  if (daq->state >= Running) {
    PyObject* o = pdsdaq_clear(self);
    if (o == NULL) {
      PyErr_SetString(PyExc_RuntimeError,"Already running");
      return o;
    }
    Py_DECREF(o);
  }

  if (daq->state < Configured) {
    PyErr_SetString(PyExc_RuntimeError,"Not configured");
    return NULL;
  }

  int       l1t_events = -1;
  int       l3t_events = -1;
  PyObject* duration = 0;
  PyObject* controls = 0;
  PyObject* monitors = 0;
  PyObject* labels   = 0;

  if (kwds) { // No kwargs is null instead of 'empty' dict
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
        if (strncmp(name,it->name(),PVControl::NameSize)==0) {
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
        if (strncmp(name,it->name(),PVMonitor::NameSize)==0) {
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
        if (strncmp(name,it->name(),PVLabel::NameSize)==0) {
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

  return pdsdaq_rcv(self, Running);
}

PyObject* pdsdaq_end      (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;

  if (daq->state != Running) {
    PyErr_SetString(PyExc_RuntimeError,"Not running(begin)");
    return NULL;
  }

  return pdsdaq_rcv(self, Open, true);
}

PyObject* pdsdaq_wait      (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;

  if (daq->state != Running) {
    Py_INCREF(Py_None);
      return Py_None;
  }

  return pdsdaq_rcv(self, Open, true);
}

PyObject* pdsdaq_stop     (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;

  if (daq->state >= Running) {
    char* buff = new char[MaxConfigSize];
    ControlConfigType* cfg = Pds::ControlConfig::_new(buff,
                                                      list<PVControl>(),
                                                      list<PVMonitor>(),
                                                      list<PVLabel  >(),
                                                      EndCalibTime);
    ::write(daq->socket, buff, cfg->_sizeof());
    delete[] buff;

    return pdsdaq_rcv(self, Open);
  } else {
    Py_INCREF(Py_None);
    return Py_None;
  }
}

PyObject* pdsdaq_endrun   (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  if (daq->state >= Open) {
    char* buff = new char[MaxConfigSize];
    ControlConfigType* cfg = Pds::ControlConfig::_new(buff,
                                                      list<PVControl>(),
                                                      list<PVMonitor>(),
                                                      list<PVLabel  >(),
                                                      EndRunTime);
    ::write(daq->socket, buff, cfg->_sizeof());
    delete[] buff;

    // If we are running wait for the end Calib cycle otherwise just return
    if (daq->state >= Running) {
      return pdsdaq_rcv(self, Configured);
    } else {
      daq->state = Configured;
      Py_INCREF(Py_None);
      return Py_None;
    }
  } else {
    Py_INCREF(Py_None);
    return Py_None;
  }
}

PyObject* pdsdaq_eventnum(PyObject* self)
{
  int64_t iEventNum = -1;

  pdsdaq* daq = (pdsdaq*)self;
  if (daq->state >= Configured) {
    // Automatic Calib cycle transisitions may have left messages in socket
    while (true) {
      PyObject* o = pdsdaq_clear(self);
      if (o == NULL) break;
      Py_DECREF(o);
    }

    int s = daq->socket;
    Pds::RemoteSeqCmd cmd(Pds::RemoteSeqCmd::CMD_GET_CUR_EVENT_NUM);
    ::write(s, &cmd, sizeof(cmd));

    int nb;
    Py_BEGIN_ALLOW_THREADS
    nb = ::recv(s,&iEventNum,sizeof(iEventNum),MSG_WAITALL);
    Py_END_ALLOW_THREADS

    if (nb < 0)
    {
      ostringstream o;
      o << "pdsdap_eventnum(): recv() failed... disconnecting.";
      PyErr_SetString(PyExc_RuntimeError,o.str().c_str());
      Py_DECREF(pdsdaq_disconnect(self));
    }
  }

  return PyLong_FromLong(iEventNum);
}

PyObject* pdsdaq_l3eventnum(PyObject* self)
{
  int64_t iEventNum = -1;

  pdsdaq* daq = (pdsdaq*)self;
  if (daq->state >= Configured) {
    // Automatic Calib cycle transisitions may have left messages in socket
    while (true) {
      PyObject* o = pdsdaq_clear(self);
      if (o == NULL) break;
      Py_DECREF(o);
    }

    int s = daq->socket;
    Pds::RemoteSeqCmd cmd(Pds::RemoteSeqCmd::CMD_GET_CUR_L3EVENT_NUM);
    ::write(s, &cmd, sizeof(cmd));

    int nb;
    Py_BEGIN_ALLOW_THREADS
    nb = ::recv(s,&iEventNum,sizeof(iEventNum),MSG_WAITALL);
    Py_END_ALLOW_THREADS

    if (nb < 0)
    {
      ostringstream o;
      o << "pdsdap_l3eventnum(): recv() failed... disconnecting.";
      PyErr_SetString(PyExc_RuntimeError,o.str().c_str());
      Py_DECREF(pdsdaq_disconnect(self));
    }
  }

  return PyLong_FromLong(iEventNum);
}

PyObject* pdsdaq_state(PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;

  if (daq->state >= Configured) {
    // Automatic Calib cycle transisitions may have left messages in socket
    while (true) {
      PyObject* o = pdsdaq_clear(self);
      if (o == NULL) break;
      Py_DECREF(o);
    }
    // Suppress the runtime warnings that asking for the state may generate
    PyErr_Clear();
  }

  return PyInt_FromLong(daq->state);
}

PyObject* pdsdaq_rcv      (PyObject* self, int state, bool interrupt)
{
  pdsdaq* daq = (pdsdaq*)self;
  RemoteSeqResponse result;
  int s = daq->socket;
  int sig = daq->signal[0];
  int r = -1;
  int rsig = 0;
  int msg = 0;
  if (interrupt) { // this receive can be interrupted by request
    bool wait = true;
    struct pollfd pfds[] = {
      { s,    POLLIN, 0 },
      { sig,  POLLIN, 0 },
    };
    struct timespec sleep_time = {0, 100000}; // 100 us
    do {
      while (daq->pending) {
        pdsdaq_unlock(self);
        Py_BEGIN_ALLOW_THREADS
        nanosleep(&sleep_time, NULL);
        Py_END_ALLOW_THREADS
        pdsdaq_lock(self, false);
      }
      // if someone else got our transition just exit
      if (daq->state <= state) {
        Py_INCREF(Py_None);
        return Py_None;
      }
      Py_BEGIN_ALLOW_THREADS
      if (::poll(pfds, (nfds_t) (sizeof(pfds)/sizeof(struct pollfd)), -1) < 0) {
        wait = false;
      } else {
        for (unsigned n=0; n<(sizeof(pfds)/sizeof(struct pollfd)); n++) {
          if (pfds[n].revents & POLLIN) {
            if (pfds[n].fd == s) {
              r = ::recv(s,&result,sizeof(result),MSG_WAITALL);
              wait = false;
            } else if (pfds[n].fd == sig) {
              rsig = ::read(sig,&msg,sizeof(msg));
              if (rsig != sizeof(msg) || msg) {
                wait = false;
              }
            }
          }
        }
      }
      Py_END_ALLOW_THREADS
    } while (wait);
  } else {
    Py_BEGIN_ALLOW_THREADS
    r = ::recv(s,&result,sizeof(result),MSG_WAITALL);
    Py_END_ALLOW_THREADS
  }
  if (r<0) {
    PyErr_SetString(PyExc_ValueError,"pdsdaq_rcv interrupted");
    return NULL;
  }
  else if (r==0) {
    PyErr_SetString(PyExc_RuntimeError,"Remote DAQ has closed the connection... disconnecting.");
    Py_DECREF(pdsdaq_disconnect(self));
    return NULL;
  }
  else if (result.damage()) {
    ostringstream o;
    o << "Remote DAQ failed "
      << Pds::TransitionId::name(result.id())
      << " transition... disconnecting.";
    PyErr_SetString(PyExc_RuntimeError,o.str().c_str());
    Py_DECREF(pdsdaq_disconnect(self));
    return NULL;
  }
  else if ((result.id() == Pds::TransitionId::Enable) && (daq->state >= Running)) {
    // Ignore EVR enables from new chunks
    return pdsdaq_rcv(self, state, interrupt);
  }
  else {
    strcpy(daq->exptname, result.exptname());
    daq->runnum  = result.runnum ();
    daq->state   = state;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

#ifdef WITH_THREAD
PyObject* pdsdaq_blocking (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  return PyBool_FromLong(daq->blocking);
}
#endif

PyObject* pdsdaq_clear    (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  RemoteSeqResponse result;
  int s = daq->socket;
  int nb;

  Py_BEGIN_ALLOW_THREADS
  nb = ::recv(s,&result,sizeof(result),MSG_DONTWAIT | MSG_PEEK);
  Py_END_ALLOW_THREADS

  if (nb < 0) {
    return NULL; // nothing left to read
  } else if (daq->state >= Running) {
    return pdsdaq_rcv(self, Open);
  } else {
    return pdsdaq_rcv(self, daq->state);
  }
}

bool pdsdaq_lock    (PyObject* self, bool interrupt)
{
  pdsdaq* daq = (pdsdaq*)self;
#ifdef WITH_THREAD
  if (! PyThread_acquire_lock(daq->lock, NOWAIT_LOCK)) {
    if (daq->blocking) {
      int rc;
      if (interrupt && (daq->waiting)) {
        if (daq->pending++ == 0) {
          int msg = 0;
          ::write(daq->signal[1], &msg, sizeof(msg));
        }
      }
      Py_BEGIN_ALLOW_THREADS
      rc = PyThread_acquire_lock(daq->lock, WAIT_LOCK);
      Py_END_ALLOW_THREADS
      if (interrupt) {
        if (--daq->pending < 0)
          daq->pending = 0;
      }
      return rc;
    } else {
      return false;
    }
  }
#endif
  return true;
}

void pdsdaq_unlock  (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
#ifdef WITH_THREAD
  PyThread_release_lock(daq->lock);
#endif
}

//
//  Module methods
//
//

static PyMethodDef PydaqMethods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

#ifdef IS_PY3K
static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "pydaq",
        NULL,
        -1,
        PydaqMethods,
        NULL,
        NULL,
        NULL,
        NULL
};
#endif

//
//  Module initialization
//
DECLARE_INIT(pydaq)
{
  if (PyType_Ready(&pdsdaq_type) < 0)
    INITERROR;

#ifdef IS_PY3K
  PyObject *m = PyModule_Create(&moduledef);
#else
  PyObject *m = Py_InitModule("pydaq", PydaqMethods);
#endif
  if (m == NULL)
    INITERROR;

  Py_INCREF(&pdsdaq_type);
  PyModule_AddObject(m, "Control" , (PyObject*)&pdsdaq_type);

#ifdef IS_PY3K
  return m;
#endif
}
