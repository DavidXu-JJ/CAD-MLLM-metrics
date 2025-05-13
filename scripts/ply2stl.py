import os
import trimesh
import argparse
from tqdm.contrib.concurrent import process_map

def convert_ply_to_stl(ply_file, folder_path):
    ply_file_path = os.path.join(folder_path, ply_file)
    mesh = trimesh.load(ply_file_path)
    stl_file_path = os.path.join(folder_path, ply_file.replace(".ply", ".stl"))
    mesh.export(stl_file_path)
    return ply_file  # Return the processed file name for tracking

def convert_all_ply_files(folder_path):
    files = os.listdir(folder_path)
    ply_files = [f for f in files if f.endswith(".ply")]

    # Use process_map to handle the conversion with multiple processes and show progress bar
    process_map(convert_ply_to_stl, ply_files, [folder_path] * len(ply_files), max_workers=16, chunksize=16)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert all .ply files in a folder to .stl format."
    )
    parser.add_argument(
        "path", type=str, help="The path to the folder containing .ply files."
    )

    args = parser.parse_args()

    convert_all_ply_files(args.path)
