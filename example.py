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
