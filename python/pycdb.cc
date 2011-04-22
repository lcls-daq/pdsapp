//
//  pycdb module
//
//  Defines two python classes: pdsdb and pdsxtc
//

#include <Python.h>
#include <structmember.h>

#include "pdsapp/python/pycdb.hh"

#include "pdsapp/python/Xtc.icc"
#include "pdsapp/python/DiodeFexConfig.icc"
#include "pdsapp/python/Db.icc"

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
