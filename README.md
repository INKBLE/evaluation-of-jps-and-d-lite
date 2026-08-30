# Evaluation of JPS and D* Lite

用于比较八邻域网格路径规划算法的可复现实验仓库。桌面端将 A*、Jump Point Search（JPS）和 D* Lite 放在相同地图与正确性规则下比较；ESP32 端将同一类规划器编译为 ESP-IDF 固件，按“一个规划器 × 一个场景”的重刷隔离策略采集时间、扩展节点和堆状态。

## 核心特性

- **三种规划器**：A* 作为静态参考，JPS 用方向剪枝与跳点搜索减少扩展，D* Lite 保留初始搜索状态以响应单格地图更新。
- **确定性场景**：随机地图由种子生成；桌面端支持静态与动态事件，ESP32 端将地图 ID、种子和更新坐标写入每条记录。
- **统一代价与碰撞规则**：八邻域移动，直走/对角线代价为 `10/14`；当两个正交侧格均被阻挡时禁止对角移动。
- **正确性优先**：记录路径是否找到、路径合法性以及与 A* 参考代价的一致性；JPS 的强制邻居逻辑显式检查对角移动有效性，避免穿过障碍角点。
- **可审计基准输出**：CSV 包含场景、重复次数、耗时、扩展节点、状态及失败原因；预热行被标记并在统计中排除。
- **受控 ESP32 重测**：脚本为每个 profile 生成配置、构建并刷写固件，捕获串口 CSV，拒绝缺失完成标记、格式错误或重复冲突样本。

## 架构与设计原理

```text
Grid（连续 vector<int>）
  ├─ A*：静态最优路径参考
  ├─ JPS：方向剪枝 / 强制邻居 / 跳点
  └─ D* Lite：初始规划 -> updateCell() -> replan()
       │
       ├─ 桌面：PathSearchFix.cpp -> CSV / 分析脚本
       └─ ESP32：ESP-IDF app_main() -> UART CSV -> 主机汇总
```

`Grid` 将二维坐标映射到连续 `vector<int>`。桌面与固件版本都以起点 `(0, 0)`、终点 `(W-1, H-1)` 为默认场景。动态测试选择仍可求解的内部路径格进行阻塞：A*/JPS 在更新后从头搜索；D* Lite 的计时区间包含 `updateCell + replan`，而不包含其初始建图和初始化。

ESP32 固件使用 `esp_timer_get_time()` 计时，在算法调用前后采样可用堆，并注册失败分配回调捕获 `std::bad_alloc` 的请求大小和阶段。`since_boot_minimum_free_heap` 是**自启动以来的全局低水位**，不是单次算法的峰值内存，因此不得将其解释为独立的内存上界。

## 环境与构建命令

### 桌面基准

要求：C++17 编译器和 Python 3。仓库根目录没有 CMake/Make 构建文件，直接编译单文件程序：

```bash
cd /home/inkble/my_projects/evaluation-of-jps-and-d-lite
g++ -std=c++17 -O2 -Wall -Wextra -pedantic PathSearchFix.cpp -o path_benchmark

# 小规模静态冒烟与结果分析
./path_benchmark --mode random-static --target-solvable 3 \
  --max-candidates 200 --timing-repetitions 2 --warmup-runs 1 \
  --output-dir results/smoke_static
python3 tools/analyze_benchmarks.py --input-dir results/smoke_static
```

仅在确实生成并审核 raw CSV 后报告性能结论；`README_BENCHMARKS.md` 中的 smoke 命令适用于验证管线，不代表通用性能结果。

### ESP32 基准

要求：已安装并导出环境的 ESP-IDF（项目配置使用 ESP-IDF 构建系统）、`idf.py`、Python 包 `pyserial`，以及可访问的串口设备。

```bash
cd /home/inkble/my_projects/evaluation-of-jps-and-d-lite
source "$IDF_PATH/export.sh"
python3 -m pip install pyserial

# 单个、最小化的隔离 smoke profile
python3 tools/run_esp32_matrix.py \
  --project esp_benchmark --device /dev/ttyUSB0 --smoke
```

完整矩阵工具会在每个 planner/scenario profile 前重新构建和刷写设备；执行期间会暂时改写 `esp_benchmark/main/benchmark_profile.h`，并在退出时恢复原内容。详见 [`docs/esp32_rerun_protocol.md`](docs/esp32_rerun_protocol.md)。

### 验证标准

- 成功路径必须通过网格边界、障碍与步进合法性检查，并与参考代价一致。
- 分析工具检查样本键唯一性；三个规划器均可用时，检查同一地图/阶段/重复下的最终路径代价一致性。
- ESP32 必须观察到 `planner_scenario_complete`；看门狗、panic、超时、CSV 异常或冲突会使脚本以非零状态退出。
- OOM 单独记录，不能据此直接推导算法固有内存界限。

## 使用示例

```bash
# 生成桌面静态测试数据并查看汇总 CSV
./path_benchmark --mode random-static --output-dir results/local_run
python3 tools/analyze_benchmarks.py --input-dir results/local_run

# 查看受版本控制的 ESP32 汇总结果（示例）
head -n 5 results/esp32_rerun_smoke_clean/esp32_summary.csv
```

源码入口为 [`PathSearchFix.cpp`](PathSearchFix.cpp) 和 [`esp_benchmark/main/path_benchmark.cpp`](esp_benchmark/main/path_benchmark.cpp)。基准规则、数据格式与实验限制请以 [`README_BENCHMARKS.md`](README_BENCHMARKS.md) 和 `docs/` 下的协议文档为准。