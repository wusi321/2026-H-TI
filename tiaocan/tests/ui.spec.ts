import { expect, test } from "@playwright/test";

test("renders tuning console and nonblank canvases", async ({ page }, testInfo) => {
  await page.goto("/");
  await expect(page.getByRole("heading", { name: "滚球调参台" })).toBeVisible();

  await page.getByRole("button", { name: "演示数据" }).click();
  await expect(page.locator("canvas")).toHaveCount(2);

  const canvasPixels = await page.locator("canvas").evaluateAll((canvases) =>
    canvases.map((canvas) => {
      const element = canvas as HTMLCanvasElement;
      const context = element.getContext("2d");
      if (!context || element.width === 0 || element.height === 0) return 0;
      const pixels = context.getImageData(0, 0, element.width, element.height).data;
      let opaque = 0;
      for (let index = 3; index < pixels.length; index += 4) {
        if (pixels[index] !== 0) opaque += 1;
      }
      return opaque;
    }),
  );
  expect(canvasPixels.every((count) => count > 1000)).toBe(true);
  const hasHorizontalOverflow = await page.evaluate(
    () => document.documentElement.scrollWidth > document.documentElement.clientWidth,
  );
  expect(hasHorizontalOverflow).toBe(false);
  await page.evaluate(() => window.scrollTo(0, 0));
  await page.screenshot({
    path: testInfo.outputPath("tuning-console.png"),
    fullPage: true,
  });
});

test("explains Web Serial browser requirement when unavailable", async ({ page }) => {
  await page.addInitScript(() => {
    Object.defineProperty(Navigator.prototype, "serial", {
      configurable: true,
      get: () => undefined,
    });
  });
  await page.goto("/");
  await expect(page.getByText("Web Serial 需要桌面版 Chrome 或 Edge")).toBeVisible();
  await expect(page.getByRole("button", { name: "选择串口" })).toBeDisabled();
});
