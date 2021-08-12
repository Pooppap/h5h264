Compression filter for HDF5 files using H.264 compression.
It uses H.264 lossless compression through [ffmpeg](http://ffmpeg.org).

# Dependencies
H264 compression filter depends on two libraries:
1. FFmpeg
2. libhdf5-dev

# Installation
1. Install dependencies from the system package manager e.g.
```bash
sudo apt-get install ffmpeg libhdf5-dev
```
2. Clone the repository and build using `cmake`, e.g.:

```bash
git clone https://github.com/Pooppap/h5h264.git
cd h5h264
mkdir build
cd build
cmake ..
make
```

3. Use `sudo make install` to install this compression library to HDF5-predefined location or set `HDF5_PLUGIN_PATH` to the `build/lib`, as illustrated below. Both options will allow HDF5 to dynamically load the H264 filter.
```bash
export HDF5_PLUGIN_PATH=${HOME}/h5h264/build/lib
```

# Usage
The filter identification number is `32030`. This value must be passed as the
`filter` argument to `H5Pset_filter`.

This filters requires 3 user-defined compression-options,`compression_opts`. They are, in order of `compression_opts` arguments:
1. `width`: Frame width
2. `height`: Frame Height
3. `item_size`: the size of one element in the array in bytes, e.g. one element in an array of float32 = 4 bytes. Currently only support 1, 2, or 4 byte

Thus, to create `h5py` dataset with H264 compression, the command `create_dataset` can be invoked as in the following example:
```python
...
dset = f.create_dataset(
    "data",
    (n_frames, width, height),
    compression=32030,
    data=data,
    chunks=(5, width, height),
    compression_opts=(width, height, data.dtype.itemsize)
)
...
```
The `chunks` argument is completely up to the user, but `chunks=(n_frames, width, height)` is preferable to maximise the encoding efficiency.

The tests directories contain more examples in both C and Python

Reading a compressed dataset requires no extra code;

# Troubleshooting
- Compression does not exist: make sure that HDF5 version is >= 1.8.11
