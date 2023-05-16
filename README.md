# Network Programming

## [20230411: Tuần 1](https://github.com/haiphamcoder/Network-Programming/tree/main/exercises/20230411)

**Bài 1:** Viết chương trình tcp_client, kết nối đến một máy chủ xác định bởi địa chỉ IP và cổng. Sau đó nhận dữ liệu từ bàn phím và gửi đến server. Tham số được truyền vào từ dòng lệnh có dạng:

> tcp_client <địa chỉ IP> <cổng>

**Bài 2:** Viết chương trình tcp_server, đợi kết nối ở cổng xác định bởi tham số dòng lệnh. Mỗi khi có client kết nối đến, thì gửi xâu chào được chỉ ra trong một tệp tin xác định, sau đó ghi toàn bộ nội dung client gửi đến vào một tệp tin khác được chỉ ra trong tham số dòng lệnh:

> tcp_server <cổng> <tệp tin chứa câu chào> <tệp tin lưu nội dung client gửi đến>

**Bài 3:** Viết chương trình sv_client, cho phép người dùng nhập dữ liệu là thông tin của sinh viên bao gồm MSSV, họ tên, ngày sinh, và điểm trung bình các môn học. Các thông tin trên được đóng gói và gửi sang sv_server. Địa chỉ và cổng của server được nhập từ tham số dòng lệnh.

**Bài 4:** Viết chương trình sv_server, nhận dữ liệu từ sv_client, in ra màn hình và đồng thời ghi vào file sv_log.txt. Dữ liệu được ghi trên một dòng với mỗi client, kèm theo địa chỉ IP và thời gian client đã gửi. Tham số cổng và tên file log được nhập từ tham số dòng lệnh.Ví dụ dữ liệu trong file log:

> 127.0.0.1 2023-04-10 09:00:00 20201234 Nguyen Van A 2002-04-10 3.99
> 127.0.0.1 2023-04-10 09:00:10 20205678 Tran Van B 2002-08-18 3.50

## [20230418: Tuần 2](https://github.com/haiphamcoder/Network-Programming/tree/main/exercises/20230418)

**Bài 1:** Ứng dụng info_client cho phép người dùng nhập tên máy tính (là chuỗi ký tự),danh sách các ổ đĩa (gồm ký tự và kích thước ổ đĩa)từ bàn phím.Các dữ liệu này sau đó được đóng gói và chuyển sang info_server.

Ứng dụng info_server nhận dữ liệu từ info_client, tách các dữ liệu và in ra màn hình. Ví dụ:

> + Tên máy tính MY_LAPTOP_DELL
>
> + Số ổ đĩa 3
>
>   C–500GB
>
>   D–250GB
>
>   E–250GB

**Bài 2:** Ứng dụng client đọc nội dung file văn bản và gửi sang server.

Ứng dụng server nhận dữ liệu từ client,in ra màn hình số lần xuất hiện xâu ký tự (server không cần tạo file để chứa nội dung nhận được): “0123456789”

Chú ý cần xử lý trường hợp khi xâu“0123456789” nằm giữa 2 lần truyền. Ví dụ nội dung file văn bản:

> SOICTSOICT0123456789012345678901234567890123456789SOICTSOICTSOICT01234567890123456789012345678901234567890123456789012345678901234567890123456789SOICTSOICT

**Bài 3:** Ứng dụng udp_file_sender cho phép người dùng nhập tên file từ dòng lệnh, sau đó truyền tên và nội dung file sang udp_file_receiver. Địa chỉ IP và cổng của receiver cũng được nhập từ dòng lệnh.

Ứng dụng udp_file_receiver nhận dữ liệu từ udp_file_sender và ghi vào file.Cổng chờ được nhập từ dòng lệnh.

**Bài 4:** Tại một thời điểm có thể có nhiều udp_file_sender cùng chạy.Đểm minh họa tình huống này, có thể dùng lệnh usleep() để tạm dừng chương trình trong một khoảng thời gian nhằm giảm tốc độ gửi file.

Ứng dụng udp_file_receiver có thể nhận nội dung các file từ nhiều udp_file_sender khác nhau. Ứng dụng cần phân biệt nội dung file được gửi từ sender nào để thực hiện việc ghép nội dung file được chính xác.

## [20230425: Tuần 3](https://github.com/haiphamcoder/Network-Programming/tree/main/exercises/20230425)

**Bài 1:** Sử dụng hàm select()/poll(), viết chương  trình chat_server thực hiện các chức năng sau:

Nhận kết nối từ các client, và vào hỏi tên client cho đến khi client gửi đúng cú pháp:“client_id: client_name” trong đó client_name là tên của client, xâu ký tự viết liền.

Sau đó nhận dữ liệu từ một client và gửi dữ liệu đó đến các client còn lại, ví dụ:

client có id “abc” gửi “xin chao” thì các client khác sẽ nhận được: “abc: xin chao” hoặc có thể thêm  thời gian vào trước ví dụ:

> 2023/05/06 11:00:00PM abc: xin chao

## [20230509: Tuần 4](https://github.com/haiphamcoder/Network-Programming/tree/main/exercises/20230509)

**Bài 1:** Sử dụng hàm select()/poll(), viết chương  trình chat_server thực hiện các chức năng sau:

Nhận kết nối từ các client, và vào hỏi tên client chođến khi client gửi đúng cú pháp:“client_id: client_name”, trong đó client_name là tên của client, xâu ký tự viết liền.

Sau đó nhận dữ liệu từ một client và gửi dữ liệu đóđến các client còn lại, ví dụ: client có id “abc” gửi “xin chao” thì các client khác sẽ nhận được: “abc: xin chao”hoặc có thể thêm thời gian vào trước, ví  dụ: “2023/05/06 11:00:00PM abc: xin chao”.

**Bài 2:** Sử dụng hàm select()/poll(), viết chương trình telnet_server thực hiện các chức năng sau:

Khi đã kết nối với 1 client nào đó, yêu cầu client gửi user và pass, so sánh với file cơ sở dữ liệu là một file text, mỗi dòng chứa một cặp user + pass ví dụ:

> admin admin
>
> guest nopass
>
> ...

Nếu so sánh sai, không tìm thấy tài khoản thì báo lỗi đăng nhập. Nếu đúng thì đợi lệnh từ client, thực hiện lệnh và trả kết quả cho client. Dùng hàm system(“dir > out.txt”) để thực hiện lệnh.

dir là ví dụ lệnh dir mà client gửi. > out.txt để định hướng lại dữ liệu ra từ lệnh dir, khi đó kết quả lệnh dir sẽ được ghi vào file văn bản.
