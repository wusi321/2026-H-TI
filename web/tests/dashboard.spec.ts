import { expect, test } from "@playwright/test";

test("renders live telemetry without viewport overflow", async ({ page }, testInfo) => {
  await page.goto("/");
  await expect(page.getByRole("heading", { name: "NC Motion Console" })).toBeVisible();
  await page.getByRole("button", { name: "演示数据" }).click();

  const validFrames = page.locator(".diagnostic-grid > div").first().locator("strong");
  await expect.poll(async () => Number(await validFrames.textContent())).toBeGreaterThan(5);
  await expect(page.getByText("演示运行", { exact: true })).toBeVisible();

  const pageOverflows = await page.evaluate(() =>
    document.documentElement.scrollWidth > window.innerWidth + 1,
  );
  expect(pageOverflows).toBe(false);

  const pathHasTelemetryPixels = await page.locator(".path-canvas").evaluate((element) => {
    const canvas = element as HTMLCanvasElement;
    const context = canvas.getContext("2d");
    if (!context || canvas.width === 0 || canvas.height === 0) return false;
    const pixels = context.getImageData(0, 0, canvas.width, canvas.height).data;
    let tealPixels = 0;
    for (let index = 0; index < pixels.length; index += 16) {
      const red = pixels[index];
      const green = pixels[index + 1];
      const blue = pixels[index + 2];
      if (green > red * 1.7 && green > blue * 1.15 && green > 70) {
        tealPixels += 1;
      }
    }
    return tealPixels > 10;
  });
  expect(pathHasTelemetryPixels).toBe(true);

  await page.screenshot({ path: testInfo.outputPath("dashboard.png"), fullPage: true });
});
