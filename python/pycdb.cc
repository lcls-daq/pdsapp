//
//  pycdb module
//
//  Defines two python classes: pdsdb and pdsxtc
//

#include <Python.h>
#include <structmember.h>

#include "pdsapp/config/Path.hh"

#include "pdsapp/config/Experiment.hh"

#include "pdsapp/python/pycdb.hh"

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#include <glob.h>

using std::vector;
using std::string;
using std::ostringstream;
using std::hex;
using std::setw;

using namespace Pds_ConfigDb;

//
//  pdsdb class methods
//
static void      pdsdb_dealloc(pdsdb* self);
static PyObject* pdsdb_new    (PyTypeObject* type, PyObject* args, PyObject* kwds);
static int       pdsdb_init   (pdsdb* self, PyObject* args, PyObject* kwds);
static PyObject* pdsdb_get    (PyObject* self_o, PyObject* args, PyObject* kwds);

static PyMethodDef pdsdb_methods[] = {
  {"get"   , (PyCFunction)pdsdb_get   , METH_KEYWORDS, "Return the xtc data"},
  {NULL},
};

//
//  Register pdsdb members
//
static PyMemberDef pdsdb_members[] = {
  {NULL} 
};

static PyTypeObject pdsdb_type = {
  PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "pycdb.Db",                /* tp_name */
    sizeof(pdsdb),             /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)pdsdb_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "pycdb Db objects",        /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    pdsdb_methods,             /* tp_methods */
    pdsdb_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)pdsdb_init,      /* tp_init */
    0,                         /* tp_alloc */
    pdsdb_new,                 /* tp_new */
};
    
#include "pdsapp/python/Xtc.icc"
#include "pdsapp/python/DiodeFexConfig.icc"

//
//  Module methods
//
//

static PyMethodDef PycdbMethods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

//
//  Module initialization
//
PyMODINIT_FUNC
initpycdb(void)
{
  if (PyType_Ready(&pdsdb_type) < 0) {
    return; 
  }

  if (PyType_Ready(&pds_Xtc_type) < 0) {
    return; 
  }

  if (PyType_Ready(&pds_DiodeFexConfig_type) < 0) {
    return; 
  }

  PyObject *m = Py_InitModule("pycdb", PycdbMethods);
  if (m == NULL)
    return;

  Py_INCREF(&pdsdb_type);
  PyModule_AddObject(m, "Db" , (PyObject*)&pdsdb_type);

  Py_INCREF(&pds_Xtc_type);
  PyModule_AddObject(m, "Xtc", (PyObject*)&pds_Xtc_type);

  Py_INCREF(&pds_DiodeFexConfig_type);
  PyModule_AddObject(m, "DiodeFexConfig", (PyObject*)&pds_DiodeFexConfig_type);
}


//
//  pdsdb class functions
//

void pdsdb_dealloc(pdsdb* self)
{
  if (self->expt) {
    delete self->expt;
    delete self->path;
  }

  self->ob_type->tp_free((PyObject*)self);
}

PyObject* pdsdb_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  pdsdb* self;

  self = (pdsdb*)type->tp_alloc(type,0);
  if (self != NULL) {
    self->expt = 0;
    self->path = 0;
  }

  return (PyObject*)self;
}

int pdsdb_init(pdsdb* self, PyObject* args, PyObject* kwds)
{
  int sts;

  { char* kwlist[] = {"path",NULL};
    const char *path = 0;
    sts = PyArg_ParseTupleAndKeywords(args,kwds,"s",kwlist,
                                      &path);
    if (sts) {
      self->path = new Path(path);
      self->expt = new Experiment(*self->path);
      self->expt->read();
      return 0;
    }
  }
  return -1;
}

//
//  Return a configuration object of type <type> from <src> associated with <key>
//
PyObject* pdsdb_get(PyObject* self_o, PyObject* args, PyObject* kwds)
{
  PyObject* result = PyTuple_New(0);

  pdsdb* self = reinterpret_cast<pdsdb*>(self_o);

  char* kwlist[] = {"key","phy","level","typeid",NULL};
  const unsigned Default(0xffffffff);
  unsigned key   (Default);
  unsigned phy   (Default);
  unsigned level (Default);
  unsigned typid (Default);
  int sts = PyArg_ParseTupleAndKeywords(args,kwds,"I|III",kwlist,
                                        &key,&level,&phy,&typid);

  if (sts==0) {
    char* kwlist2[] = {"alias","phy","level","typid",NULL};
    const char* alias=0;
    sts = PyArg_ParseTupleAndKeywords(args,kwds,"s|III",kwlist2,
                                      &alias,&level,&phy,&typid);

    if (sts==0) {
      return result;
    }

    const TableEntry* entry = self->expt->table().get_top_entry(alias);
    if (entry)
      key = strtoul(entry->key().c_str(),NULL,16);

    printf("key %s = 0x%x\n",alias,key);
  }

  if (key==Default) {
    return result;
  }

  char kname[256];
  sprintf(kname,"%08x/[0-9]*",key);

  //  Match source
  vector<string> devices;
  string kpath = self->path->key_path(kname);
  glob_t g;
  glob(kpath.c_str(),0,0,&g);

  for(unsigned k=0; k<g.gl_pathc; k++) {
    const char* src = basename(g.gl_pathv[k]);
    unsigned    dev = strtoul(src,NULL,16);
    if ((level==Pds::Level::Source && phy==dev) ||
        (level==dev) ||
        (level==Default && phy==Default)) {

      Pds::Src info = Pds::Src(Pds::Level::Type(level));
      if (level==Pds::Level::Source)
        *new((void*)&info) 
          Pds::DetInfo(0, 
                       Pds::DetInfo::Detector((dev>>24)&0xff), (dev>>16)&0xff,
                       Pds::DetInfo::Device  ((dev>> 8)&0xff), (dev>> 0)&0xff);
        
      string kpath = string(g.gl_pathv[k]) + "/*";
      glob_t gk;
      glob(kpath.c_str(),0,0,&gk);
      for(unsigned m=0; m<gk.gl_pathc; m++) {
        unsigned typ = strtoul(basename(gk.gl_pathv[m]),NULL,16);
        //  Match type
        if (typid==Default || typid==typ) {
          int i = PyTuple_Size(result);
          _PyTuple_Resize(&result, i+1);

          PyObject* o;
          Pds::TypeId contains(Pds::TypeId::Type(typ&0xffff),
                               typ>>16);
          switch(contains.id()) {
          case Pds::TypeId::Id_DiodeFexConfig:
            o = _PyObject_New(&pds_DiodeFexConfig_type);
            break;
          default:
            o = _PyObject_New(&pds_Xtc_type);
            break;
          }
          pdsxtc_read(o, info, contains, gk.gl_pathv[m]);
          PyTuple_SetItem(result,i,o);
        }
      }
      globfree(&gk);
    }
  }
  globfree(&g);

  return result;

//   Py_INCREF(Py_None);
//   return Py_None;
}

