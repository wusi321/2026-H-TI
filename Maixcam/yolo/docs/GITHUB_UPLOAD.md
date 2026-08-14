# GitHub 上传说明

## 推荐上传内容

当前目录已经按 GitHub 仓库整理完成，可以直接把 `gangzhu-yolo11n-pose-training` 作为仓库根目录上传。

建议保留：

- `datasets/gangzhu_pose_v2`
- `datasets/positive_source_voc`
- `datasets/source_voc_review_20260730`
- `datasets/negative_samples_185508`
- `scripts`
- `maixcam_conversion`
- `models/yolo11n-pose.pt`
- `results/weights/best.pt`
- `results/weights/best.onnx`
- `docs`
- `README.md`

## 大文件提醒

当前整理后目录约 101 MB。单个最大文件是：

- `results/weights/best.onnx`，约 10.3 MB
- `models/yolo11n-pose.pt`，约 6.0 MB
- `results/weights/best.pt`，约 5.3 MB
- `maixcam_conversion/deploy/gangzhu_yolo11n_pose_320_int8.cvimodel`，约 2.8 MB

这些文件都低于 GitHub 单文件 100 MB 限制，可以直接上传。若后续加入更大的 `.pt`、`.onnx`、视频或压缩包，建议使用 Git LFS，或放到 Release 附件中。

## 初始化仓库

```powershell
cd <repo-root>
git init
git add .
git commit -m "Add steel ball YOLO11n pose training project"
```

然后在 GitHub 创建空仓库，按页面提示添加远程地址并 push：

```powershell
git remote add origin https://github.com/<your-name>/<repo-name>.git
git branch -M main
git push -u origin main
```

## 上传前检查

```powershell
git status --short
git ls-files | Select-String ".venv|__pycache__|Thumbs.db"
```

第二条命令正常情况下不应输出内容。
