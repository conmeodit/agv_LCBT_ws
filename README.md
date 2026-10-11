
## Cấu trúc thư mục

```text
agv_LCBT_ws/
├── docs/
├── firmware/
│   └── mega_v2/
├── scripts/
├── src/
│   ├── agv_base/
│   ├── agv_bringup/
│   ├── agv_description/
│   ├── agv_imu/
│   ├── agv_localization/
│   ├── agv_navigation/
│   ├── agv_power/
│   └── agv_safety/
└── udev/
```

Sau khi chạy `colcon build`, ROS 2 sẽ sinh thêm các thư mục `build/`,
`install/` và `log/`. Đây là sản phẩm build cục bộ và không được đưa vào Git.

## Nội dung từng thư mục

### `src/`

Chứa toàn bộ ROS 2 package của robot. Mỗi package có một trách nhiệm riêng,
không truy cập chéo phần cứng nếu không thuộc phạm vi của package đó.

#### `src/agv_base/`

Package giao tiếp cơ sở giữa Raspberry Pi và Arduino Mega.

Trách nhiệm dự kiến:

- Là package duy nhất được phép mở cổng USB Serial của Arduino Mega.
- Nhận lệnh vận tốc đã qua lớp an toàn và gửi xuống Mega.
- Nhận, kiểm tra và giải mã telemetry theo Serial Protocol V2.
- Phân phối encoder, IMU thô, ADC và trạng thái phần cứng qua ROS 2 topics.
- Tính wheel odometry từ encoder khi phần cứng đã được hiệu chuẩn.
- Giám sát kết nối, sequence number, timeout và lỗi giao thức.

`agv_imu` và `agv_power` không được tự mở cổng Serial của Mega.

#### `src/agv_imu/`

Package xử lý dữ liệu BNO055 được `agv_base` nhận từ Arduino Mega.

Trách nhiệm dự kiến:

- Kiểm tra tính hợp lệ của dữ liệu IMU.
- Quy đổi đơn vị và chuẩn hóa quaternion.
- Gán đúng `frame_id` và covariance.
- Xuất `sensor_msgs/msg/Imu` khi dữ liệu hợp lệ.

Package này không đọc I2C trực tiếp trên Raspberry Pi vì BNO055 được nối với
Arduino Mega.

#### `src/agv_power/`

Package giám sát nguồn và tín hiệu chẩn đoán của driver động cơ.

Trách nhiệm dự kiến:

- Chuyển giá trị ADC A0 thành điện áp pin sau khi hiệu chuẩn cầu phân áp.
- Theo dõi các kênh BTS7960 IS tại A2, A3, A4 và A5 sau khi đặc tuyến thực tế
  đã được xác minh.
- Phát cảnh báo điện áp và trạng thái nguồn theo ngưỡng cấu hình.

Robot không sử dụng mạch shunt và LM358. A1 không được sử dụng. Package này
không được tạo dữ liệu dòng pin tổng, công suất hoặc SOC coulomb-counting khi
không có cảm biến phù hợp.

#### `src/agv_safety/`

Package quản lý lớp an toàn phần mềm trước khi lệnh vận tốc đến `agv_base`.

Trách nhiệm dự kiến:

- Giới hạn vận tốc và gia tốc.
- Kiểm tra tuổi của lệnh điều khiển và trạng thái kết nối.
- Quản lý trạng thái arm/disarm và software E-stop.
- Ngăn robot chuyển động khi điều kiện an toàn chưa thỏa mãn.
- Phát lệnh dừng khi ROS, telemetry hoặc kết nối Mega gặp lỗi.

Software safety không thay thế E-stop, cầu chì và các biện pháp bảo vệ điện
phần cứng.

#### `src/agv_description/`

Package mô tả hình học và các hệ tọa độ của robot.

Nội dung dự kiến:

- URDF/Xacro của robot dẫn động vi sai.
- Liên kết và frame của thân robot, bánh xe, LiDAR và IMU.
- Thông số hình học có thể cấu hình.
- Launch file cho `robot_state_publisher`.

Bán kính bánh, khoảng cách hai bánh và vị trí cảm biến phải được đo trên robot
thật; không mặc định giá trị CAD hoặc mô phỏng là giá trị đã hiệu chuẩn.

#### `src/agv_localization/`

Package cấu hình định vị và sensor fusion.

Nội dung dự kiến:

- Cấu hình `robot_localization` và EKF.
- Kết hợp wheel odometry với IMU.
- Cấu hình localization bằng bản đồ khi được triển khai.
- Các launch file và tham số liên quan tới định vị.

Theo quy ước TF, EKF sở hữu transform `odom -> base_link`; SLAM hoặc AMCL sở
hữu `map -> odom`. Không để nhiều node đồng thời publish cùng một transform.

#### `src/agv_navigation/`

Package chứa cấu hình và thành phần tích hợp Nav2.

Nội dung dự kiến:

- Planner, controller và smoother.
- Global/local costmap.
- Velocity smoother và collision monitor.
- Behavior tree và các profile chạy thử nghiệm.
- Cấu hình tích hợp thuật toán nghiên cứu.

Các thuật toán nghiên cứu phức tạp như PSTMO/adaptive pivot nên được giữ trong
package hoặc plugin chuyên biệt thay vì nhồi toàn bộ vào package này.

#### `src/agv_bringup/`

Package điều phối việc khởi chạy toàn bộ hệ thống.

Nội dung dự kiến:

- Nạp tham số và hardware profile.
- Khởi chạy driver phần cứng, LiDAR và mô tả robot.
- Khởi chạy các chế độ mapping, localization và navigation khi đã hoàn thiện.
- Cung cấp launch file cấp cao cho robot thật.

Package này chỉ điều phối; logic xử lý chính phải nằm trong package chịu trách
nhiệm tương ứng.

### `firmware/`

Chứa mã nguồn chạy trên các vi điều khiển của robot.

#### `firmware/mega_v2/`

Firmware dành cho Arduino Mega 2560 theo Serial Protocol V2.

Nội dung dự kiến:

- Đọc encoder hai bánh.
- Đọc BNO055 qua I2C.
- Đọc ADC theo thứ tự A0, A2, A3, A4, A5.
- Điều khiển hai BTS7960 và PID tốc độ từng bánh.
- Quản lý arm/disarm, watchdog và trạng thái lỗi.
- Đóng gói telemetry và xử lý lệnh từ Raspberry Pi.

Firmware phải khởi động với động cơ bị vô hiệu hóa, không tự chạy lại sau khi
USB kết nối lại và không dùng `delay()` làm nghẽn vòng điều khiển.

### `docs/`

Chứa tài liệu thiết kế và vận hành, dự kiến gồm:

- Kiến trúc hệ thống.
- Topic, message và TF contract.
- Đặc tả Serial Protocol V2.
- Sơ đồ đấu dây và bảng chân Arduino Mega.
- Hướng dẫn triển khai trên Raspberry Pi.
- Quy trình kiểm thử, hiệu chuẩn và xác nhận an toàn.
- Nhật ký các thay đổi phần cứng quan trọng.

Tài liệu trong thư mục này phải được cập nhật khi interface firmware/ROS hoặc
cấu hình phần cứng thay đổi.

### `scripts/`

Chứa các công cụ hỗ trợ phát triển và kiểm tra, chẳng hạn:

- Kiểm tra tính nhất quán của cấu hình.
- Sinh header chân firmware từ hardware profile khi cơ chế này được thiết kế.
- Kiểm tra dependency và môi trường ROS 2.
- Thu thập log hoặc hỗ trợ benchmark.
- Hỗ trợ nhập thuật toán từ repository nghiên cứu mà không sửa nguồn gốc.

Script phải có hành vi rõ ràng, không tự động ghi đè cấu hình hoặc điều khiển
động cơ.

### `udev/`

Chứa quy tắc udev cho các thiết bị USB như Arduino Mega và RPLIDAR.

Mục tiêu là cung cấp tên thiết bị ổn định thay vì phụ thuộc vào thứ tự
`/dev/ttyUSB*` hoặc `/dev/ttyACM*`. Quy tắc phải dựa trên thuộc tính thiết bị
đã được kiểm tra trên phần cứng thực tế, không dùng serial number giả định.

## Ranh giới Raspberry Pi và Arduino Mega

Arduino Mega phụ trách xử lý phần cứng thời gian thực: encoder, BNO055, ADC,
PWM/enable BTS7960, PID tốc độ bánh và watchdog độc lập.

Raspberry Pi phụ trách ROS 2, wheel odometry, xử lý IMU, LiDAR, EKF, SLAM,
localization, Nav2, thuật toán nghiên cứu và phát lệnh vận tốc xuống Mega.

## Trạng thái hiện tại

Workspace hiện mới có cây thư mục ban đầu và tài liệu README này. Chưa có ROS 2
package hoàn chỉnh, firmware, Serial Protocol V2, launch file, cấu hình Nav2 hay
bằng chứng build/test trên Raspberry Pi 5.
