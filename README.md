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

The results for each metric will be saved in newly created folders seperately. I personally suggest to first use some toy cases for your testing.

## Result collect

Example:

```
sh eval.sh ./toy_case/recon
sh eval.sh ./toy_case/gt
python3 scripts/merge_results.py toy_case
```

## Bibtex

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
