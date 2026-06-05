# RVTK

[English](README.md)

面向 Unreal Engine 的实时可视化工具包。

该插件目前提供一个运行时组件：`URVTKPolyDataComponent`。它会加载一个二进制三角网格，加载一个包含标量时间步文件的目录，并在每一帧根据当前时间步为网格着色。

## 组件

将 `RVTKPolyDataComponent` 添加到 Actor 上，即可显示带有动画标量数据的 polydata 网格。

该组件继承自 `UDynamicMeshComponent`，因此它会直接持有并渲染动态网格。启动时它会：

1. 从 `Poly Bin Path` 加载网格。
2. 从 `Scalar Bin Directory` 加载标量 `.bin` 文件。
3. 构建动态网格。
4. 应用插件自带材质。
5. 等待通过时间序列 API 更新当前时间。

## 数据设置

在组件上配置以下属性：

- `Poly Bin Path`：网格 `.bin` 文件路径，通常命名为 `mesh.bin`。
- `Scalar Bin Directory`：包含每个时间步标量 `.bin` 文件的目录。

在 Unreal Editor 中编辑路径时，路径会被保存为相对于项目 `Content` 目录的路径。

组件会在指定目录中按 `*.bin` 搜索标量文件。文件名会按数字排序，因此建议使用 `0.bin`、`1.bin`、`2.bin` 这样的命名方式。每个标量文件中的标量数量必须与网格顶点数量一致。

## 时间序列 API

主要运行时 API 位于 Blueprint 的 `Time Series` 分类下。

- `GetTimeRange()`：返回可用时间步的起始值和结束值。
- `GetDuration()`：返回 `EndTime - StartTime`。
- `GetNumberOfSteps()`：返回已加载的标量文件数量。
- `GetCurrentTimeStep()`：返回最后一次应用到网格上的时间步。
- `UpdateTimeStep(float TimeStep)`：根据请求的时间更新网格颜色。

RVTK 不负责管理时钟。你的游戏、仿真逻辑或 Blueprint 应该维护自己的播放时间，并在每一帧调用 `UpdateTimeStep`。

Blueprint 示例流程：

1. 保存一个 `PlaybackTime` 浮点数。
2. 在 Tick 中累加 `Delta Seconds * PlaybackSpeed`。
3. 使用 `GetTimeRange()` / `GetDuration()` 对时间进行循环或限制。
4. 在 `RVTKPolyDataComponent` 上调用 `UpdateTimeStep(PlaybackTime)`。

如果请求的时间落在两个标量文件之间，组件会先插值标量值，然后再应用颜色查找表。

## 常用设置

- `Position Scale`：构建动态网格时缩放顶点位置。
- `Lut Range`：用于着色的标量最小值和最大值范围。
- `Lut Resolution`：颜色查找表中的颜色数量。
- `Debug`：启用组件内置的调试播放循环。
- `Debug Time Scale`：控制内置调试循环的播放速度。

使用 `Lut Range` 可以控制标量值如何映射到颜色。超出范围的值会被钳制到范围内。

## 内置 Blueprint

插件在 `Plugins/RVTK/Content` 下包含两个 Blueprint 资源：

- `BP_DebugPolyData`：用于快速可视化调试的 Actor。将它拖入关卡后，它会绘制网格，并使用组件调试模式循环播放。
- `BP_ExamplePolyData`：基础用法示例，展示如何从 Blueprint 驱动该组件。

当你只需要确认数据是否能加载并渲染时，可以使用 `BP_DebugPolyData`。如果要接入自己的时钟和播放控制，可以从 `BP_ExamplePolyData` 开始。

## 典型用法

1. 启用 RVTK 插件。
2. 将网格 `.bin` 文件和标量 `.bin` 目录放到项目 `Content` 文件夹下。
3. 在关卡中添加一个 Actor。
4. 给该 Actor 添加 `RVTKPolyDataComponent`。
5. 将 `Poly Bin Path` 设置为网格文件。
6. 将 `Scalar Bin Directory` 设置为标量文件目录。
7. 在 Blueprint 或 C++ 中维护一个播放时钟，并在每一帧调用 `UpdateTimeStep`。
8. 根据需要调整 `Lut Range`、`Position Scale` 和播放速度。

## 备注

- 加载网格数据时，组件会将数据转换到 Unreal 坐标系：翻转 Y 轴并应用 `Position Scale`。
- 法线会根据网格三角面生成。
- 默认时间步间隔目前是每个标量文件 `0.1` 秒。
- 播放时组件只更新顶点颜色，不更新网格拓扑。
- 对于 Shipping 构建，请在项目设置中配置 `Additional Non-Asset Directories to Copy`，将 `bin` 文件夹包含进去。
- 使用 Lumen 照明时，polydata 可能会显得发光。可以在 `M_FEM_Unlit_HeadLight_5_4.uasset` 中调整 color scale；默认值为 `1.0`，通常将其设为 `0.03` 可以保持足够亮度并减少发光感。

## 已知问题

- 源 PolyData 的法线有问题，并且可能不正确，但这不会影响可视化结果，因为材质中会使用 `dxdy` 计算法线。
