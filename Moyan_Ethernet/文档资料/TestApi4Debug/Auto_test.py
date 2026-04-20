import socket

"""
魔眼和充电桩自动测试脚本
"""

class SenderAndReceiver:
    def __init__(self, host, port):
        self.host = host
        self.port = port


    def socket_init(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def socket_connect(self):
        try:
            self.socket.connect((self.host, self.port))
            print(f"连接到 {self.host}:{self.port}")
        except socket.error as e:
            print(f"连接失败: {e}")

    def socket_send(self, data):
        try:
            row_data = data
            data = bytes.fromhex(data)
            self.socket.sendall(data)
            print(f"已发送数据: {row_data}")
        except socket.error as e:
            print(f"发送数据失败: {e}")

    def socket_close(self):
        self.socket.close()
        print("连接已关闭")

    def socket_receive(self, buffer_size=1024):
        try:
            self.socket.settimeout(5)
            row_data = self.socket.recv(buffer_size)
            if data:
                print(f"接收到数据: {row_data.decode()}")
            else:
                print("未接收到数据，连接可能关闭")
        except socket.timeout:
            print("接收数据超时")
        except socket.error as e:
            print(f"接收数据失败: {e}")

    def sand_and_rec(self, data):
        senderAndReceiver.socket_send(data)
        senderAndReceiver.socket_receive()

if __name__=="__main__":
    senderAndReceiver = SenderAndReceiver(host='127.0.0.1', port=10016)
    senderAndReceiver.socket_init()
    senderAndReceiver.socket_connect()
    data_dict = {
        1:"AA DD 09 1D 00 FF DF EE FF", # 握手
        2:"DD BB 14 1D 00 FF DF 01 02 00 00 00 15 00 00 01 06  01 EE FF", # 当充电时间达到设定时间或电量，发送对应通道的时间和电能累计
        # DD BB 帧头 15 长度 00 FF DF FF设备号
        # 01 环号 02通道号 00 00 00 05 浮点数电量 00 00 00 06整数时间 01 工作模式 EE FF帧尾
        3:"EA EA 25 1D 00 FF DF 01 00 00 00 00 00 20 00 00 00 00 00 00" +
          " 00 00 05 00 00 00 00 00 01 00 00 00 00 00 00 EE FF", # 心跳包
    }

    while True:
        num = input("选择要发送的数据：")
        if num != '0':
            data = data_dict.get(int(num))
            senderAndReceiver.sand_and_rec(data)
        else:
            senderAndReceiver.socket_close()
            break

