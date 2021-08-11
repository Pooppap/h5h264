#include <stdlib.h>
// #include <H5PLextern.h>

#include "hdf5.h"
#include "decode.c"
#include "encode.c"
#include "h264_filter.h"

#if H5_VERS_MAJOR == 1 && H5_VERS_MINOR < 7

#define PUSH_ERR(func, minor, str)  H5Epush(__FILE__, func, __LINE__, H5E_PLINE, minor, str)
#define H5PY_GET_FILTER H5Pget_filter_by_id

#else

#define PUSH_ERR(func, minor, str)  H5Epush1(__FILE__, func, __LINE__, H5E_PLINE, minor, str)
#define H5PY_GET_FILTER(a,b,c,d,e,f,g) H5Pget_filter_by_id2(a,b,c,d,e,f,g,NULL)

#endif

#if H5_VERS_MAJOR == 1 && H5_VERS_MINOR == 6
#define H5PY_H5Z_NEWCLS 0
#elif H5_VERS_MAJOR == 1 && H5_VERS_MINOR == 8 && H5_VERS_RELEASE < 3
#define H5PY_H5Z_NEWCLS 1
#elif H5_USE_16_API
#define H5PY_H5Z_NEWCLS 0
#else /* Default: use new class */
#define H5PY_H5Z_NEWCLS 1
#endif

size_t h264_filter(unsigned flags, size_t cd_nelmts, const unsigned cd_values[], size_t nbytes, size_t *buf_size, void **buf);

#if H5PY_H5Z_NEWCLS
static const H5Z_class_t filter_class = {
    H5Z_CLASS_T_VERS,
    (H5Z_filter_t)(H5Z_FILTER_H264),
    1, 1,
    "h264",
    NULL,
    NULL,
    (H5Z_func_t)(h264_filter)
};
#else
static const H5Z_class_t filter_class = {
    (H5Z_filter_t)(H5Z_FILTER_H264),
    "h264",
    NULL,
    NULL,
    (H5Z_func_t)(h264_filter)
};
#endif

#if defined(H5_VERSION_GE)
#if H5_VERSION_GE(1, 8, 11)

#include "H5PLextern.h"

H5PL_type_t H5PLget_plugin_type(void){ return H5PL_TYPE_FILTER; }

const void *H5PLget_plugin_info(void){ return &filter_class; }

#endif
#endif

/* Try to register the filter, passing on the HDF5 return value */
int register_h264(void){

    int retval;

    retval = H5Zregister(&filter_class);
    if(retval<0){
        PUSH_ERR("register_h264", H5E_CANTREGISTER, "Can't register H264 filter");
    }
    return retval;
}

/* The filter function */
size_t h264_filter(unsigned flags, size_t cd_nelmts, const unsigned cd_values[], size_t nbytes, size_t *buf_size, void **buf)
{
  char *output_buffer = NULL;

  /* We're compressing */
  if(!(flags & H5Z_FLAG_REVERSE))
  {
    /* Compress data */
    if(cd_nelmts != 3)
    {
      /* We're missing the cd values */
      fprintf(stderr, "cd_values are missing\n");
      return 0;
    }
    int width = cd_values[0];
    int height = cd_values[1];
    int item_size = cd_values[2];
    if(nbytes % (height*width*item_size))
    {
      /* The cd values must be wrong */
      fprintf(stderr, "cd_values do not match input size\n");
      fprintf(stderr, "height - %d width - %d item_size - %d, nbytes - %zu\n", height, width, item_size, nbytes);
      return 0;
    }
    output_buffer = h264_encode(*buf, nbytes, height, width,
        item_size, buf_size);
    #ifdef H5PY_H264_DEBUG
        fprintf(stdout, "[debug] h5h264: encoded %zu bytes into %zu bytes for a ratio of %f\n", nbytes, *buf_size, ((float)*buf_size)/nbytes);
    #endif

  /* We're decompressing */
  }
  else
  {
    output_buffer = h264_decode(*buf, nbytes, buf_size);
    #ifdef H5PY_H264_DEBUG
      fprintf(stdout,"[debug] h5h264: decompressed %zu bytes\n",*buf_size);
    #endif
  } /* compressing vs decompressing */

  if(!output_buffer)
  {
    free(output_buffer);
    return 0;
  }

  free(*buf);
  *buf = output_buffer;
  return *buf_size;
} /* End filter function */
