from __future__ import annotations

import platform
import sys

import cv2
import torch
import ultralytics


def main() -> None:
    print("python:", sys.version.replace("\n", " "))
    print("executable:", sys.executable)
    print("platform:", platform.platform())
    print("torch:", torch.__version__)
    print("torch_cuda:", torch.version.cuda)
    print("cuda_available:", torch.cuda.is_available())
    if torch.cuda.is_available():
        print("gpu:", torch.cuda.get_device_name(0))
        total_gb = torch.cuda.get_device_properties(0).total_memory / (1024**3)
        print("gpu_memory_gb: {:.2f}".format(total_gb))
    print("opencv:", cv2.__version__)
    print("ultralytics:", ultralytics.__version__)

    if not torch.cuda.is_available():
        raise SystemExit("CUDA is unavailable. Do not start formal training on CPU.")


if __name__ == "__main__":
    main()

