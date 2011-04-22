#ifndef PdsPython_pycdb_hh
#define PdsPython_pycdb_hh

#include "pdsdata/xtc/Xtc.hh"

namespace Pds_ConfigDb {
  class Path;
  class Experiment; 
};

typedef struct {
  PyObject_HEAD
  Pds_ConfigDb::Path*       path;
  Pds_ConfigDb::Experiment* expt;
} pdsdb;

typedef struct {
  PyObject_HEAD
  Pds::Src    src;
  Pds::TypeId contains;
  unsigned    extent;
  char*       payload;
} pdsxtc;

#define DefineXtcType(name)                                             \
  static PyTypeObject pds_ ## name ## _type = {                         \
    PyObject_HEAD_INIT(NULL)                                            \
    0,                          /* ob_size */                           \
    "pycdb." #name,             /* tp_name */                           \
    sizeof(pdsxtc),             /* tp_basicsize */                      \
    0,                          /* tp_itemsize */                       \
    (destructor)pdsxtc_dealloc, /*tp_dealloc*/                          \
    0,                          /*tp_print*/                            \
    0,                          /*tp_getattr*/                          \
    0,                          /*tp_setattr*/                          \
    0,                          /*tp_compare*/                          \
    0,                          /*tp_repr*/                             \
    0,                          /*tp_as_number*/                        \
    0,                          /*tp_as_sequence*/                      \
    0,                          /*tp_as_mapping*/                       \
    0,                          /*tp_hash */                            \
    0,                          /*tp_call*/                             \
    0,                          /*tp_str*/                              \
    0,                          /*tp_getattro*/                         \
    0,                          /*tp_setattro*/                         \
    0,                          /*tp_as_buffer*/                        \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/       \
    "pycdb " #name " objects",  /* tp_doc */                            \
    0,		                /* tp_traverse */                       \
    0,		                /* tp_clear */                          \
    0,		                /* tp_richcompare */                    \
    0,		                /* tp_weaklistoffset */                 \
    0,		                /* tp_iter */                           \
    0,		                /* tp_iternext */                       \
    pds_ ## name ## _methods,   /* tp_methods */                        \
    pdsxtc_members,             /* tp_members */                        \
    0,                          /* tp_getset */                         \
    0,                          /* tp_base */                           \
    0,                          /* tp_dict */                           \
    0,                          /* tp_descr_get */                      \
    0,                          /* tp_descr_set */                      \
    0,                          /* tp_dictoffset */                     \
    (initproc)pdsxtc_init,      /* tp_init */                           \
    0,                          /* tp_alloc */                          \
    pdsxtc_new,                 /* tp_new */                            \
  };

#endif
