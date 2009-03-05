#!/pcds/package/python-2.5.2/bin/python

def pds_typeids():
    return ["Any",
            "Id_Xtc",
            "Id_Frame",
            "Id_Waveform",
            "Id_AcqConfig",
            "Id_TwoDGaussian",
            "Id_Opal1kConfig",
            "Id_FrameFexConfig",
            "Id_EvrConfig"]

def pds_type_index(type):
    typeids=pds_typeids()
    if typeids.__contains__(type):
        return "%08x" % typeids.index(type)
    return None
    
def pds_detectors():
    return ["NoDetector",
            "IMS",
            "PEM",
            "ETOF",
            "ITOF",
            "MBS",
            "IIS",
            "XES"]

def pds_devices():
    return ["NoDevice",
            "EVR",
            "Acqiris",
            "Opal1000"]

