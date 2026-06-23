import socket
import struct
import threading

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from std_msgs.msg import Int32MultiArray

PKT_CMD   = 0x01
PKT_TELEM = 0x02
PKT_HELLO = 0x03

ESP32_PORT = 8889
PI_PORT    = 8890


class Esp32Bridge(Node):

    def __init__(self):
        super().__init__('esp32_bridge')

        # Optional mDNS hint. Leave empty to rely purely on broadcast discovery.
        self.declare_parameter('esp32_host', '')
        host = self.get_parameter('esp32_host').value
        self.esp32_ip = None
        if host:
            try:
                self.esp32_ip = socket.gethostbyname(host)
            except socket.gaierror:
                self.get_logger().warn(f'could not resolve {host}, waiting for hello')

        # one socket: bind for telemetry rx, also used for cmd tx
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('0.0.0.0', PI_PORT))
        self.sock.setblocking(True)

        self.sub = self.create_subscription(Twist, 'cmd_vel', self.on_cmd_vel, 10)
        self.pub_enc = self.create_publisher(Int32MultiArray, 'encoder_counts', 10)

        self.rx_thread = threading.Thread(target=self.rx_loop, daemon=True)
        self.rx_thread.start()

        self.get_logger().info('bridge up, waiting for esp32')

    def on_cmd_vel(self, msg: Twist):
        # Map Twist -> robot frame ints (-100..100)
        # vy = forward (linear.x), vx = strafe (linear.y), omega = yaw (angular.z)
        if self.esp32_ip is None:
            return
        vy    = self.clamp(msg.linear.x  * 100.0)
        vx    = self.clamp(msg.linear.y  * 100.0)
        omega = self.clamp(msg.angular.z * 100.0)

        pkt = struct.pack('<Bhhh', PKT_CMD, vx, vy, omega)
        self.sock.sendto(pkt, (self.esp32_ip, ESP32_PORT))

    def rx_loop(self):
        while rclpy.ok():
            try:
                data, addr = self.sock.recvfrom(64)
            except OSError:
                break

            # Discovery hello: learn address, ack so the esp32 stops broadcasting
            if len(data) >= 1 and data[0] == PKT_HELLO:
                if self.esp32_ip != addr[0]:
                    self.esp32_ip = addr[0]
                    self.get_logger().info(f'found esp32 at {self.esp32_ip}')
                ack = struct.pack('<Bhhh', PKT_CMD, 0, 0, 0)
                self.sock.sendto(ack, (addr[0], ESP32_PORT))
                continue

            # Telemetry: encoder counts + timestamp
            if len(data) >= 21 and data[0] == PKT_TELEM:
                c = struct.unpack('<iiii', data[1:17])
                ts = struct.unpack('<I', data[17:21])[0]
                m = Int32MultiArray()
                m.data = list(c)
                self.pub_enc.publish(m)

    @staticmethod
    def clamp(v):
        return int(max(-100, min(100, v)))


def main():
    rclpy.init()
    node = Esp32Bridge()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
