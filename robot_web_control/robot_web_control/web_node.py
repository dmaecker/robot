import json
import threading
import asyncio

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import CompressedImage
from geometry_msgs.msg import Twist
from aiohttp import web

from robot_web_control.webpage import PAGE


class WebNode(Node):
    # ROS node: holds the latest JPEG frame and publishes joystick commands as Twist
    def __init__(self):
        super().__init__('web_node')

        self.declare_parameter('port', 8080)
        self.declare_parameter('image_topic', '/image_raw/compressed')
        self.declare_parameter('cmd_topic', '/cmd_vel')
        self.declare_parameter('max_linear', 0.3)
        self.declare_parameter('max_angular', 1.5)
        self.declare_parameter('stream_fps', 15.0)

        self.port = self.get_parameter('port').value
        self.max_linear = self.get_parameter('max_linear').value
        self.max_angular = self.get_parameter('max_angular').value
        self.stream_period = 1.0 / float(self.get_parameter('stream_fps').value)

        image_topic = self.get_parameter('image_topic').value
        cmd_topic = self.get_parameter('cmd_topic').value

        self._latest_jpeg = None
        self._lock = threading.Lock()

        self.create_subscription(
            CompressedImage, image_topic, self._on_image, qos_profile_sensor_data)
        self._cmd_pub = self.create_publisher(Twist, cmd_topic, 1)

        self.get_logger().info(
            f'web_node up: subscribing {image_topic}, publishing {cmd_topic}')

    def _on_image(self, msg):
        # Store raw JPEG bytes; format already 'jpeg' from v4l2_camera MJPG mode
        with self._lock:
            self._latest_jpeg = bytes(msg.data)

    def get_jpeg(self):
        with self._lock:
            return self._latest_jpeg

    def publish_cmd(self, vx, vy, omega):
        # Map normalized [-1,1] joystick axes to  velocity limits
        t = Twist()
        t.linear.x = float(vx) * self.max_linear
        t.linear.y = float(vy) * self.max_linear
        t.angular.z = float(omega) * self.max_angular
        self._cmd_pub.publish(t)


async def index_handler(request):
    return web.Response(text=PAGE, content_type='text/html')


async def stream_handler(request):
    
    node = request.app['node']
    resp = web.StreamResponse()
    resp.content_type = 'multipart/x-mixed-replace; boundary=frame'
    resp.headers['Cache-Control'] = 'no-cache'
    await resp.prepare(request)
    try:
        while True:
            jpeg = node.get_jpeg()
            if jpeg:
                await resp.write(
                    b'--frame\r\n'
                    b'Content-Type: image/jpeg\r\n'
                    b'Content-Length: ' + str(len(jpeg)).encode() + b'\r\n\r\n'
                    + jpeg + b'\r\n')
            await asyncio.sleep(node.stream_period)
    except (ConnectionResetError, asyncio.CancelledError):
        pass
    return resp


async def ws_handler(request):
    # Receives joystick packets and republishes them as /cmd_vel
    node = request.app['node']
    ws = web.WebSocketResponse()
    await ws.prepare(request)
    async for m in ws:
        if m.type == web.WSMsgType.TEXT:
            try:
                d = json.loads(m.data)
                node.publish_cmd(
                    d.get('vx', 0.0), d.get('vy', 0.0), d.get('omega', 0.0))
            except (json.JSONDecodeError, ValueError):
                node.get_logger().warn('bad ws packet')
        elif m.type == web.WSMsgType.ERROR:
            break
    # Stop motors when the socket closes
    node.publish_cmd(0.0, 0.0, 0.0)
    return ws


def run_http(node):
    # aiohttp needs its own event loop in this thread
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    app = web.Application()
    app['node'] = node
    app.router.add_get('/', index_handler)
    app.router.add_get('/stream', stream_handler)
    app.router.add_get('/ws', ws_handler)
    web.run_app(app, host='0.0.0.0', port=node.port, loop=loop,
                handle_signals=False, print=None)


def main():
    rclpy.init()
    node = WebNode()
    http_thread = threading.Thread(target=run_http, args=(node,), daemon=True)
    http_thread.start()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
