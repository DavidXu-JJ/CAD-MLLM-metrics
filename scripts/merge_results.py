import os
import json
import argparse
import trimesh
import numpy as np
from tqdm import tqdm
from multiprocessing import Pool

NUM_THREADS = 12

def collect_segment_num_error(paths):
    seg_path, gt_seg_path = paths
    if seg_path.split('/')[-1].split('.')[0] != gt_seg_path.split('/')[-1].split('.')[0]:
        return None
    if not os.path.exists(gt_seg_path):
        return None
    seg = None
    with open(seg_path, 'r', encoding='utf-8') as file:
        seg = file.readline()

    gt_seg = None
    with open(gt_seg_path, 'r', encoding='utf-8') as file:
        gt_seg = file.readline()

    return abs(int(seg) - int(gt_seg))

def normalize_mesh(mesh):
    points = mesh.vertices

    min_point = np.min(points, axis=0)
    max_point = np.max(points, axis=0)

    mean = (min_point + max_point) / 2.0
    points = points - mean

    scale = np.max(max_point - min_point) / 2.0
    points = points / scale

    mesh.vertices = points
    return mesh

def collect_danling_edge_length(paths):
    dangling_edge_path = paths

    dangling_edge_length = None
    with open(dangling_edge_path, 'r', encoding='utf-8') as file:
        dangling_edge_length = float(file.readline())

    return dangling_edge_length 


def collect_self_intersection_percentage(path):
    try:
        self_intersection_tri_num = None
        all_tri_num = None
        with open(path, 'r', encoding='utf-8') as file:
            self_intersection_tri_num = int(file.readline())
            all_tri_num = int(file.readline())

        return 1.0 * self_intersection_tri_num / all_tri_num
    except:
        return None

def collect_flux_enclosure_error(path):
    flux_enclosure_error = None
    with open(path, 'r', encoding='utf-8') as file:
        flux_enclosure_error = float(file.readline())

    return flux_enclosure_error


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert all .ply files in a folder to .stl format."
    )
    parser.add_argument(
        "path", type=str, help="The path to the evaluation folder."
    )

    args = parser.parse_args()
    segment_num_path = os.path.join(args.path, 'recon_segment_num')
    dangling_edge_path = os.path.join(args.path, 'recon_dangling_edge')
    self_intersection_path = os.path.join(args.path, 'recon_self_intersection')
    flux_enclosure_error_path = os.path.join(args.path, 'recon_flux_enclosure_error')

    gt_segment_num_path = os.path.join(args.path, 'gt_segment_num')

    # collect segment num error
    segment_num_suffix = os.listdir(segment_num_path)
    segment_num_paths = []
    gt_segment_num_paths = []
    for suffix in segment_num_suffix:
        segment_num_paths.append(os.path.join(segment_num_path, suffix))
        gt_segment_num_paths.append(os.path.join(gt_segment_num_path, suffix))

    paths = zip(segment_num_paths, gt_segment_num_paths)
    seg_num_error_list = []
    load_iter = Pool(NUM_THREADS).imap(collect_segment_num_error, paths)
    for seg_num_error in tqdm(load_iter, total=len(segment_num_paths)):
        if seg_num_error is None:
            continue
        seg_num_error_list.append(seg_num_error)
    print(len(seg_num_error_list))

    # collect dangling edge length
    dangling_edge_suffix = os.listdir(dangling_edge_path)
    dangling_edge_paths = []
    for suffix in dangling_edge_suffix:
        dangling_edge_paths.append(os.path.join(dangling_edge_path, suffix))
    dangling_edge_length_list = []
    load_iter = Pool(NUM_THREADS).imap(collect_danling_edge_length, dangling_edge_paths)
    for dangling_edge_length in tqdm(load_iter, total=len(dangling_edge_paths)):
        if dangling_edge_length is None:
            continue
        dangling_edge_length_list.append(dangling_edge_length)
    print(len(dangling_edge_length_list))

    # collect self intersection percentage
    self_intersection_suffix = os.listdir(self_intersection_path)
    self_intersection_paths = []
    for suffix in self_intersection_suffix:
        self_intersection_paths.append(os.path.join(self_intersection_path, suffix))
    self_intersection_percentage_list = []
    load_iter = Pool(NUM_THREADS).imap(collect_self_intersection_percentage, self_intersection_paths)
    for self_intersection_percentage in tqdm(load_iter, total=len(self_intersection_paths)):
        if self_intersection_percentage is None:
            continue
        self_intersection_percentage_list.append(self_intersection_percentage)
    print(len(self_intersection_percentage_list))


    # collect flux enclosure error
    flux_suffix = os.listdir(flux_enclosure_error_path)
    flux_enclosure_error_paths = []
    for suffix in flux_suffix:
        flux_enclosure_error_paths.append(os.path.join(flux_enclosure_error_path, suffix))
    flux_enclosure_error_list = []
    load_iter = Pool(NUM_THREADS).imap(collect_flux_enclosure_error, flux_enclosure_error_paths)
    for flux_enclosure_error in tqdm(load_iter, total=len(flux_enclosure_error_paths)):
        if flux_enclosure_error is None:
            continue
        flux_enclosure_error_list.append(flux_enclosure_error)
    print(len(flux_enclosure_error_list))

    results = {}
    results['segment_num_error'] = np.array(seg_num_error_list).mean()
    dangling_edge_length_list = np.array(dangling_edge_length_list)
    valid_mask = ~np.isnan(dangling_edge_length_list)
    results['dangling_edge_length'] = dangling_edge_length_list[valid_mask].mean()
    results['self_intersection_percentage'] = np.array(self_intersection_percentage_list).mean()
    flux_enclosure_error_list = np.array(flux_enclosure_error_list)
    valid_mask = ~np.isnan(flux_enclosure_error_list)
    results['flux_enclosure_error'] = flux_enclosure_error_list[valid_mask].mean()

    with open(os.path.join(args.path, 'results.json'), 'w') as json_file:
        json.dump(results, json_file, indent=4)
