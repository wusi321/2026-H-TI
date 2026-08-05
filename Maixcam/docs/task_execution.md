# H题任务执行与控制分工

## 控制分工

MaixCAM Pro只负责视觉观测，连续通过UART发送：

- 钢珠位置 `position_mm`，摆杆中心为0，左负右正；
- 钢珠速度 `velocity_mm_s`；
- `VALID / MEASURED / TRACKED` 标志；
- 置信度、视觉时间戳和处理耗时。

单片机负责：任务选择、循线、A/B点判断、计时、摆杆位置外环、舵机角度环、PWM输出、限幅以及失联保护。MaixCAM不直接控制舵机。

## 任务1：图传与录像

默认使用 MaixCAM 原生 WebRTC 图传。确认 `maixcam/config.py` 中
`ENABLE_WEBRTC = True`，MaixCAM连接电脑热点后，电脑第一次使用时先在浏览器
加载 `host\webrtc_recorder\browser_extension` 扩展，然后运行：

```bat
host\start_webrtc_recorder.cmd
```

网页会打开 `http://127.0.0.1:18765`，填写当前 MaixCAM IP 后连接
`http://设备IP:8000` 的原生 WebRTC 页面。每次按键启动正式测试前先开始录像，
测试完成后点击“完成录制”，等待浏览器生成 MP4 或 WebM 下载。

若现场固件不支持原生 WebRTC，可作为后备启用 `ENABLE_RTSP = True` 并运行：

```bat
host\record_rtsp.cmd 192.168.137.201
```

## 任务2-6状态机

状态机参考 `D:\Maixcam\K230代码\main.py` 的任务编号，但采用厘米/毫米物理坐标，不采用像素坐标。

### 任务2

单片机启动循线一圈并在A点停车，时间限制20秒。滚球目标默认保持在 `0cm`，但任务2得分不依赖滚球控制。

### 任务3

```text
确认稳定在0cm
-> 目标切换到+5cm
-> 确认稳定
-> 目标切换到-5cm
-> 确认稳定
-> 持续保持-5cm
```

总时间限制5秒。进入下一阶段必须同时满足：位置误差不超过8mm、速度不超过50mm/s，并连续保持250ms。不能只因钢珠高速经过目标点就切换状态。

### 任务4

目标固定为 `0cm`，单片机同时执行A到B循线，时间限制8秒。到达B由红外循迹/标志线逻辑判断，不由MaixCAM判断。

### 任务5

目标固定为 `0cm`，循线一圈并通过A，时间限制30秒。

### 任务6

任务启动瞬间锁存当前有效的 `position_mm` 作为目标；若视觉暂时无效，则停车等待下一帧有效观测再锁存。目标限制在钢珠中心的可达范围 `-120mm` 到 `+120mm`，随后循线一圈并保持该位置。

## 单片机主循环接入

```c
ball_vision_packet_t packet;
ball_vision_link_t vision_link;
ball_observation_t observation;
ball_task_output_t task_output;

/* 初始化时执行一次。 */
ball_vision_link_init(&vision_link);

/* 主循环或接收任务消费UART/DMA缓冲区时，为每个收到的字节调用。 */
ball_vision_link_push(
    &vision_link,
    rx_byte,
    millis(),
    &observation,
    &packet
);

/* 固定周期控制循环。 */
ball_task_update(
    &task_controller,
    &observation,
    route_complete,
    millis(),
    &task_output
);

if (!task_output.balance_enabled) {
    servo_request_safe_level();
} else {
    balance_position_loop(
        task_output.target_mm,
        observation.position_mm,
        observation.velocity_mm_s
    );
}

line_following_update(task_output.route_mode);
```

`ball_vision_link_push()` 只有在帧头、字段语义、CRC和序号均有效时才更新
`observation.received_ms`。MaixCAM或UART被明确重启时，调用一次
`ball_vision_link_reset()`，否则重启后从0开始的序号会被当作旧帧。

按键切换任务时调用 `ball_task_start()`；急停、取消任务或测试结束时调用
`ball_task_stop()`，随后立即停止循线并让摆杆回到安全位。传入的
`route_complete` 必须是当前任务的到达事件，启动新任务时应清零旧事件。

推荐的位置外环形式：

```text
error = target_mm - position_mm
theta_ref = Kp * error - Kd * velocity_mm_s
```

`theta_ref`必须经过角度、角速度和PWM限幅，再交给更快的舵机角度环。视觉超过100ms无有效数据时，停止积分、禁止继续扩大舵机命令，并让摆杆回到安全小角度或水平位置。
