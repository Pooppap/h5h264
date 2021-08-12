import h5py
import argparse
import numpy as np

from tempfile import TemporaryFile

parser = argparse.ArgumentParser()
parser.add_argument(
    "--n_frames",
    help="Number of frames",
    default=20,
    type=int
)
parser.add_argument(
    "--width",
    help="Frame width",
    default=640,
    type=int
)
parser.add_argument(
    "--height",
    help="Frame height",
    default=480,
    type=int
)


def gen_data(width, height, n_frames):
    data = np.zeros((n_frames, width, height), dtype=np.float32)
    for i in range(n_frames):
        for j in range(width):
            for k in range(height):
                data[i, j, k] = 3 * i + j + k

    return data


def encode(temp_file, data):
    with h5py.File(temp_file, "w") as _file:
        _file.create_dataset(
            "h264",
            data.shape,
            compression=32030,
            data=data,
            chunks=data.shape,
            compression_opts=(data.shape[1], data.shape[2], data.dtype.itemsize)
        )


def decode(temp_file):
    with h5py.File(temp_file, "r") as _file:
        data = _file["h264"][()]

    return data


def main(args):
    temp_file = TemporaryFile()

    data = gen_data(args.width,  args.height, args.n_frames)
    encode(temp_file, data)
    data_2 = decode(temp_file)

    print(f"Original Array is equal to Decoded Array: {np.array_equal(data, data_2)}")



if __name__ == '__main__':
    args = parser.parse_args()
    main(args)
