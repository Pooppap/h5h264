#ifndef H5PY_H264_H
#define H5PY_H264_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Filter revision number, starting at 1 */
#define H5PY_FILTER_H264_VERSION 2

/* Filter ID registered with the HDF Group as of 2/6/09.  For maintenance
requests, contact the filter author directly. */
#define H5Z_FILTER_H264 32030

/* Register the filter with the library. Returns a negative value on failure,
and a non-negative value on success.
*/
int register_h264(void);

#ifdef __cplusplus
}
#endif

#endif