# Báo cáo Thực hành: Đồng hồ Thời gian thực với DS3231 và LCD trên STM32

## 1. Giới thiệu

Dự án này xây dựng một chiếc đồng hồ kỹ thuật số hoàn chỉnh sử dụng vi điều khiển STM32F407VET6. Đồng hồ có khả năng hiển thị thời gian (giờ, phút, giây), ngày tháng năm và thứ trong tuần lên màn hình LCD có hình nền. Người dùng có thể tương tác với đồng hồ thông qua các nút nhấn để vào chế độ cài đặt và điều chỉnh thời gian.

Module thời gian thực (RTC) DS3231 được sử dụng để duy trì thời gian một cách chính xác, ngay cả khi vi điều khiển bị mất nguồn, nhờ vào viên pin dự phòng.

### Các chức năng chính:
-   Hiển thị thời gian, ngày tháng, thứ trong tuần trên màn hình LCD có hình nền.
-   Chế độ **NORMAL**: Hiển thị thời gian hiện tại.
-   Chế độ **ADJUST**: Cho phép người dùng điều chỉnh lần lượt: Giờ, Phút, Thứ, Ngày, Tháng, Năm.
-   Sử dụng máy trạng thái (State Machine) để quản lý các chế độ hoạt động.
-   Tối ưu hóa việc cập nhật màn hình để giảm nhấp nháy và tăng hiệu năng.

## 2. Phần cứng và Phần mềm

### 2.1. Linh kiện phần cứng
-   Kit phát triển STM32F407VET6.
-   Module RTC DS3231 (giao tiếp qua I2C).
-   Màn hình LCD 2.4 inch (giao tiếp qua FSMC).
-   Các nút nhấn (MODE, SET, UP, DOWN) để điều khiển.
-   Dây cắm và breadboard.

### 2.2. Công cụ phần mềm
-   **IDE**: STM32CubeIDE.
-   **Code Generator**: STM32CubeMX để cấu hình chân và các ngoại vi (I2C, FSMC, GPIO, TIM).
-   **Thư viện**: HAL (Hardware Abstraction Layer) của STMicroelectronics.
-   **Ngôn ngữ**: C.

## 3. Thiết kế hệ thống

### 3.1. Sơ đồ khối

```
+----------------+      I2C      +----------+
|                | <-----------> |  DS3231  |
|   STM32F407    |               |   RTC    |
|                |      FSMC     +----------+
|                | <-----------> |   LCD    |
|                |               +----------+
|                |      GPIO     +----------+
|                | <-----------  |  Buttons |
+----------------+               +----------+
```

### 3.2. Thiết kế phần mềm

Phần mềm được tổ chức theo kiến trúc máy trạng thái (State Machine) để quản lý các chế độ hoạt động của đồng hồ.

#### a. Máy trạng thái (State Machine)

Hệ thống có các trạng thái được định nghĩa trong `enum ClockState`:
-   `NORMAL`: Trạng thái hiển thị thời gian bình thường.
-   `ADJUST_INIT`: Trạng thái khởi tạo, đọc thời gian hiện tại vào các biến tạm để chuẩn bị điều chỉnh.
-   `ADJUST_HOUR`: Chỉnh giờ.
-   `ADJUST_MIN`: Chỉnh phút.
-   `ADJUST_DAY`: Chỉnh thứ.
-   `ADJUST_DATE`: Chỉnh ngày.
-   `ADJUST_MONTH`: Chỉnh tháng.
-   `ADJUST_YEAR`: Chỉnh năm.

**Luồng chuyển trạng thái:**
1.  Ở trạng thái `NORMAL`, nhấn nút `MODE` sẽ chuyển sang `ADJUST_INIT` và ngay lập tức sang `ADJUST_HOUR`.
2.  Trong các trạng thái `ADJUST_*`, nhấn nút `SET` sẽ chuyển sang trạng thái chỉnh thông số tiếp theo (ví dụ: từ `ADJUST_HOUR` sang `ADJUST_MIN`).
3.  Sau khi chỉnh `YEAR` và nhấn `SET`, tất cả các giá trị tạm thời sẽ được ghi vào module DS3231 và hệ thống quay trở lại trạng thái `NORMAL`.

#### b. Luồng hoạt động chính (`main.c`)

-   Trong hàm `main()`, sau khi khởi tạo hệ thống (`system_init()`), chương trình đi vào vòng lặp `while(1)`.
-   Một timer (`TIM2`) được cấu hình để tạo ra một ngắt định kỳ (50ms), cắm cờ `timer2_flag`.
-   Bên trong vòng lặp, khi cờ `timer2_flag` được bật:
    -   Quét trạng thái các nút nhấn (`button_scan()`).
    -   Kiểm tra trạng thái hiện tại của đồng hồ (`clockState`):
        -   Nếu là `NORMAL`, đọc thời gian từ DS3231 và kiểm tra nút `MODE` để chuyển sang chế độ chỉnh sửa.
        -   Nếu là một trong các trạng thái `ADJUST_*`, gọi hàm `handle_adjust_mode()` để xử lý logic điều chỉnh.
    -   Gọi hàm `DisplayTime()` để cập nhật hiển thị lên LCD.

#### c. Xử lý chế độ điều chỉnh (`handle_adjust_mode`)

Hàm này là một `switch-case` lớn dựa trên `clockState`.
-   Khi ở một trạng thái `ADJUST_*` cụ thể, nó sẽ kiểm tra các nút `UP` và `DOWN` (cả nhấn và nhấn giữ) để tăng/giảm giá trị của biến tạm tương ứng (ví dụ: `temp_hours`).
-   Nó cũng kiểm tra nút `SET` để chuyển sang trạng thái tiếp theo.
-   Khi ở trạng thái `ADJUST_YEAR`, việc nhấn `SET` sẽ kích hoạt ghi tất cả các biến tạm (`temp_hours`, `temp_min`,...) vào DS3231 và chuyển về `NORMAL`.

#### d. Hiển thị lên LCD (`DisplayTime`)

Đây là một hàm quan trọng, chịu trách nhiệm hiển thị thông tin một cách thông minh.
-   **Tối ưu hóa hiển thị**: Hàm sử dụng các biến `static prev_*` để lưu lại giá trị đã hiển thị ở lần cập nhật trước. Ở mỗi lần gọi, nó chỉ vẽ lại những thông tin nào đã thay đổi so với lần trước. Điều này giúp giảm đáng kể việc vẽ lại toàn bộ màn hình, tránh nhấp nháy và tiết kiệm tài nguyên CPU.
-   **Hiển thị theo chế độ**:
    -   Ở chế độ `NORMAL`, nó hiển thị thời gian đọc từ DS3231.
    -   Ở chế độ `ADJUST`, nó hiển thị giá trị từ các biến tạm (`temp_*`).
-   **Hiệu ứng nhấp nháy (Blinking)**: Trong chế độ `ADJUST`, thông số đang được chỉnh sẽ nhấp nháy với tần số 1Hz (sáng 500ms, tắt 500ms). Điều này được thực hiện bằng cách sử dụng một biến đếm `blink_counter` và chỉ hiển thị văn bản khi `blink_counter` nằm trong một khoảng nhất định. Khi "tắt", hàm `RestoreBackground()` được gọi để vẽ lại phần nền ảnh phía sau.
-   **Hàm `RestoreBackground()`**: Một chức năng phụ trợ quan trọng, dùng để vẽ lại một vùng chữ nhật trên màn hình từ dữ liệu ảnh gốc (`gImage_a`). Điều này rất hữu ích để "xóa" văn bản cũ hoặc tạo hiệu ứng nhấp nháy mà không cần xóa toàn bộ màn hình.

## 4. Kết quả và Đánh giá

### 4.1. Kết quả đạt được
-   Hệ thống hoạt động ổn định, hiển thị thời gian chính xác.
-   Chức năng điều chỉnh thời gian hoạt động đúng như thiết kế, giao diện người dùng trực quan thông qua hiệu ứng nhấp nháy.
-   Việc tối ưu hóa hiển thị giúp màn hình cập nhật mượt mà, không bị nhấp nháy.
-   Module DS3231 giữ thời gian tốt sau khi ngắt nguồn điện của vi điều khiển.

### 4.2. Hạn chế và Hướng phát triển
-   **Logic ngày tháng**: Logic tăng/giảm ngày (`ADJUST_DATE`) chưa kiểm tra tính hợp lệ của ngày trong tháng (ví dụ: tháng 2 chỉ có 28/29 ngày, tháng 4 chỉ có 30 ngày). Cần cải tiến để tự động điều chỉnh khi người dùng tăng/giảm vượt quá giới hạn.
-   **Giao diện người dùng**: Giao diện hiện tại dựa trên văn bản. Có thể phát triển một giao diện đồ họa (GUI) đẹp mắt hơn.
-   **Thêm tính năng**:
    -   Thêm chức năng báo thức (Alarm).
    -   Hiển thị nhiệt độ từ cảm biến tích hợp sẵn trong DS3231.
    -   Lưu trữ nhiều mốc thời gian báo thức vào EEPROM.

## 5. Kết luận

Dự án đã hoàn thành mục tiêu đề ra là xây dựng một đồng hồ thời gian thực đa chức năng. Qua quá trình thực hiện, các kỹ năng sau đã được củng cố:
-   Giao tiếp I2C với module RTC.
-   Lập trình điều khiển màn hình LCD qua giao tiếp FSMC.
-   Kỹ thuật lập trình với máy trạng thái để quản lý các tác vụ phức tạp.
-   Tối ưu hóa hiệu năng hiển thị trong các hệ thống nhúng.

Đây là một bài thực hành rất hữu ích, cung cấp nền tảng vững chắc để phát triển các ứng dụng nhúng phức tạp hơn trong tương lai.