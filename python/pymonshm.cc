//
//  pymonshm module
//
//  Defines one python class: pdsmonshm
//

//#define DBUG

#include <Python.h>
#include <structmember.h>
#include "p3compat.h"
#include "pdsapp/python/pymonshm.hh"
#include "pdsdata/app/XtcMonitorServer.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Dgram.hh"

#include <string.h>
#include <sstream>
using std::ostringstream;

#include <list>
using std::list;

//
//  This is largely a copy of PadMonServer with some simplifications
//

namespace Pds {

  class MyMonitorServer : public XtcMonitorServer {
  public:
    MyMonitorServer(const char* tag,
                    unsigned sizeofBuffers, 
                    unsigned numberofEvBuffers, 
                    unsigned numberofClients) :
      XtcMonitorServer(tag,
                       sizeofBuffers,
                       numberofEvBuffers,
                       numberofClients),
      _sizeofBuffers(sizeofBuffers)
    {
      _init();
      distribute(true);
    }
    ~MyMonitorServer() 
    {
    }
  public:
    XtcMonitorServer::Result events(Dgram* dg) {
      if (XtcMonitorServer::events(dg) == XtcMonitorServer::Handled)
        _deleteDatagram(dg);
      return XtcMonitorServer::Deferred;
    }
    Dgram* newDatagram() 
    { 
      return (Dgram*)new char[_sizeofBuffers];
    }
    void   deleteDatagram(Dgram* dg) { _deleteDatagram(dg); }
  private:
    void  _deleteDatagram(Dgram* dg)
    {
      delete[] (char*)dg;
    }
  private:
    size_t _sizeofBuffers;
  };

  class FrameProcessor {
  public:
    virtual ~FrameProcessor() {}
    virtual Dgram* configure(Dgram*) = 0;
    virtual Dgram* event    (Dgram*, const char*) = 0;
  };
};

using namespace Pds;

static const ProcInfo segInfo(Pds::Level::Segment,0,0);
static const DetInfo  srcInfo(0,DetInfo::CxiEndstation,0,DetInfo::Opal1000,0);
static unsigned _ievent = 0;

//
//  Insert a simulated transition
//
static Dgram* insert(Dgram*              dg,
                     TransitionId::Value tr)
{
  timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);

  new((void*)&dg->seq) Sequence(Sequence::Event, 
                                tr, 
                                ClockTime(tv.tv_sec,tv.tv_nsec), 
                                TimeStamp(0,0x1ffff,
                                          tr==TransitionId::L1Accept?_ievent++:0,
                                          0));
  new((char*)&dg->xtc) Xtc(TypeId(TypeId::Id_Xtc,0), segInfo);
  return dg;
}

#include "NullProcessor.icc"
#include "Epix100aProcessor.icc"

//
//  pdsmonshm class methods
//
static void      pdsmonshm_dealloc(pdsmonshm* self);
static PyObject* pdsmonshm_new    (PyTypeObject* type, PyObject* args, PyObject* kwds);
static int       pdsmonshm_init   (pdsmonshm* self, PyObject* args, PyObject* kwds);
static PyObject* pdsmonshm_configure(PyObject* self, PyObject* args, PyObject* kwds);
static PyObject* pdsmonshm_event    (PyObject* self, PyObject* args);

static PyMethodDef pdsmonshm_methods[] = {
  {"configure" , (PyCFunction)pdsmonshm_configure , METH_VARARGS|METH_KEYWORDS, "Configure the run"},
  {"event"     , (PyCFunction)pdsmonshm_event     , METH_VARARGS, "Queue the event"},
  {NULL},
};

//
//  Register pdsmonshm members
//
static PyMemberDef pdsmonshm_members[] = {
  {NULL}
};

static PyTypeObject pdsmonshm_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pymonshm.Server",          /* tp_name */
    sizeof(pdsmonshm),          /* tp_basicsize */
    0,                          /* tp_itemsize */
    (destructor)pdsmonshm_dealloc, /*tp_dealloc*/
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
    "pymonshm Server objects",    /* tp_doc */
    0,                    /* tp_traverse */
    0,                    /* tp_clear */
    0,                    /* tp_richcompare */
    0,                    /* tp_weaklistoffset */
    0,                    /* tp_iter */
    0,                    /* tp_iternext */
    pdsmonshm_methods,             /* tp_methods */
    pdsmonshm_members,             /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    (initproc)pdsmonshm_init,      /* tp_init */
    0,                          /* tp_alloc */
    pdsmonshm_new,                 /* tp_new */
};

//
//  pdsmonshm class functions
//

void pdsmonshm_dealloc(pdsmonshm* self)
{
  if (self->srv)
    delete self->srv;

  if (self->processor)
    delete self->processor;

  Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject* pdsmonshm_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  pdsmonshm* self;

  self = (pdsmonshm*)type->tp_alloc(type,0);
  if (self != NULL) {
  }

  return (PyObject*)self;
}

int pdsmonshm_init(pdsmonshm* self, PyObject* args, PyObject* kwds)
{
  const char* kwlist[] = {"tag","nclients","buffersize","nbuffers",NULL};
  const char* tag;
  unsigned nclients     = 1;
  unsigned sizeofBuffers   = 0xA00000;
  unsigned numberofBuffers = 8;

  while(1) {
    if (PyArg_ParseTupleAndKeywords(args,kwds,"sI|II",const_cast<char**>(kwlist),
                                    &tag,&nclients,&sizeofBuffers,&numberofBuffers))
      break;
    return -1;
  }

  PyErr_Clear();

  printf("tag       : %s\n", tag);
  printf("clients   : %u\n",nclients);
  printf("buffersize: %u\n",sizeofBuffers);
  printf("nbuffers  : %u\n",numberofBuffers);

  self->srv      = new Pds::MyMonitorServer(tag,sizeofBuffers,numberofBuffers,nclients);

  return 0;
}

PyObject* pdsmonshm_configure(PyObject* self, PyObject* args, PyObject* kwds)
{
  const char* kwlist[] = {"rows","columns","bitdepth","offset",NULL};
  const char* dettype  = 0;
  unsigned    rows     = 0;
  unsigned    columns  = 0;
  unsigned    bitdepth = 14;
  unsigned    offset   = 0;

  pdsmonshm*   srv     = (pdsmonshm*)self;

  while(1) {
    if (PyArg_ParseTuple(args,"s",&dettype)) {
      if (strcasecmp(dettype,"Epix100a")==0) {
        srv->processor = new Epix100aProcessor;
        break;
      }
      return NULL;
    }

    if (PyArg_ParseTupleAndKeywords(args,kwds,"II|II",const_cast<char**>(kwlist),
                                    &rows,&columns,&bitdepth,&offset))
      srv->processor = new NullProcessor(rows,columns,bitdepth,offset);
      break;
    return NULL;
  }

  PyErr_Clear();

  Pds::MyMonitorServer& mon = *srv->srv;
  mon.events(insert(mon.newDatagram(), TransitionId::Map));
  mon.events(srv->processor->configure(mon.newDatagram()));
  mon.events(insert(mon.newDatagram(), TransitionId::BeginRun));
  mon.events(insert(mon.newDatagram(), TransitionId::BeginCalibCycle));
  mon.events(insert(mon.newDatagram(), TransitionId::Enable));

  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* pdsmonshm_event    (PyObject* self, PyObject* args)
{
  PyObject* o;
  if (PyArg_ParseTuple(args,"O",&o)==0)
    return NULL;

  Py_buffer b;
  if (PyObject_GetBuffer(o, &b, PyBUF_SIMPLE)<0)
    return NULL;

  pdsmonshm*   srv      = (pdsmonshm*)self;

  Pds::MyMonitorServer& mon = *srv->srv;
  mon.events(srv->processor->event(mon.newDatagram(),(const char*)b.buf));

  PyBuffer_Release(&b);

  Py_INCREF(Py_None);
  return Py_None;
}


//
//  Module methods
//
//

static PyMethodDef PymonshmMethods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

#ifdef IS_PY3K
static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "pymonshm",
        NULL,
        -1,
        PymonshmMethods,
        NULL,
        NULL,
        NULL,
        NULL
};
#endif

//
//  Module initialization
//
DECLARE_INIT(pymonshm)
{
  if (PyType_Ready(&pdsmonshm_type) < 0)
    INITERROR;

#ifdef IS_PY3K
  PyObject *m = PyModule_Create(&moduledef);
#else
  PyObject *m = Py_InitModule("pymonshm", PymonshmMethods);
#endif
  if (m == NULL)
    INITERROR;

  Py_INCREF(&pdsmonshm_type);
  PyModule_AddObject(m, "Server" , (PyObject*)&pdsmonshm_type);

#ifdef IS_PY3K
  return m;
#endif
}
