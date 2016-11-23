#include"yagxtc.hh"

#include<stdlib.h>
#include<string.h>
#include<vector>

#include "cadef.h"
#include "hdf5.h"

#define IMAGE_CHUNK 1
#define PV_CHUNK 1024
#define TIME_DIM 2
#define MAX_DATA_DIM 3

#define CHECK(val, msg, ...) \
  { if (val < 0) { printf(msg, ##__VA_ARGS__); return false; } }

#define FILLDIM(arr, num, val) \
  { for (int i=0; i<num; i++) arr[i] = val; }

#define TIMEDIM(arr, x, y) \
  { arr[0] = x; arr[1] = y; }

#define DATADIM(arr, src, val) \
  { arr[0] = val; for (int i=0; i<MAX_DATA_DIM-1; i++) arr[i+1] = src[i]; }

using namespace std;

hid_t type_convert(long dbrtype)
{
    hid_t h5type;

    switch(dbrtype) {
    case DBR_TIME_STRING:
        h5type = H5Tcopy(H5T_C_S1);
        H5Tset_size(h5type, MAX_STRING_SIZE);
        break;
    case DBR_TIME_CHAR:
        h5type = H5Tcopy(H5T_NATIVE_UINT8);
        break;
    case DBR_TIME_ENUM:
    case DBR_TIME_INT:
        h5type = H5Tcopy(H5T_NATIVE_INT16);
        break;
    case DBR_TIME_LONG:
        h5type = H5Tcopy(H5T_NATIVE_INT32);
        break;
    case DBR_TIME_FLOAT:
        h5type = H5Tcopy(H5T_NATIVE_FLOAT);
        break;
    case DBR_TIME_DOUBLE:
        h5type = H5Tcopy(H5T_NATIVE_DOUBLE);
        break;
    default:
        return h5type = -1;
    }

    return h5type;
}

int get_pv_chunk(long dbrtype, int nelem)
{
    int chunk = PV_CHUNK;

    switch(dbrtype) {
    case DBR_TIME_STRING:
        chunk /= MAX_STRING_SIZE;
        break;
    case DBR_TIME_CHAR:
        chunk /= sizeof(char);
        break;
    case DBR_TIME_ENUM:
    case DBR_TIME_INT:
        chunk /= sizeof(int16_t);
        break;
    case DBR_TIME_LONG:
        chunk /= sizeof(int32_t);
        break;
    case DBR_TIME_FLOAT:
        chunk /= sizeof(float);
        break;
    case DBR_TIME_DOUBLE:
        chunk /= sizeof(double);
        break;    
    }

    return chunk;
}

class hdfsrc {
 public:
    hdfsrc(int _id, string _name, int _chunk, int _rank, hsize_t* _dims, const long& _dbrtype, hid_t& _parent)
        : id(_id), name(_name), chunk(_chunk), rank(_rank), parent(_parent), cnt(0), damagecnt(0), open(false), dbrtype(_dbrtype) {

        /* initialize offsets */
        FILLDIM(offsetsTime, TIME_DIM, 0);
        FILLDIM(offsets, MAX_DATA_DIM, 0);

        dataTimeType = H5Tcopy(H5T_NATIVE_INT32);

        /* initialize dimension arrays */
        TIMEDIM(elementTime, 1, 3);
        TIMEDIM(currentTimeDims, cnt, 3);
        TIMEDIM(maximumTimeDims, H5S_UNLIMITED, 3);
        TIMEDIM(chunkTimeDims, chunk, 3);

        DATADIM(element, _dims, 1);
        DATADIM(currentDims, _dims, cnt);
        DATADIM(maximumDims, _dims, H5S_UNLIMITED);
        DATADIM(chunkDims, _dims, chunk);
    };

    bool initialize() {
        dataType = type_convert(dbrtype);
        CHECK(dataType, "Data source is an unsupported epics dbr type: %ld\n", dbrtype);

        group = H5Gcreate(parent, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        CHECK(group, "Failed to create hdf5 group named %s\n", name.c_str());

        /* Create the timestamp dataset */
        hid_t fileTimeDataspaceId = H5Screate_simple(TIME_DIM,currentTimeDims,maximumTimeDims);

        hid_t plistIdTime = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_chunk(plistIdTime, TIME_DIM, chunkTimeDims);

        hid_t accessIdTime = H5Pcreate(H5P_DATASET_ACCESS);

        datasetIdTime = H5Dcreate(group,
                                  "time",
                                  dataTimeType,
                                  fileTimeDataspaceId,
                                  H5P_DEFAULT,
                                  plistIdTime,
                                  accessIdTime);

        H5Sclose(fileTimeDataspaceId);
        H5Pclose(plistIdTime);
        H5Pclose(accessIdTime);

        CHECK(datasetIdTime, "Failed to create hdf5 timestamp dataset for source %s\n", name.c_str());

        /* Create the 'data' dataset */
        hid_t fileDataspaceId = H5Screate_simple(rank, currentDims, maximumDims);
        
        hid_t plistId = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_chunk(plistId, rank, chunkDims);

        hid_t accessId = H5Pcreate(H5P_DATASET_ACCESS);

        datasetId = H5Dcreate(group,
                              "data",
                              dataType,
                              fileDataspaceId,
                              H5P_DEFAULT,
                              plistId,
                              accessId);

        H5Sclose(fileDataspaceId);
        H5Pclose(plistId);
        H5Pclose(accessId);

        CHECK(datasetId, "Failed to create hdf5 primary dataset for source %s\n", name.c_str());

        open = true;

        return open;
    };

    bool append(unsigned int secs, unsigned int nsecs, void *data) {
        if (!open) return false;

        hid_t status;

        uint32_t time_data[] = { secs, nsecs, nsecs & 0x1ffff };

        /* write the timestamps to its dataset */
        offsetsTime[0] = cnt;
        currentTimeDims[0] = cnt+1;
        H5Dset_extent(datasetIdTime, currentTimeDims);

        hid_t dspaceWriteIdTime = H5Dget_space(datasetIdTime);
        H5Sselect_hyperslab(dspaceWriteIdTime, H5S_SELECT_SET, offsetsTime, NULL, elementTime, NULL);

        hid_t memoryDataspaceIdTime = H5Screate_simple(TIME_DIM,elementTime,NULL);

        status = H5Dwrite(datasetIdTime, dataTimeType, memoryDataspaceIdTime, dspaceWriteIdTime, H5P_DEFAULT, time_data);

        H5Sclose(memoryDataspaceIdTime);
        H5Sclose(dspaceWriteIdTime);

        CHECK(status, "Failed to write timestamp data for %s for event %d %d\n", name.c_str(), secs, nsecs);

        /* write the actual data to its dataset */
        offsets[0] = cnt;
        currentDims[0] = cnt+1;
        H5Dset_extent(datasetId, currentDims);

        hid_t dspaceWriteId = H5Dget_space(datasetId);
        H5Sselect_hyperslab(dspaceWriteId, H5S_SELECT_SET, offsets, NULL, element, NULL);

        hid_t memoryDataspaceId = H5Screate_simple(rank,element,NULL);

        status = H5Dwrite(datasetId, dataType, memoryDataspaceId, dspaceWriteId, H5P_DEFAULT, data);

        H5Sclose(memoryDataspaceId);
        H5Sclose(dspaceWriteId);

        CHECK(status, "Failed to write data for %s for event %d %d\n", name.c_str(), secs, nsecs);

        cnt+=1;

        return true;
    }

    void close() {
        if (open) {
            open = false;
            H5Dclose(datasetId);
            H5Dclose(datasetIdTime);
            H5Tclose(dataType);
            H5Tclose(dataTimeType);
            H5Gclose(group);
        }
    };

    int     id;
    string  name;
    int     chunk;
    int     rank;
    hid_t&  parent;
    hid_t   group;
    int     cnt;
    int     damagecnt;
    bool    open;
    const long& dbrtype;
    hsize_t elementTime[TIME_DIM];
    hsize_t currentTimeDims[TIME_DIM];
    hsize_t maximumTimeDims[TIME_DIM];
    hsize_t chunkTimeDims[TIME_DIM];
    hsize_t offsetsTime[TIME_DIM];
    hsize_t element[MAX_DATA_DIM];
    hsize_t currentDims[MAX_DATA_DIM];
    hsize_t maximumDims[MAX_DATA_DIM];
    hsize_t chunkDims[MAX_DATA_DIM];
    hsize_t offsets[MAX_DATA_DIM];
    hid_t dataTimeType;
    hid_t dataType;
    hid_t datasetIdTime;
    hid_t datasetId;
};


static char* hname;                 // The current output file name.
static int numsrc_hdf = 0;          // Total number of sources.
static vector<hdfsrc *> src_hdf;    // List of HDF5 dataset objects
static hid_t fileId;
static hid_t groupIdCam;
static hid_t groupIdPv;


void initialize_hdf(char *outfile)
{
    if (!write_hdf) return;

    int i = strlen(outfile);
    if (i >= 3 && !strcmp(outfile + i - 3, ".h5")) {
        i -= 3;
        outfile[i] = 0;
    }

    hname = new char[i + 3];
    sprintf(hname, "%s.h5", outfile);

    fileId = H5Fcreate(hname,
                       /*H5F_ACC_TRUNC|H5F_ACC_SWMR_WRITE,*/
                       H5F_ACC_TRUNC,
                       H5P_DEFAULT,
                       H5P_DEFAULT);

    if (fileId < 0) {
        printf("Failed to open hdf5 file %s, aborting.\n", hname);
        fprintf(stderr, "error Faield to open hdf5 file: aborting.\n");
        exit(1);
    } else {
        groupIdCam = H5Gcreate(fileId, "/Cameras", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        groupIdPv = H5Gcreate(fileId, "/PVs", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    }
}

int register_hdf_image(string name, int width, int height, const long& dbrtype, unsigned int /*secs*/, unsigned int /*nsecs*/)
{
    if (!write_hdf) return -1;

    hsize_t dims[] = {(hsize_t) height, (hsize_t) width};

    src_hdf.push_back(new hdfsrc(numsrc_hdf, name, IMAGE_CHUNK, 3, dims, dbrtype, groupIdCam));

    return numsrc_hdf++;
}

int register_hdf_pv(string name, int nelem, const long& dbrtype, unsigned int /*secs*/, unsigned int /*nsecs*/)
{
    if (!write_hdf) return -1;

    hsize_t dims[] = {(hsize_t) nelem, 0};

    src_hdf.push_back(new hdfsrc(numsrc_hdf, name, get_pv_chunk(dbrtype, nelem), nelem>1?2:1, dims, dbrtype, groupIdPv));

    return numsrc_hdf++;
}

void configure_hdf(int id, unsigned int /*secs*/, unsigned int /*nsecs*/)
{
    if (!write_hdf) return;

    if (!src_hdf[id]->initialize()) {
        printf("Creating datasets to for %s in hdf5 failed, aborting.\n", src_hdf[id]->name.c_str());
        fprintf(stderr, "error Creating datasets in hdf5 failed: aborting.\n");
        exit(1);
    }
}

void data_hdf(int id, unsigned int secs, unsigned int nsecs, void *data)
{
    if (!write_hdf) return;

    if (!src_hdf[id]->append(secs, nsecs, data)) {
        printf("Writing data to for %s to hdf5 failed, aborting.\n", src_hdf[id]->name.c_str());
        fprintf(stderr, "error Writing data to hdf5 failed: aborting.\n");
        exit(1);
    }
}

void cleanup_hdf(void)
{
    if (!write_hdf) return;

    for (int i = 0; i < numsrc_hdf; i++) {
        src_hdf[i]->close();
    }

    H5Gclose(groupIdCam);
    H5Gclose(groupIdPv);
    H5Fclose(fileId);
}

#undef CHECK
#undef TIMEDIM
#undef DATADIM
#undef FILLDIM
#undef TIME_DIM
#undef DATA_DIM
#undef IMAGE_CHUNK
#undef PV_CHUNK
