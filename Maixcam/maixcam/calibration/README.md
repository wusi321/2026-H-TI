# One-dimensional rod position calibration

The bundled `position_calibration.json` is calibrated from the physical
`-10/-5/0/+5/+10 cm` marks in the final 640x360 RGB installation. The uncropped
phone photo showed the LCD image rotated clockwise by 90 degrees. The active
LCD quadrilateral was mapped back to 640x360, while the optical perspective
inside the camera image was deliberately preserved.

## 2026-07-31 measured calibration

The fixed ROI remains `[0,108,640,92]`. The fitted rod axis is
`(0,175)..(639,175)`. Measured positions along that axis are:

| Physical position | Axis position `s_px` | Local scale to next point |
| ---: | ---: | ---: |
| -10 cm | 66 | 25.0 px/cm |
| -5 cm | 191 | 29.4 px/cm |
| 0 cm | 338 | 29.8 px/cm |
| +5 cm | 487 | 23.8 px/cm |
| +10 cm | 606 | - |

The cropped and uncropped photos were registered with 170 inlier image
features. The five red-line centers in the cropped photo were approximately
`240.5, 655.0, 1144.5, 1643.0, 2046.0 px`; mapping the LCD plane back to the
camera frame produced the values above with about `+/-2..3 px` uncertainty.
The unequal segment scales are camera perspective effects. Keep the piecewise
mapping; do not replace these points with one pixels-per-centimeter constant.
No RGB ball photos were available, so detector thresholds and ball-size limits
were not changed.

## Capture

1. Fix the final camera mount, PPR beam and diffuse light.
2. Keep the beam at its normal level position.
3. Put the ball at `-10, -5, 0, +5, +10 cm` using the physical scale. Add
   farther points only when the complete ball center remains in the image.
4. At each position, read several `px:(cx,cy)` values from the terminal and use
   their median.
5. Create a CSV with `x_cm,cx_px,cy_px`.

The example pixel values are not real calibration data. Do not recreate
unmeasured `+/-12 cm` anchors by uniformly dividing the image.

## Build

From a PC terminal:

```powershell
cd D:\桌面\2026ti\MaixcamPro
python host\build_position_calibration.py `
  host\calibration_samples.csv `
  maixcam\calibration\position_calibration.json `
  --roi 0 108 640 92 `
  --axis-start-px 0 175 `
  --axis-end-px 639 175
```

The ROI must cover the complete groove at every small operating angle. The
current beam needs a wide horizontal ROI. The fixed-axis arguments preserve
the observed pipe endpoints instead of extrapolating the outer segments as if
the lens scale were uniform. Omit them only for a new multi-point capture where
the fitted physical endpoints are both visible and verified.

The rod coordinate is `-12.5 cm` at the left endpoint, `0 cm` at the physical
center and `+12.5 cm` at the right endpoint. Piecewise calibration corrects
small perspective and scale errors better than converting from one global
pixels-per-centimeter value.

## Validate

Before enabling valid UART measurements, collect at least 100 frames at each
position and verify:

- mean absolute error at most `0.2 cm`;
- worst error at most `0.5 cm`;
- the coordinate increases in the required physical direction;
- minimum and maximum operating tilt change the result by at most `0.3 cm`.

If tilt drift is greater than `0.3 cm`, first check camera rigidity. Add angle
compensation only if the measured drift remains significant.
