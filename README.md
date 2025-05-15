# CAD-MLLM-metrics

The repo is part of code release of [CAD-MLLM](https://github.com/CAD-MLLM/CAD-MLLM).

It contains the utility code to compute the proposed new metrics **Segment Error (SegE), Dangling Edge Length (DangEL), Self Intersection Ratio (SIR), and Flux Enclosure Error (FluxEE)**.

## Build

```
sh build.sh
```

## Before you compute

1. Please keep the tested meshes in the same scale. (Dangling edges length will be affected by the scale)

2. Segment error (SegE) requires additional ground truth mesh for evaluation.

## Compute the metrics for each model

Place all your `.ply` files into one folder, like:

```
└── folder
    ├── mesh1.ply
    └── mesh2.ply
```

Then run:

```
sh eval.sh /path/to/your/folder
```

(Note: transform `.ply` into `.stl` is essential to load as a manifold.)

The results for each metric will be saved in separate folders. I suggest first using some toy cases for your testing.

## Result collect

Example:

Under the `toy_case` directory, ensure that the mesh file in the `recon` folder uses the same filename prefix as the corresponding ground-truth mesh in the `gt` folder (Used to compute the ground-truth mesh's segment number).

```
├── gt
│   └── mesh1.ply
└── recon
    └── mesh1.ply
```

Run computation:

```
sh eval.sh ./toy_case/recon
sh eval.sh ./toy_case/gt
python3 scripts/merge_results.py toy_case
```

There would be a `result.json` generated under `toy_case`.

## Acknowledgements

We appreciate the following projects for their awesome foundation code used in this repo: [CGAL](https://github.com/CGAL/cgal), [polyscope](https://github.com/nmwsharp/polyscope).

## Bibtex

Please kindly consider citing this paper if you find these metrics helpful:

```
@misc{xu2024CADMLLM,
      title={CAD-MLLM: Unifying Multimodality-Conditioned CAD Generation With MLLM}, 
      author={Jingwei Xu and Zibo Zhao and Chenyu Wang and Wen Liu and Yi Ma and Shenghua Gao},
      year={2024},
      eprint={2411.04954},
      archivePrefix={arXiv},
      primaryClass={cs.CV}
}
```
