//
//  pydaq module
//
//  Defines one python class: pdsdaq
//

#include <Python.h>
#include <structmember.h>

#include "pdsapp/python/pydaq.hh"
#include "pds/config/ControlConfigType.hh"
#include "pdsdata/control/PVControl.hh"
#include "pdsdata/control/PVMonitor.hh"
#include "pdsdata/control/PVLabel.hh"
using Pds::ControlData::PVControl;
using Pds::ControlData::PVMonitor;
using Pds::ControlData::PVLabel;

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
static const int MaxConfigSize = 0x100000;
static const int Control_Port  = 10130;

static const unsigned RecordSetMask = 0x20000;
static const unsigned RecordValMask = 0x10000;
static const unsigned DbKeyMask     = 0x0ffff;

enum PydaqState { Disconnected, Connected, Configured, Running };

//
//  pdsdaq class methods
//
static void      pdsdaq_dealloc(pdsdaq* self);
static PyObject* pdsdaq_new    (PyTypeObject* type, PyObject* args, PyObject* kwds);
static int       pdsdaq_init   (pdsdaq* self, PyObject* args, PyObject* kwds);
static PyObject* pdsdaq_dbpath   (PyObject* self);
static PyObject* pdsdaq_dbkey    (PyObject* self);
static PyObject* pdsdaq_runnum   (PyObject* self);
static PyObject* pdsdaq_expt     (PyObject* self);
static PyObject* pdsdaq_disconnect(PyObject* self);
static PyObject* pdsdaq_connect  (PyObject* self);
static PyObject* pdsdaq_configure(PyObject* self, PyObject* args, PyObject* kwds);
static PyObject* pdsdaq_begin    (PyObject* self, PyObject* args, PyObject* kwds);
static PyObject* pdsdaq_end      (PyObject* self);
static PyObject* pdsdaq_stop     (PyObject* self);
static PyObject* pdsdaq_rcv      (PyObject* self);

static PyMethodDef pdsdaq_methods[] = {
  {"dbpath"    , (PyCFunction)pdsdaq_dbpath    , METH_NOARGS  , "Get database path"},
  {"dbkey"     , (PyCFunction)pdsdaq_dbkey     , METH_NOARGS  , "Get database key"},
  {"runnumber" , (PyCFunction)pdsdaq_runnum    , METH_NOARGS  , "Get run number"},
  {"experiment", (PyCFunction)pdsdaq_expt      , METH_NOARGS  , "Get experiment number"},
  {"configure" , (PyCFunction)pdsdaq_configure , METH_KEYWORDS, "Configure the scan"},
  {"begin"     , (PyCFunction)pdsdaq_begin     , METH_KEYWORDS, "Configure the cycle"},
  {"end"       , (PyCFunction)pdsdaq_end       , METH_NOARGS  , "Wait for the cycle end"},
  {"stop"      , (PyCFunction)pdsdaq_stop      , METH_NOARGS  , "End the current cycle"},
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
    self->dbkey    = 0;
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
    uint32_t len;
    if (::recv(s, &len, sizeof(len), MSG_WAITALL) < 0)
      break;

    char buff[256];
    if (::recv(s, buff, len, MSG_WAITALL) != len)
      break;
    buff[len] = 0;
    *strrchr(buff,'/') = 0;

    uint32_t key;
    if (::recv(s, &key, sizeof(key), MSG_WAITALL) < 0)
      break;

    strcpy(daq->dbpath,buff);
    daq->dbkey  = key;

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

PyObject* pdsdaq_dbkey    (PyObject* self)
{
  pdsdaq* daq = (pdsdaq*)self;
  return PyLong_FromLong(daq->dbkey);
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
  int       events   = -1;
  PyObject* record   = 0;
  PyObject* duration = 0;
  PyObject* controls = 0;
  PyObject* monitors = 0;
  PyObject* labels   = 0;

  PyObject* keys = PyDict_Keys  (kwds);
  PyObject* vals = PyDict_Values(kwds);
  for(int i=0; i<PyList_Size(keys); i++) {
    const char* name = PyString_AsString(PyList_GetItem(keys,i));
    PyObject* obj = PyList_GetItem(vals,i);
    if (strcmp("key"     ,name)==0) {
      if (!ParseInt (obj,key   ,"key"   )) return NULL;
    }
    else if (strcmp("events"  ,name)==0) {
      if (!ParseInt (obj,events,"events")) return NULL;
    }
    else if (strcmp("record"  ,name)==0)  record  =obj;
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

  daq->dbkey    = (key & DbKeyMask);

  uint32_t urecord = 0;
  if (record) {
    urecord |= RecordSetMask;
    if (record==Py_True)
      urecord |= RecordValMask;
  }

  uint32_t ukey = daq->dbkey | urecord;
  ::write(daq->socket, &ukey, sizeof(ukey));

  list<PVControl> clist;
  if (controls)
    for(unsigned i=0; i<PyList_Size(controls); i++) {
      PyObject* item = PyList_GetItem(controls,i);
      clist.push_back(PVControl(PyString_AsString(PyTuple_GetItem(item,0)),
                                PyFloat_AsDouble (PyTuple_GetItem(item,1))));
    }

  list<PVMonitor> mlist;
  if (monitors)
    for(unsigned i=0; i<PyList_Size(monitors); i++) {
      PyObject* item = PyList_GetItem(monitors,i);
      mlist.push_back(PVMonitor(PyString_AsString(PyTuple_GetItem(item,0)),
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
    cfg = new (daq->buffer) ControlConfigType(clist,mlist,llist,dur);
  }
  else {
    cfg = new (daq->buffer) ControlConfigType(clist,mlist,llist,events);
  }

  daq->state = Configured;

  ::write(daq->socket,daq->buffer,cfg->size());

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

  int       events   = -1;
  PyObject* duration = 0;
  PyObject* controls = 0;
  PyObject* monitors = 0;
  PyObject* labels   = 0;

  while(1) {
    { char* kwlist[] = {"events"  ,"controls","monitors","labels",NULL};
      if ( PyArg_ParseTupleAndKeywords(args,kwds,"i|OOO",kwlist,
                                       &events, &controls, &monitors, &labels) )
        break; }
    { char* kwlist[] = {"duration","controls","monitors","labels",NULL};
      if ( PyArg_ParseTupleAndKeywords(args,kwds,"O|OO",kwlist,
                                       &duration, &controls, &monitors, &labels) )
        break; }
    { char* kwlist[] = {"controls","monitors","labels",NULL};
      if ( PyArg_ParseTupleAndKeywords(args,kwds,"|OOO",kwlist,
                                       &controls, &monitors, &labels) )
        break; }

    return NULL;
  }

  PyErr_Clear();

  ControlConfigType* cfg = reinterpret_cast<ControlConfigType*>(daq->buffer);

  list<PVControl> clist;
  { for(unsigned i=0; i<cfg->npvControls(); i++)
      clist.push_back(cfg->pvControl(i)); }

  if (controls) {
    for(unsigned i=0; i<PyList_Size(controls); i++) {
      PyObject* item = PyList_GetItem(controls,i);
      const char* name = PyString_AsString(PyTuple_GetItem(item,0));
      list<PVControl>::iterator it=clist.begin(); 
      do {
        if (strcmp(name,it->name())==0) {
          (*it) = PVControl(name,PyFloat_AsDouble (PyTuple_GetItem(item,1)));
          break;
        }
      } while (++it!= clist.end());
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
      mlist.push_back(cfg->pvMonitor(i)); }

  if (monitors) {
    for(unsigned i=0; i<PyList_Size(monitors); i++) {
      PyObject* item = PyList_GetItem(monitors,i);
      const char* name = PyString_AsString(PyTuple_GetItem(item,0));
      list<PVMonitor>::iterator it=mlist.begin(); 
      do {
        if (strcmp(name,it->name())==0) {
          (*it) = PVMonitor(name,
                            PyFloat_AsDouble (PyTuple_GetItem(item,1)),
                            PyFloat_AsDouble (PyTuple_GetItem(item,2)));
          break;
        }
      } while (++it!= mlist.end());
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
      llist.push_back(cfg->pvLabel(i)); }

  if (labels) {
    for(unsigned i=0; i<PyList_Size(labels); i++) {
      PyObject* item = PyList_GetItem(labels,i);
      const char* name = PyString_AsString(PyTuple_GetItem(item,0));
      list<PVLabel>::iterator it=llist.begin(); 
      do {
        if (strcmp(name,it->name())==0) {
          (*it) = PVLabel(name, PyString_AsString(PyTuple_GetItem(item,1)));
          break;
        }
      } while (++it!= llist.end());
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
    cfg = new (daq->buffer) ControlConfigType(clist,mlist,llist,dur);
  }
  else if (events>=0) {
    cfg = new (daq->buffer) ControlConfigType(clist,mlist,llist,events);
  }
  else if (cfg->uses_duration()) {
    ClockTime dur(cfg->duration());
    cfg = new (daq->buffer) ControlConfigType(clist,mlist,llist,dur);
  }
  else {
    unsigned events = cfg->events();
    cfg = new (daq->buffer) ControlConfigType(clist,mlist,llist,events);
  }

  ::write(daq->socket,daq->buffer,cfg->size());

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
  if (daq->state == Running) {
    ControlConfigType* cfg = new (daq->buffer) ControlConfigType(list<PVControl>(),
                                                                 list<PVMonitor>(),
                                                                 list<PVLabel  >(),
                                                                 ClockTime(0,0));
    ::write(daq->socket,daq->buffer,cfg->size());
  }

  Py_INCREF(Py_None);
  return Py_None;
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
