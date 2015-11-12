//
//  pycdb module
//
//  Defines two python classes: pdsdb and pdsxtc
//

#include <Python.h>
#include <structmember.h>

#define MyArg_ParseTupleAndKeywords(args,kwds,fmt,kwlist,...) PyArg_ParseTupleAndKeywords(args,kwds,fmt,const_cast<char**>(kwlist),__VA_ARGS__)

#include "pdsapp/python/pycdb.hh"

#include "pdsapp/python/Xtc.icc"
#include "pdsapp/python/DiodeFexConfig.icc"
#include "pdsapp/python/CspadConfig.icc"
#include "pdsapp/python/Cspad2x2Config.icc"
#include "pdsapp/python/EpixConfig.icc"
#include "pdsapp/python/Epix10kConfig.icc"
#include "pdsapp/python/Epix100aConfig.icc"
#include "pdsapp/python/IpmFexConfig.icc"
#include "pdsapp/python/IpimbConfig.icc"
#include "pdsapp/python/PrincetonConfig.icc"
#include "pdsapp/python/EvrConfig.icc"
#include "pdsapp/python/FliConfig.icc"
#include "pdsapp/python/AndorConfig.icc"
#include "pdsapp/python/PimaxConfig.icc"
#include "pdsapp/python/RayonixConfig.icc"
#include "pdsapp/python/AcqirisConfig.icc"
#include "pdsapp/python/Db.icc"
#include "pdsapp/python/pycdbHelp.icc"


//
//  Module methods
//
//

static PyMethodDef PycdbMethods[] = {
    {"help"  , (PyCFunction)pds_pycdb_help  , METH_VARARGS, "Help Function"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

//
//  Module initialization
//
PyMODINIT_FUNC
initpycdb(void) 
{
  if (PyType_Ready(&pdsdb_type) < 0)
    return; 

  if (PyType_Ready(&pds_Xtc_type) < 0)
    return; 

  if (PyType_Ready(&pds_DiodeFexConfig_type) < 0)
    return; 

  if (PyType_Ready(&pds_CspadConfig_type) < 0)
    return; 
  
  if (PyType_Ready(&pds_Cspad2x2Config_type) < 0)
    return;

  if (PyType_Ready(&pds_EpixConfig_type) < 0)
    return;

  if (PyType_Ready(&pds_Epix10kConfig_type) < 0)
    return;

  if (PyType_Ready(&pds_Epix100aConfig_type) < 0)
    return;

 if (PyType_Ready(&pds_IpmFexConfig_type) < 0)
    return; 

  if (PyType_Ready(&pds_IpimbConfig_type) < 0)
    return; 

  if (PyType_Ready(&pds_PrincetonConfig_type) < 0)
    return; 

  if (PyType_Ready(&pds_EvrConfig_type) < 0)
    return; 

  if (PyType_Ready(&pds_FliConfig_type) < 0)
    return; 

  if (PyType_Ready(&pds_AndorConfig_type) < 0)
    return; 
    
  if (PyType_Ready(&pds_PimaxConfig_type) < 0)
    return; 

  if (PyType_Ready(&pds_RayonixConfig_type) < 0)
    return; 

  if (PyType_Ready(&pds_AcqirisConfig_type) < 0)
    return;

  PyObject *m = Py_InitModule("pycdb", PycdbMethods);
  if (m == NULL)
    return;

  Py_INCREF(&pdsdb_type);
  PyModule_AddObject(m, "Db" , (PyObject*)&pdsdb_type);

  Py_INCREF(&pds_Xtc_type);
  PyModule_AddObject(m, "Xtc", (PyObject*)&pds_Xtc_type);

  Py_INCREF(&pds_DiodeFexConfig_type);
  PyModule_AddObject(m, "DiodeFexConfig", (PyObject*)&pds_DiodeFexConfig_type);

  Py_INCREF(&pds_CspadConfig_type);
  PyModule_AddObject(m, "CspadConfig", (PyObject*)&pds_CspadConfig_type);
  
  Py_INCREF(&pds_Cspad2x2Config_type);
  PyModule_AddObject(m, "Cspad2x2Config", (PyObject*)&pds_Cspad2x2Config_type);

  Py_INCREF(&pds_EpixConfig_type);
  PyModule_AddObject(m, "EpixConfig", (PyObject*)&pds_EpixConfig_type);

  Py_INCREF(&pds_Epix10kConfig_type);
  PyModule_AddObject(m, "Epix10kConfig", (PyObject*)&pds_Epix10kConfig_type);

  Py_INCREF(&pds_Epix100aConfig_type);
  PyModule_AddObject(m, "Epix100aConfig", (PyObject*)&pds_Epix100aConfig_type);

  Py_INCREF(&pds_IpmFexConfig_type);
  PyModule_AddObject(m, "IpmFexConfig", (PyObject*)&pds_IpmFexConfig_type);

  Py_INCREF(&pds_IpimbConfig_type);
  PyModule_AddObject(m, "IpimbConfig", (PyObject*)&pds_IpimbConfig_type);  

  Py_INCREF(&pds_PrincetonConfig_type);
  PyModule_AddObject(m, "PrincetonConfig", (PyObject*)&pds_PrincetonConfig_type); 
  
  Py_INCREF(&pds_EvrConfig_type);
  PyModule_AddObject(m, "EvrConfig", (PyObject*)&pds_EvrConfig_type); 
  
  Py_INCREF(&pds_FliConfig_type);
  PyModule_AddObject(m, "FliConfig", (PyObject*)&pds_FliConfig_type);     
  
  Py_INCREF(&pds_AndorConfig_type);
  PyModule_AddObject(m, "AndorConfig", (PyObject*)&pds_AndorConfig_type);       

  Py_INCREF(&pds_PimaxConfig_type);
  PyModule_AddObject(m, "PimaxConfig", (PyObject*)&pds_PimaxConfig_type);       

  Py_INCREF(&pds_RayonixConfig_type);
  PyModule_AddObject(m, "RayonixConfig", (PyObject*)&pds_RayonixConfig_type);

  Py_INCREF(&pds_AcqirisConfig_type);
  PyModule_AddObject(m, "AcqirisConfig", (PyObject*)&pds_AcqirisConfig_type);
}
