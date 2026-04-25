import sys
print(sys.executable)  # 复制这个输出路径

try:
    from openai import OpenAI
except ImportError:
    print("警告: OpenAI库未安装，请运行: pip install openai")
    OpenAI = None
try:
    import cv2

    CV2_AVAILABLE = True
except ImportError as e:
    print(f"警告: OpenCV库导入失败: {e}")
    print("解决方案:")
    print("1. 重新安装OpenCV: pip uninstall opencv-python && pip install opencv-python")
    print("2. 或安装预编译版本: pip install opencv-python-headless")
    print("3. 检查Visual C++运行时库是否安装")
    print("程序将尝试在无OpenCV模式下运行...")
    CV2_AVAILABLE = False
    cv2 = None
import time
import numpy as np
import base64
import threading

try:
    import torch
except ImportError:
    print("警告: PyTorch库未安装，请运行: pip install torch")
    torch = None
from PIL import ImageFont, ImageDraw, Image
import numpy as np

try:
    from ultralytics import YOLO
except ImportError:
    print("警告: Ultralytics YOLO库未安装，请运行: pip install ultralytics")
    YOLO = None
import base64
from textwrap import wrap
import threading
from queue import Queue

try:
    import pyaudio
except ImportError:
    print("警告: PyAudio库未安装，请运行: pip install pyaudio")
    pyaudio = None
import wave
import os
import math

try:
    import mediapipe as mp
except ImportError:
    print("警告: MediaPipe库未安装，请运行: pip install mediapipe")
    mp = None
try:
    import autopy
except ImportError:
    print("警告: AutoPy库未安装，请运行: pip install autopy")
    autopy = None
import collections

try:
    import webrtcvad
except ImportError:
    print("警告: WebRTC VAD库未安装，请运行: pip install webrtcvad")
    webrtcvad = None
try:
    import dashscope
    from dashscope.audio.asr import Recognition, RecognitionCallback, RecognitionResult
except ImportError:
    print("警告: DashScope库未安装，请运行: pip install dashscope")
    dashscope = None
    Recognition = None
    RecognitionCallback = object  # 提供一个基类
    RecognitionResult = None
import json
import concurrent.futures  # For asynchronous loading
import mmap
import struct
from multiprocessing import shared_memory

# Configuration Management
CONFIG_FILE = "config.json"


def load_config():
    default_config = {
        "dashscope_api_key": "sk-220786dcfe964f98926def01ebc9eb95",
        "camera_index": 0,
        "window_name": "AI Vision Assistant",
        "model_path": "yolov13x.pt",
        "font_path": "msyh.ttc",
        "font_size": 25,
        "max_text_height": 600,
        "max_text_lines": 20,
        "force_shared_memory_only": True,  # 强制只使用共享内存模式
        "line_spacing": 3,
        "circle_radius": 100,
        "vad_enabled": True,  # 默认启动VAD语音检测
        "vad_config": {
            "audio": {
                "format": pyaudio.paInt16 if pyaudio else 8,  # 8 = paInt16
                "channels": 1,
                "rate": 16000,
                "chunk": 480,
                "frame_duration": 10,
                "silence_timeout": 0.5,
                "speech_threshold": 0.20,
                "pre_buffer_duration": 0.01,
                "noise_threshold": 11,
                "max_record_seconds": 3.0
            }
        },
        "voice_config": {
            "audio": {
                "format": pyaudio.paInt16 if pyaudio else 8,
                "channels": 1,
                "rate": 16000,
                "chunk": 1024,
                "max_record_seconds": 8,
                "output_rate": 24000
            },
            "filenames": {
                "audio": "temp_audio.wav"
            }
        }
    }
    if os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, 'r') as f:
            user_config = json.load(f)
        default_config.update(user_config)

    return default_config


CONFIG = load_config()

# 完全按照main06.py的全局变量定义
scroll_offset = 0
last_response = ""
last_thumbnail = None
analysis_active = False
sending_status = ""
voice_recognition_text = ""
circle_radius = 100
vad_enabled = True  # 默认启动VAD语音检测
current_detected_objects = []
current_target = None
response_lock = threading.Lock()  # 添加响应锁
client = None  # 全局OpenAI客户端

# 初始化OpenAI客户端
if OpenAI is not None:
    client = OpenAI(
        base_url="https://dashscope.aliyuncs.com/compatible-mode/v1",
        api_key=CONFIG["dashscope_api_key"],
        timeout=30.0
    )

# 字体配置 - 与main06.py一致
FONT_PATH = "msyh.ttc"
FONT_SIZE = 25
try:
    font = ImageFont.truetype(FONT_PATH, FONT_SIZE) if FONT_PATH else None
    print(f"[OK] 字体加载成功: {FONT_PATH}")
except Exception as e:
    print(f"[WARN] 字体加载失败: {e}")
    font = ImageFont.load_default()
    print("[OK] 使用默认字体")


# AI结果共享内存接口
class AIResultsInterface:
    """AI分析结果共享内存接口"""

    def __init__(self):
        self.results_shm = None
        self.connected = False
        self.write_lock = threading.RLock()  # 递归锁，防止写入竞争

    def connect(self):
        """创建AI结果共享内存"""
        try:
            print("🔍 正在创建AI结果共享内存...")
            # 尝试创建共享内存，如果已存在则连接
            try:
                self.results_shm = shared_memory.SharedMemory(name="ai_analysis_results", create=True, size=4096)
                print("✅ 成功创建AI结果共享内存")
            except FileExistsError:
                self.results_shm = shared_memory.SharedMemory(name="ai_analysis_results")
                print("✅ 成功连接到已存在的AI结果共享内存")

            self.connected = True

            # 初始化共享内存内容
            self._initialize_shared_memory()
            return True

        except Exception as e:
            print(f"❌ 创建AI结果共享内存失败: {e}")
            self.connected = False
            return False

    def _initialize_shared_memory(self):
        """初始化共享内存内容"""
        if not self.connected or not self.results_shm:
            return

        # 清零整个共享内存
        self.results_shm.buf[:] = b'\x00' * 4096

        # 写入魔数 (0x41495254 = "AIRT")
        magic = 0x41495254
        struct.pack_into('<I', self.results_shm.buf, 0, magic)

        # 初始化时间戳
        timestamp = int(time.time())
        struct.pack_into('<I', self.results_shm.buf, 4, timestamp)

        print("🔄 AI结果共享内存已初始化")

    def write_results(self, voice_text="", ai_response="", detection_boxes=None):
        """写入AI分析结果"""
        if not self.connected or not self.results_shm:
            print("❌ AI结果共享内存未连接，无法写入")
            return False

        # 线程安全的写入操作
        with self.write_lock:
            try:
                # 编码文本为UTF-8字节
                voice_bytes = voice_text.encode('utf-8', errors='replace')[:511]  # 最多511字节，保留1字节给null terminator
                ai_bytes = ai_response.encode('utf-8', errors='replace')[:2335]  # 最多2335字节，保留1字节给null terminator

                # 处理检测框数据
                detection_count = 0
                if detection_boxes and isinstance(detection_boxes, list):
                    detection_count = min(len(detection_boxes), 28)  # 最多28个检测框

                # 清零相关区域，确保干净的数据
                self.results_shm.buf[32:32 + 512] = b'\x00' * 512  # 清零语音区域
                self.results_shm.buf[544:544 + 1120] = b'\x00' * 1120  # 清零检测框区域
                self.results_shm.buf[1664:1664 + 2336] = b'\x00' * 2336  # 清零AI响应区域

                # 写入头部信息
                struct.pack_into('<I', self.results_shm.buf, 0, 0x41495254)  # 魔数
                struct.pack_into('<I', self.results_shm.buf, 4, int(time.time()))  # 时间戳
                struct.pack_into('<I', self.results_shm.buf, 8, len(voice_bytes))  # 语音文本长度
                struct.pack_into('<I', self.results_shm.buf, 12, detection_count)  # 检测框数量
                struct.pack_into('<I', self.results_shm.buf, 16, len(ai_bytes))  # AI响应长度

                # 保留字段 (3个uint32_t)
                struct.pack_into('<III', self.results_shm.buf, 20, 0, 0, 0)

                # 写入语音识别文本 (偏移32, 长度512)
                if voice_bytes:
                    voice_section = self.results_shm.buf[32:32 + 512]
                    voice_section[:len(voice_bytes)] = voice_bytes
                    voice_section[len(voice_bytes)] = 0  # null terminator

                # 写入检测框数据 (偏移544, 每个检测框40字节，最多28个)
                if detection_count > 0:
                    detection_offset = 544
                    for i, detection_box in enumerate(detection_boxes[:detection_count]):
                        box_offset = detection_offset + i * 40

                        # DetectionBox结构: x(4), y(4), w(4), h(4), confidence(4), class_id(4), class_name(16)
                        try:
                            x, y, w, h = detection_box.get('bbox', [0, 0, 0, 0])
                            confidence = detection_box.get('conf', 0.0)
                            class_name = detection_box.get('class', 'unknown')[:15]  # 最多15字符，保留1字节给null terminator
                            class_id = detection_box.get('class_id', 0)

                            # 写入检测框数据
                            struct.pack_into('<f', self.results_shm.buf, box_offset, float(x))  # x
                            struct.pack_into('<f', self.results_shm.buf, box_offset + 4, float(y))  # y
                            struct.pack_into('<f', self.results_shm.buf, box_offset + 8, float(w))  # w
                            struct.pack_into('<f', self.results_shm.buf, box_offset + 12, float(h))  # h
                            struct.pack_into('<f', self.results_shm.buf, box_offset + 16,
                                             float(confidence))  # confidence
                            struct.pack_into('<I', self.results_shm.buf, box_offset + 20, int(class_id))  # class_id

                            # 写入类别名称 (16字节，包含null terminator)
                            class_name_bytes = class_name.encode('utf-8', errors='replace')
                            class_name_section = self.results_shm.buf[box_offset + 24:box_offset + 40]
                            class_name_section[:len(class_name_bytes)] = class_name_bytes
                            class_name_section[len(class_name_bytes)] = 0  # null terminator

                        except Exception as box_error:
                            print(f"⚠️ 写入检测框 {i} 时出错: {box_error}")
                            # 写入默认值
                            struct.pack_into('<ffffII', self.results_shm.buf, box_offset, 0.0, 0.0, 0.0, 0.0, 0.0, 0)

                # 写入AI响应文本 (偏移1664, 长度2336)
                if ai_bytes:
                    ai_section = self.results_shm.buf[1664:1664 + 2336]
                    ai_section[:len(ai_bytes)] = ai_bytes
                    ai_section[len(ai_bytes)] = 0  # null terminator

                print(f"✅ 成功写入AI结果到共享内存:")
                print(f"   语音文本长度: {len(voice_bytes)} 字节")
                print(f"   检测框数量: {detection_count}")
                print(f"   AI响应长度: {len(ai_bytes)} 字节")
                print(f"   语音内容: {voice_text[:30]}{'...' if len(voice_text) > 30 else ''}")
                print(f"   AI内容: {ai_response[:50]}{'...' if len(ai_response) > 50 else ''}")
                if detection_count > 0:
                    print(f"   检测框信息: {[box.get('class', 'unknown') for box in detection_boxes[:detection_count]]}")

                return True

            except Exception as e:
                print(f"❌ 写入AI结果失败: {e}")
                import traceback
                traceback.print_exc()
                return False

    def test_write_and_read(self):
        """测试写入和读取功能"""
        if not self.connected or not self.results_shm:
            return False

        try:
            # 写入测试数据
            test_voice = "测试语音识别结果"
            test_ai = "这是一个测试AI响应，用来验证共享内存是否正常工作。"

            success = self.write_results(test_voice, test_ai)
            if not success:
                print("❌ 测试写入失败")
                return False

            # 尝试读取数据验证
            magic = struct.unpack('<I', self.results_shm.buf[:4])[0]
            voice_len = struct.unpack('<I', self.results_shm.buf[8:12])[0]
            ai_len = struct.unpack('<I', self.results_shm.buf[16:20])[0]

            print(f"🔍 测试读取结果:")
            print(f"   魔数: 0x{magic:08X} (期望: 0x41495254)")
            print(f"   语音长度: {voice_len}")
            print(f"   AI响应长度: {ai_len}")

            if magic == 0x41495254:
                print("✅ 共享内存测试成功")
                return True
            else:
                print("❌ 共享内存测试失败：魔数不匹配")
                return False

        except Exception as e:
            print(f"共享内存测试异常: {e}")
            return False

    def disconnect(self):
        """断开AI结果共享内存连接"""
        if self.results_shm:
            try:
                self.results_shm.close()
                self.connected = False
                print("✅ 已断开AI结果共享内存连接")
            except Exception as e:
                print(f"❌ 断开AI结果共享内存连接失败: {e}")
            finally:
                self.results_shm = None
                self.connected = False

    def __enter__(self):
        """支持with语句的上下文管理"""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """自动清理资源"""
        self.disconnect()
        if exc_type is not None:
            print(f"⚠️ AIResultsInterface异常退出: {exc_type.__name__}: {exc_val}")


# 模式控制共享内存接口
class ModeControlInterface:
    """模式控制共享内存接口"""

    def __init__(self):
        self.mode_control_shm = None
        self.connected = False
        self.current_mode = 'A'  # 默认模式
        self.last_check_time = 0
        self.detection_active = False  # 默认检测未激活

    def connect(self):
        """连接到模式控制共享内存"""
        try:
            print("🔍 正在尝试连接模式控制共享内存...")
            self.mode_control_shm = shared_memory.SharedMemory(name="ai_mode_control")
            self.connected = True
            print("✅ 成功连接到模式控制共享内存")
            return True
        except FileNotFoundError:
            print("⚠️  模式控制共享内存不存在 - mainwindow程序可能未运行")
            self.connected = False
            return False
        except Exception as e:
            print(f"❌ 连接模式控制共享内存失败: {e}")
            self.connected = False
            return False

    def check_mode_change(self):
        """检查模式是否发生变化"""
        if not self.connected or not self.mode_control_shm:
            return None, None

        try:
            # 读取共享内存数据
            data = self.mode_control_shm.buf

            # 检查魔数 (0x4D4F4445 = "MODE")
            magic = struct.unpack('<I', data[:4])[0]
            if magic != 0x4D4F4445:
                return None, None

            # 读取模式字符和更新标志
            mode_char = chr(data[4]) if data[4] != 0 else 'A'
            update_flag = data[5]

            # 读取检测控制指令 (字节6: 检测控制)
            detection_control = data[6] if len(data) > 6 else None

            mode_change = None
            detection_change = None

            # 如果有更新标志就触发（允许重复触发同一模式）
            if update_flag == 1:
                old_mode = self.current_mode
                self.current_mode = mode_char
                if mode_char != old_mode:
                    print(f"🔄 模式已从 {old_mode} 切换到 {mode_char}")
                else:
                    print(f"🔄 重复触发 {mode_char} 模式")
                mode_change = mode_char

                # 清除更新标志（告知mainwindow已处理）
                data[5] = 0

            # 检查检测控制变化
            if detection_control is not None:
                if detection_control == 1 and not getattr(self, 'detection_active', False):
                    # 启动检测
                    self.detection_active = True
                    detection_change = 'start'
                    print(f"🎯 收到启动目标检测指令")
                elif detection_control == 0 and getattr(self, 'detection_active', False):
                    # 停止检测
                    self.detection_active = False
                    detection_change = 'stop'
                    print(f"🛑 收到停止目标检测指令")

            return mode_change, detection_change

        except Exception as e:
            print(f"❌ 检查模式变化时出错: {e}")
            import traceback
            traceback.print_exc()

        return None, None

    def disconnect(self):
        """断开模式控制共享内存连接"""
        if self.mode_control_shm:
            try:
                self.mode_control_shm.close()
                self.connected = False
                print("已断开模式控制共享内存连接")
            except Exception as e:
                print(f"断开模式控制共享内存连接失败: {e}")


# 共享内存数据结构定义
class SharedMemoryInterface:
    """与主程序的共享内存接口"""

    def __init__(self):
        self.image_shm = None
        self.gaze_shm = None
        self.image_buffer = None
        self.gaze_buffer = None
        self.max_image_width = 1280  # 最大支持宽度
        self.max_image_height = 720  # 最大支持高度
        self.image_channels = 3
        # 实际图像尺寸（动态获取）
        self.actual_image_width = 1280
        self.actual_image_height = 720
        self.connected = False

        # 坐标转换功能 - 禁用，避免双重转换
        self.coordinate_conversion_enabled = False  # 禁用第一层转换
        self.screen_width = 2560  # 硬编码屏幕宽度
        self.screen_height = 1440  # 硬编码屏幕高度

        # 窗口偏移量（相对屏幕的位置）
        self.window_offset_x = 0  # 窗口左上角X坐标
        self.window_offset_y = 0  # 窗口左上角Y坐标

    def __del__(self):
        """析构函数，确保资源释放"""
        self.disconnect()

    def connect(self):
        """连接到共享内存"""
        print("🔍 正在尝试连接共享内存...")
        try:
            # 连接图像共享内存 (假设主程序创建名为"eyetracking_image"的共享内存)
            self.image_shm = shared_memory.SharedMemory(name="eyetracking_image")
            # 使用最大尺寸分配内存缓冲区
            max_image_size = self.max_image_width * self.max_image_height * self.image_channels
            print(f"   最大图像大小: {max_image_size} bytes, 实际共享内存大小: {len(self.image_shm.buf)} bytes")

            if len(self.image_shm.buf) < max_image_size:
                print(f"⚠️  图像共享内存大小不足，但尝试继续")

            # 创建用于接收图像尺寸信息的缓冲区
            self.image_buffer = None  # 将在获取实际尺寸后动态创建

            # 连接视线坐标共享内存 (假设主程序创建名为"eyetracking_gaze"的共享内存)
            self.gaze_shm = shared_memory.SharedMemory(name="eyetracking_gaze")
            print(f"   视线数据大小: {len(self.gaze_shm.buf)} bytes")
            # 视线数据结构: [gaze_x(float), gaze_y(float), valid(int), timestamp(float)]
            self.gaze_buffer = np.ndarray((4,), dtype=np.float32, buffer=self.gaze_shm.buf[:16])  # 4*4=16 bytes

            # 连接模式控制共享内存以获取图像尺寸信息
            try:
                self.mode_control_shm = shared_memory.SharedMemory(name="ai_mode_control")
                print("✅ 模式控制共享内存连接成功，可获取动态图像尺寸")
            except:
                print("⚠️  模式控制共享内存连接失败，使用默认尺寸")
                self.mode_control_shm = None

            self.connected = True
            print(f"✅ 成功连接到共享内存")
            print(f"   - 图像共享内存: eyetracking_image ({len(self.image_shm.buf)} bytes)")
            print(f"   - 视线共享内存: eyetracking_gaze ({len(self.gaze_shm.buf)} bytes)")

            # 立即测试一下数据
            test_frame = self.get_frame()
            if test_frame is not None:
                print(f"   ✅ 初始测试成功，图像形状: {test_frame.shape}")
            else:
                print(f"   ⚠️  初始测试失败，数据可能为空")

            # 直接获取屏幕分辨率
            self.get_screen_resolution()

            return True

        except FileNotFoundError:
            print("⚠️  共享内存不存在 - mainwindow眼动程序可能未运行")
            print("    程序将使用摄像头回退模式")
            print("💡 解决方案:")
            print("    1. 先启动 mainwindow.exe 程序")
            print("    2. 在mainwindow中激活摄像头功能")
            print("    3. 然后重新启动本程序以使用共享内存模式")
            print("    4. 或继续使用当前的摄像头独立模式")
            self.connected = False
            return False
        except PermissionError:
            print("❌ 共享内存访问权限不足")
            print("    请以管理员权限运行程序，或检查mainwindow程序状态")
            self.connected = False
            return False
        except Exception as e:
            print(f"❌ 连接共享内存失败: {e}")
            print("    程序将使用摄像头回退模式")
            print("🔄 程序将在运行期间定期检测共享内存连接...")
            self.connected = False
            return False

    def try_reconnect(self):
        """尝试重新连接共享内存（运行时调用）"""
        if self.connected:
            return True

        try:
            # 尝试重新连接
            self.image_shm = shared_memory.SharedMemory(name="eye_tracking_image")
            self.gaze_shm = shared_memory.SharedMemory(name="eye_tracking_gaze")
            self.results_shm = shared_memory.SharedMemory(name="ai_analysis_results")

            self.connected = True
            print("🔄 共享内存重新连接成功 - 已切换到共享内存模式")
            return True
        except:
            return False

    def get_frame(self):
        """从共享内存获取当前帧图像"""
        if not self.connected:
            return None

        try:
            # 动态获取图像尺寸信息（如果可用）
            self._update_image_dimensions()

            # 如果图像缓冲区尚未初始化，现在初始化
            if self.image_buffer is None:
                self._initialize_image_buffer()

            if self.image_buffer is None:
                return None

            # 🔧 关键修复：安全读取共享内存图像，避免与mainwindow的写入冲突
            # 使用原子操作复制数据，避免读取过程中数据损坏
            frame = self.image_buffer.copy()

            # 调试信息：每100帧输出一次
            if not hasattr(self, '_frame_counter'):
                self._frame_counter = 0
            self._frame_counter += 1

            # 每100帧输出一次简化的调试信息
            if self._frame_counter % 100 == 0:
                pixel_sum = np.sum(frame)
                if pixel_sum > 0:
                    print(f"✅ 帧 #{self._frame_counter}: 数据正常 (像素和={pixel_sum})")
                else:
                    print(f"⚠️  帧 #{self._frame_counter}: 数据为空")

            if np.sum(frame) == 0:
                # 初始的几帧显示详细信息，之后只在整百帧时显示
                if self._frame_counter <= 5 or self._frame_counter % 100 == 0:
                    if self._frame_counter <= 5:
                        print(f"⚠️  第{self._frame_counter}帧图像数据为空，等待mainwindow写入数据...")
                return None

            # 验证图像有效性
            expected_shape = (self.actual_image_height, self.actual_image_width, self.image_channels)
            if frame.shape != expected_shape:
                print(f"⚠️  图像形状异常: {frame.shape}, 预期: {expected_shape}")

            return frame
        except Exception as e:
            print(f"❌ 获取共享图像失败: {e}")
            return None

    def get_gaze_point(self):
        """从共享内存获取当前视线坐标"""
        if not self.connected or self.gaze_buffer is None:
            return None
        try:
            # 🔧 关键修复：安全读取共享内存，避免与mainwindow的写入冲突
            # 创建数据副本，避免在读取过程中数据被修改
            gaze_data_copy = self.gaze_buffer.copy()
            gaze_x, gaze_y, valid, timestamp = gaze_data_copy
            # 调试输出视线数据
            if hasattr(self, '_debug_counter'):
                self._debug_counter += 1
            else:
                self._debug_counter = 1

            # 每100帧输出一次调试信息
            if self._debug_counter % 100 == 0:
                print(f"🔍 共享内存原始数据: x={gaze_x:.1f}, y={gaze_y:.1f}, valid={valid}, timestamp={timestamp:.3f}")
                pass

            if valid > 0:  # 视线数据有效
                # 应用坐标转换
                if self.coordinate_conversion_enabled:
                    converted_x, converted_y = self.convert_screen_to_image_coords(gaze_x, gaze_y)
                    # 每100帧输出转换详情
                    if self._debug_counter % 100 == 0:
                        pass
                        print(
                            f"   屏幕分辨率: {self.screen_width}x{self.screen_height}, 图像分辨率: {self.image_width}x{self.image_height}")
                    return (converted_x, converted_y)
                else:
                    return (int(gaze_x), int(gaze_y))
            else:
                return None
        except Exception as e:
            print(f"❌ 获取视线坐标失败: {e}")
            self.reconnect()
            return None

    def convert_screen_to_image_coords(self, screen_x, screen_y):
        """
        将屏幕坐标转换为图像坐标
        考虑窗口边框和标题栏的影响

        Args:
            screen_x, screen_y: 屏幕坐标 (相对屏幕左上角)

        Returns:
            tuple: (image_x, image_y) 图像内坐标
        """
        # 根据实际测试数据调整边框和标题栏偏移
        # 实测数据显示需要额外的偏移：X+8, Y+30
        border_offset_x = 16  # 实际左边框偏移(8+8)
        title_bar_height = 60  # 实际标题栏偏移(30+30)

        # 图像坐标 = 屏幕坐标 - 固定窗口位置(490, 271) - 边框偏移
        image_x = int(screen_x - 490 - border_offset_x)
        image_y = int(screen_y - 271 - title_bar_height)

        # 不限制坐标范围，允许超出图像边界
        return image_x, image_y

    def get_screen_resolution(self):
        """
        直接获取屏幕分辨率
        """
        try:
            # 使用tkinter获取屏幕分辨率
            import tkinter as tk
            root = tk.Tk()
            root.withdraw()  # 隐藏窗口
            screen_width = root.winfo_screenwidth()
            screen_height = root.winfo_screenheight()
            root.destroy()

            # 使用系统获取的分辨率（已经考虑了缩放）
            self.screen_width = screen_width
            self.screen_height = screen_height
            print(f"🖥️  获取到屏幕分辨率: {self.screen_width}x{self.screen_height}")
            return True

        except ImportError:
            try:
                # 备选方案：使用autopy
                import autopy
                screen_width, screen_height = autopy.screen.size()
                # 强制使用正确的屏幕分辨率
                self.screen_width = 2560
                self.screen_height = 1441
                print(f"🖥️  强制设置屏幕分辨率: {self.screen_width}x{self.screen_height}")
                print(f"     (autopy获取值: {int(screen_width)}x{int(screen_height)})")
                return True
            except ImportError:
                print("⚠️  无法获取屏幕分辨率，使用默认值 1920x1080")
                return False
        except Exception as e:
            print(f"⚠️  获取屏幕分辨率失败: {e}，使用默认值")
            return False

    def set_window_position(self, x, y):
        """
        设置窗口位置偏移量
        用户可以手动校准窗口位置

        Args:
            x, y: 窗口左上角相对于屏幕的坐标
        """
        self.window_offset_x = x
        self.window_offset_y = y
        print(f"🎯 设置窗口偏移量: ({x}, {y})")

    def reconnect(self):
        """重新连接共享内存"""
        try:
            print("尝试重新连接共享内存...")
            self.disconnect()
            time.sleep(0.1)  # 短暂等待
            self.connect()
        except Exception as e:
            print(f"重连失败: {e}")
            self.connected = False

    def disconnect(self):
        """断开共享内存连接"""
        try:
            if self.image_shm:
                self.image_shm.close()
            if self.gaze_shm:
                self.gaze_shm.close()
            self.connected = False
            print("已断开共享内存连接")
        except Exception as e:
            print(f"断开共享内存连接失败: {e}")

    def _update_image_dimensions(self):
        """从模式控制共享内存获取当前图像尺寸"""
        if not hasattr(self, 'mode_control_shm') or self.mode_control_shm is None:
            return

        try:
            # 从模式控制共享内存读取图像尺寸信息
            # 根据mainwindow.cpp，尺寸信息存储在偏移12和16的位置
            width_bytes = self.mode_control_shm.buf[12:16]
            height_bytes = self.mode_control_shm.buf[16:20]

            width = int.from_bytes(width_bytes, byteorder='little')
            height = int.from_bytes(height_bytes, byteorder='little')

            # 验证尺寸合理性
            if 100 <= width <= self.max_image_width and 100 <= height <= self.max_image_height:
                if width != self.actual_image_width or height != self.actual_image_height:
                    self.actual_image_width = width
                    self.actual_image_height = height
                    # 需要重新初始化图像缓冲区
                    self.image_buffer = None
                    print(f"📏 图像尺寸更新: {width}x{height}")
        except:
            pass  # 静默失败，使用默认尺寸

    def _initialize_image_buffer(self):
        """根据当前图像尺寸初始化图像缓冲区"""
        if self.image_shm is None:
            return

        try:
            image_size = self.actual_image_width * self.actual_image_height * self.image_channels
            if image_size <= len(self.image_shm.buf):
                self.image_buffer = np.ndarray(
                    (self.actual_image_height, self.actual_image_width, self.image_channels),
                    dtype=np.uint8,
                    buffer=self.image_shm.buf[:image_size]
                )
                print(f"🖼️  图像缓冲区已初始化: {self.actual_image_width}x{self.actual_image_height}")
        except Exception as e:
            print(f"⚠️  图像缓冲区初始化失败: {e}")


# Global variables
background_init_completed = False


# Global State Management
class GlobalState:
    def __init__(self):
        self.lock = threading.Lock()
        self.response_queue = Queue()
        self.scroll_offset = 0
        self.display_text = ""
        self.last_response = ""
        self.analysis_thread = None
        self.last_thumbnail = None
        self.response_complete = True
        self.current_detected_objects = []
        self.current_objects_str = ""
        self.voice_recognition_text = ""
        self.circle_radius = CONFIG["circle_radius"]
        self.vad_enabled = CONFIG["vad_enabled"]
        self.analysis_active = False
        self.sending_status = ""
        self.partial_response = ""
        self.last_update_time = 0
        self.prompt = ""
        self.crop_img = None
        # 字体初始化，优先尝试系统中文字体
        self.font = None
        font_candidates = [
            # Windows系统字体
            "C:/Windows/Fonts/msyh.ttc",  # 微软雅黑
            "C:/Windows/Fonts/simhei.ttf",  # 黑体
            "C:/Windows/Fonts/simsun.ttc",  # 宋体
            "C:/Windows/Fonts/simkai.ttf",  # 楷体
            # 用户指定的字体
            CONFIG.get("font_path", ""),
            # Linux系统字体
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        ]

        for font_path in font_candidates:
            if font_path and os.path.exists(font_path):
                try:
                    self.font = ImageFont.truetype(font_path, CONFIG["font_size"])
                    print(f"Success: Font loaded: {font_path}")
                    break
                except Exception as e:
                    print(f"Warning: Font loading failed: {font_path} - {e}")
                    continue

        # 如果都失败，使用默认字体
        if self.font is None:
            try:
                self.font = ImageFont.load_default()
                print("Warning: Using PIL default font (may not support Chinese)")
            except:
                self.font = None
                print("Error: Font loading completely failed, will use OpenCV text rendering")
        # AI响应管理
        self.last_ai_response = ""
        self.model_ready = True
        self.voice_enabled = True

        # 智能目标跟踪优化
        self.last_mouse_pos = (0, 0)
        self.mouse_move_threshold = 25  # 鼠标移动阈值
        self.last_detection_time = 0
        self.detection_cooldown = 0.3  # 检测冷却时间（秒）
        self.frame_counter = 0

        # 跟踪相关
        self.tracked_target = None  # 当前跟踪的目标
        self.target_lost_frames = 0  # 目标丢失帧数
        self.max_lost_frames = 15  # 最大允许丢失帧数
        self.tracking_confidence_threshold = 0.3  # 跟踪置信度阈值


STATE = GlobalState()


# Asynchronous Resource Loading with Timing
def load_resources_async(callback):
    start_time = time.time()
    print("🚀 加载资源中...")  # 优化: 更友好的中文提示

    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as executor:  # 优化: 限制线程数
        if YOLO is not None:
            future_yolo = executor.submit(YOLO, CONFIG["model_path"])
        else:
            future_yolo = None

        if mp is not None:
            # 🔧 关键修复：使用static_image_mode=True避免MediaPipe尝试访问摄像头
            future_mp = executor.submit(mp.solutions.hands.Hands,
                                        static_image_mode=True,  # 静态图像模式，避免摄像头冲突
                                        max_num_hands=1,
                                        model_complexity=0,
                                        min_detection_confidence=0.7,
                                        min_tracking_confidence=0.7)
        else:
            future_mp = None

        if pyaudio is not None:
            future_pyaudio = executor.submit(pyaudio.PyAudio)
        else:
            future_pyaudio = None

        if future_yolo is not None:
            model = future_yolo.result()
            if torch is not None and torch.cuda.is_available():
                model = model.to("cuda")
            else:
                model = model.to("cpu")
            print(f"YOLO loaded in {time.time() - start_time:.2f}s")
        else:
            model = None
            print("YOLO模型不可用")

        if future_mp is not None:
            hands = future_mp.result()
        else:
            hands = None
            print("MediaPipe不可用")
        if hands is not None:
            print(f"MediaPipe Hands loaded in {time.time() - start_time:.2f}s")

        if future_pyaudio is not None:
            audio_interface = future_pyaudio.result()
            print(f"PyAudio loaded in {time.time() - start_time:.2f}s")
        else:
            audio_interface = None
            print("PyAudio不可用")

        if dashscope is not None:
            dashscope.api_key = CONFIG["dashscope_api_key"]

        global client
        if OpenAI is not None:
            client = OpenAI(
                base_url="https://dashscope.aliyuncs.com/compatible-mode/v1",
                api_key=CONFIG["dashscope_api_key"],
                timeout=30.0
            )
        else:
            client = None
            print("OpenAI客户端不可用")

    print(f"[OK] 资源加载完成，耗时: {time.time() - start_time:.2f}s")

    callback(model, hands, audio_interface, client)


# MVC-like Structure
# Model Layer
class ObjectDetector:
    def __init__(self, model):
        self.model = model

    def detect_objects(self, frame):
        """优化的目标检测"""
        if self.model is None:
            print("❌ YOLO模型未加载，跳过检测")
            return None

        try:
            # 输入验证
            if frame is None or frame.size == 0:
                print("❌ 检测帧为空")
                return None

            # 优化: 动态调整图像尺寸和置信度
            img_size = 416 if frame.shape[0] < 720 else 640  # 小图用小模型
            results = self.model.predict(
                frame,
                imgsz=img_size,
                conf=0.5,  # 添加置信度阈值
                verbose=False
            )

            # 验证结果
            if results and len(results) > 0 and hasattr(results[0], 'boxes'):
                return results[0]
            else:
                return None

        except Exception as e:
            print(f"❌ 目标检测异常: {str(e)}")
            import traceback
            traceback.print_exc()
            return None

    def detect_rect_region(self, frame, center_pos, radius):
        """检测鼠标矩形区域（扩大2倍）的目标"""
        try:
            h, w = frame.shape[:2]
            cx, cy = center_pos

            # 计算矩形区域（扩大2倍）
            region_size = radius * 2  # 2倍扩大
            x1 = max(0, cx - region_size)
            y1 = max(0, cy - region_size)
            x2 = min(w, cx + region_size)
            y2 = min(h, cy + region_size)

            # 裁剪矩形区域
            region_crop = frame[int(y1):int(y2), int(x1):int(x2)]

            if region_crop.size == 0:
                return None

            # 对裁剪区域进行检测
            # 优化: 区域检测使用更小模型
            results = self.model.predict(
                region_crop,
                conf=0.4,  # 适中的置信度
                imgsz=320,  # 更小的模型尺寸
                device="cuda" if torch.cuda.is_available() else "cpu",
                verbose=False,
                half=True if torch.cuda.is_available() else False
            )

            if not results or not results[0].boxes:
                return None

            # 转换坐标到原图坐标系
            region_objects = []
            for obj in results[0].boxes:
                # 裁剪区域中的坐标
                crop_bbox = obj.xyxy[0].tolist()

                # 转换到原图坐标
                original_bbox = [
                    crop_bbox[0] + x1,
                    crop_bbox[1] + y1,
                    crop_bbox[2] + x1,
                    crop_bbox[3] + y1
                ]

                # 计算距离鼠标中心的距离
                center_distance = self._calculate_distance_to_center(original_bbox, center_pos)

                region_objects.append({
                    "class": results[0].names[int(obj.cls)],
                    "conf": float(obj.conf),
                    "bbox": original_bbox,
                    "center_distance": center_distance
                })

            # 按距离鼠标中心的远近排序
            region_objects.sort(key=lambda x: x["center_distance"])

            print(f"检测矩形区域: ({int(x1)},{int(y1)})-({int(x2)},{int(y2)}), 尺寸: {int(x2 - x1)}x{int(y2 - y1)}")

            return region_objects

        except Exception as e:
            print(f"区域检测异常: {str(e)}")
            return None

    def _is_bbox_in_circle(self, bbox, center_pos, radius):
        """判断检测框是否与圆形区域相交（放宽条件）"""
        x1, y1, x2, y2 = bbox
        cx, cy = center_pos

        # 放宽半径以及更多目标
        expanded_radius = radius * 1.2

        # 检查检测框中心点是否在放大的圆内
        bbox_center_x = (x1 + x2) / 2
        bbox_center_y = (y1 + y2) / 2
        center_distance = math.sqrt((bbox_center_x - cx) ** 2 + (bbox_center_y - cy) ** 2)

        if center_distance <= expanded_radius:
            return True

        # 检查检测框的四个角点是否在圆内
        corners = [(x1, y1), (x2, y1), (x1, y2), (x2, y2)]
        for corner_x, corner_y in corners:
            if math.sqrt((corner_x - cx) ** 2 + (corner_y - cy) ** 2) <= expanded_radius:
                return True

        # 检查圆心是否在检测框内
        if x1 <= cx <= x2 and y1 <= cy <= y2:
            return True

        return False

    def _calculate_distance_to_center(self, bbox, center_pos):
        """计算检测框中心到鼠标位置的距离"""
        bbox_center_x = (bbox[0] + bbox[2]) / 2
        bbox_center_y = (bbox[1] + bbox[3]) / 2
        return math.sqrt((bbox_center_x - center_pos[0]) ** 2 +
                         (bbox_center_y - center_pos[1]) ** 2)

    def select_closest_target(self, detected_objects, mouse_pos):
        """选择距离鼠标最近的目标作为主要跟踪目标"""
        if not detected_objects:
            return None

        # 返回距离最近的目标
        closest_target = min(detected_objects, key=lambda obj: obj["center_distance"])

        # 添加目标中心坐标用于跟踪
        closest_target["center"] = (
            (closest_target["bbox"][0] + closest_target["bbox"][2]) / 2,
            (closest_target["bbox"][1] + closest_target["bbox"][3]) / 2
        )

        return closest_target

    def is_same_target(self, target1, target2, position_threshold=50, class_match=True):
        """判断两个目标是否为同一个目标"""
        if not target1 or not target2:
            return False

        # 类别必须匹配
        if class_match and target1["class"] != target2["class"]:
            return False

        # 位置距离判断
        center1 = target1.get("center", ((target1["bbox"][0] + target1["bbox"][2]) / 2,
                                         (target1["bbox"][1] + target1["bbox"][3]) / 2))
        center2 = target2.get("center", ((target2["bbox"][0] + target2["bbox"][2]) / 2,
                                         (target2["bbox"][1] + target2["bbox"][3]) / 2))

        distance = math.sqrt((center1[0] - center2[0]) ** 2 + (center1[1] - center2[1]) ** 2)
        return distance < position_threshold

    def get_focused_object(self, detected_objects, gaze_point):
        if not detected_objects or not gaze_point:
            return None
        for obj in detected_objects:
            x1, y1, x2, y2 = map(int, obj["bbox"])
            if x1 <= gaze_point[0] <= x2 and y1 <= gaze_point[1] <= y2:
                return obj
        return None


# 已删除SmartTargetTracker类 - 目标跟踪功能实现有误，简化为基础目标检测


class IntentPredictor:
    MODE_CONFIG = {
        "A": {
            "name": "焦点分析模式",
            "prompt": """分析焦点物体（100字以内）：
            - 类别：{focus_class}({focus_conf:.1%})
            - 位置：[{x1:.0f},{y1:.0f}]到[{x2:.0f},{y2:.0f}]
            请用一段话阐述上述内容，不要分点"""
        },
        "S": {
            "name": "危险检测模式",
            "prompt": """请结合当前环境进行安全评估（100字以内）：
                    - 检测到的物体列表：{obj_list}
                    - 当前焦点物体：{focus_info}
                    分析图像中的危险行为"""
        },
        "D": {
            "name": "意图推理模式",
            "prompt": """基于以下情境进行意图推理(100字以内)：
                    - 环境构成：{obj_list}
                    - 当前焦点：{focus_info}
                    推测可能的用户意图：1. 主要意图（概率>70%）2. 次要意图（概率30%-70%）3. 潜在意图（概率<30%）"""
        },
        "F": {
            "name": "详细描述模式",
            "prompt": """请简要描述当前场景（100字以内）：
                    描述请控制在100字以内。"""
        }
    }

    def __init__(self):
        self.active_mode = "A"
        self.mode_history = []
        self.analysis_history = []

    def switch_mode(self, new_mode):
        if new_mode in self.MODE_CONFIG:
            self.mode_history.append(self.active_mode)
            self.active_mode = new_mode
            print(f"已切换到：{self.MODE_CONFIG[new_mode]['name']}")

    def generate_prompt(self, detected_objects, gaze_point, user_input=None):
        config = self.MODE_CONFIG[self.active_mode]
        focused_obj = self.get_focused_object(detected_objects, gaze_point)

        obj_list = [f"{obj['class']}({obj['conf']:.1%})" for obj in detected_objects]
        focus_info = "无" if not focused_obj else (
            f"{focused_obj['class']}({focused_obj['conf']:.1%}) "
            f"@[{focused_obj['bbox'][0]:.0f},{focused_obj['bbox'][1]:.0f}]"
        )

        # 如果有用户语音输入，优先使用
        if user_input:
            base_prompt = f"用户语音输入: {user_input}\n当前环境物体列表: {', '.join(obj_list)}"
            return base_prompt
        else:
            # 没有语音输入时使用原有提示词
            return config["prompt"].format(
                obj_list=", ".join(obj_list),
                focus_info=focus_info,
                history=" | ".join(self.analysis_history[-3:]),
                focus_class=focused_obj["class"] if focused_obj else "无",
                focus_conf=focused_obj["conf"] if focused_obj else 0,
                x1=focused_obj["bbox"][0] if focused_obj else 0,
                y1=focused_obj["bbox"][1] if focused_obj else 0,
                x2=focused_obj["bbox"][2] if focused_obj else 0,
                y2=focused_obj["bbox"][3] if focused_obj else 0
            )

    def get_focused_object(self, detected_objects, gaze_point):
        """Get the focused object based on gaze point"""
        if not detected_objects or not gaze_point:
            return None
        for obj in detected_objects:
            x1, y1, x2, y2 = map(int, obj["bbox"])
            if x1 <= gaze_point[0] <= x2 and y1 <= gaze_point[1] <= y2:
                return obj
        return None


# View Layer (UI Rendering) - 完全按照main06.py的方式实现文本框
def add_border_with_text(frame, text, scroll_offset=0):
    """完全按照main06.py实现文本框"""
    border_size = 200
    h, w = frame.shape[:2]

    bordered_frame = cv2.copyMakeBorder(
        frame, 0, border_size, 0, 0,
        cv2.BORDER_CONSTANT,
        value=[255, 255, 255]
    )

    pil_img = Image.fromarray(cv2.cvtColor(bordered_frame, cv2.COLOR_BGR2RGB))
    draw = ImageDraw.Draw(pil_img)

    # 使用全局字体变量
    global font

    lines = []
    for para in text.split('\n'):
        if not para.strip():
            lines.append('')
            continue
        wrapped_lines = wrap(para, width=50)
        lines.extend(wrapped_lines)

    # 使用与main06.py一致的常量
    LINE_SPACING = 3
    FONT_SIZE = 25
    max_visible_lines = (border_size - 20) // (FONT_SIZE + LINE_SPACING)
    start_line = max(0, min(scroll_offset, len(lines) - max_visible_lines))
    end_line = min(start_line + max_visible_lines, len(lines))

    if lines:
        bg_height = (end_line - start_line) * (FONT_SIZE + LINE_SPACING) + 20
        bg = Image.new('RGBA', (w, bg_height), (255, 255, 255, 200))
        pil_img.paste(bg, (0, h), bg)

    for i, line in enumerate(lines[start_line:end_line]):
        y_pos = h + 10 + i * (FONT_SIZE + LINE_SPACING)
        draw.text((20, y_pos), line, font=font, fill=(0, 0, 0))

    if len(lines) > max_visible_lines:
        scroll_info = f"↑↓ 查看完整内容 ({start_line + 1}-{end_line}/{len(lines)})"
        if font:
            bbox = font.getbbox(scroll_info)
            sw = bbox[2] - bbox[0]
            sh = bbox[3] - bbox[1]
            draw.text((w - sw - 20, h + border_size - sh - 10),
                      scroll_info, font=font, fill=(100, 100, 100))

    return cv2.cvtColor(np.array(pil_img), cv2.COLOR_RGB2BGR)


def create_circular_crop(frame, center, radius):
    mask = np.zeros(frame.shape[:2], dtype=np.uint8)
    cv2.circle(mask, center, radius, 255, -1)
    masked_frame = cv2.bitwise_and(frame, frame, mask=mask)
    x, y = center
    x1 = max(0, x - radius)
    y1 = max(0, y - radius)
    x2 = min(frame.shape[1], x + radius)
    y2 = min(frame.shape[0], y + radius)
    if x2 > x1 and y2 > y1:
        cropped = masked_frame[y1:y2, x1:x2]
        result = np.zeros((2 * radius, 2 * radius, 3), dtype=np.uint8)
        offset_x = radius - (x - x1)
        offset_y = radius - (y - y1)
        crop_h, crop_w = cropped.shape[:2]
        start_x = max(0, offset_x - crop_w // 2)
        start_y = max(0, offset_y - crop_h // 2)
        end_x = min(2 * radius, start_x + crop_w)
        end_y = min(2 * radius, start_y + crop_h)
        if end_y - start_y == crop_h and end_x - start_x == crop_w:
            result[start_y:end_y, start_x:end_x] = cropped
        return result
    return frame.copy()


# Controller Layer
class HandDetector:
    def __init__(self, hands):
        self.hands = hands
        self.mpDraw = mp.solutions.drawing_utils
        self.tipIds = [4, 8, 12, 16, 20]
        self.results = None
        self.lmList = []

    def findHands(self, img, draw=True):
        imgRGB = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        self.results = self.hands.process(imgRGB)
        if self.results.multi_hand_landmarks:
            for handLms in self.results.multi_hand_landmarks:
                if draw:
                    self.mpDraw.draw_landmarks(img, handLms, mp.solutions.hands.HAND_CONNECTIONS)
        return img

    def findPosition(self, img, handNo=0, draw=True):
        self.lmList = []
        if self.results.multi_hand_landmarks:
            myHand = self.results.multi_hand_landmarks[handNo]
            for id, lm in enumerate(myHand.landmark):
                h, w, c = img.shape
                cx, cy = int(lm.x * w), int(lm.y * h)
                self.lmList.append([id, cx, cy])
                if draw:
                    cv2.circle(img, (cx, cy), 5, (0, 0, 255), cv2.FILLED)
        return self.lmList

    def fingersUp(self):
        fingers = []
        if len(self.lmList) == 0:
            return fingers
        if self.lmList[self.tipIds[0]][1] > self.lmList[self.tipIds[0] - 1][1]:
            fingers.append(1)
        else:
            fingers.append(0)
        for id in range(1, 5):
            if self.lmList[self.tipIds[id]][2] < self.lmList[self.tipIds[id] - 2][2]:
                fingers.append(1)
            else:
                fingers.append(0)
        return fingers

    def isFingersClosed(self, p1, p2, img, threshold=55):
        if len(self.lmList) == 0:
            return False
        length, _, _ = self.findDistance(p1, p2, img, draw=False)
        return length < threshold

    def findDistance(self, p1, p2, img, draw=True, r=15, t=3):
        x1, y1 = self.lmList[p1][1:]
        x2, y2 = self.lmList[p2][1:]
        cx, cy = (x1 + x2) // 2, (y1 + y2) // 2
        if draw:
            cv2.line(img, (x1, y1), (x2, y2), (255, 0, 255), t)
            cv2.circle(img, (x1, y1), r, (255, 0, 255), cv2.FILLED)
            cv2.circle(img, (x2, y2), r, (255, 0, 255), cv2.FILLED)
            cv2.circle(img, (cx, cy), r, (0, 0, 255), cv2.FILLED)
        length = math.hypot(x2 - x1, y2 - y1)
        return length, img, [x1, y1, x2, y2, cx, cy]


class DataCollector:
    """数据收集器 - 从共享内存获取图像和视线数据"""

    def __init__(self, window_name, hand_detector):
        self.shared_memory = SharedMemoryInterface()
        self.gaze_pos = (640, 360)  # 当前视线位置
        self.hand_detector = hand_detector
        self.plocX, self.plocY = 0, 0
        self.use_shared_memory = False
        self.mouse_callback_pending = False  # 标志位：是否需要设置鼠标回调
        self.waiting_for_connection = False  # 是否正在等待连接
        self.connection_retry_count = 0  # 连接重试次数
        self.last_connection_attempt = 0  # 上次连接尝试时间
        self.need_reconnect_mode_control = False  # 是否需要重新连接模式控制

        print("🔄 正在初始化数据收集器...")

        # ⚠️ 关键修复：延迟启动避免与mainwindow的内存操作冲突
        print("⏳ 延迟5秒以避免与mainwindow的内存冲突...")
        time.sleep(5.0)

        # 初始化时不立即连接共享内存，等待后台资源初始化完成
        self.use_shared_memory = False
        print("🔄 数据收集器初始化完成，等待后台资源加载...")

        # 暂时不初始化摄像头，等待后台资源完成后再决定模式

    def setup_connection_after_init(self):
        """在后台资源初始化完成后设置连接"""
        print("🔄 开始设置数据收集连接...")

        # 尝试连接共享内存
        if self.shared_memory.connect():
            self.use_shared_memory = True
            print("✅ Eye-tracking共享内存模式已启用")
            return

        # 检查是否强制只使用共享内存模式
        if CONFIG.get("force_shared_memory_only", False):
            print("❌ 强制共享内存模式已启用，但连接失败")
            print("⏳ 程序将等待 mainwindow 启动并打开摄像头...")
            print("💡 请按以下步骤操作:")
            print("    1. 启动 mainwindow.exe 程序")
            print("    2. 在 mainwindow 中激活摄像头功能")
            print("    3. 本程序将自动检测并连接")

            # 进入阻塞等待循环，直到连接成功
            self.wait_for_mainwindow_connection()
        else:
            # 回退到摄像头模式
            self.use_shared_memory = False
            print("🎥 共享内存不可用，启用摄像头回退模式")
            self.init_camera_fallback()

    def wait_for_mainwindow_connection(self):
        """等待mainwindow启动并建立共享内存连接 - 连接成功后立即退出"""
        self.waiting_for_connection = True
        retry_count = 0
        max_retries = -1  # 无限重试

        print("[INFO] 开始等待 mainwindow 连接...")
        print("[INFO] 按 Ctrl+C 可以退出等待")

        try:
            while self.waiting_for_connection:
                retry_count += 1

                # 每10次重试显示一次提示
                if retry_count % 10 == 1:
                    print(f"[INFO] 尝试连接 mainwindow... (第 {retry_count} 次)")

                # 尝试连接
                if self.shared_memory.connect():
                    self.use_shared_memory = True
                    self.waiting_for_connection = False
                    # 确保所有相关状态正确重置，与直接连接时保持完全一致
                    self.mouse_callback_pending = False
                    self.connection_retry_count = 0
                    self.last_connection_attempt = 0
                    # 清理任何可能的摄像头资源
                    if hasattr(self, 'cap') and self.cap is not None:
                        self.cap.release()
                        self.cap = None
                    print("🎉 成功连接到 mainwindow 共享内存!")
                    print("✅ Eye-tracking共享内存模式已启用")

                    # 🔥 关键修复：设置标志，需要重新连接模式控制
                    self.need_reconnect_mode_control = True
                    return

                # 等待2秒后重试
                time.sleep(2.0)

        except KeyboardInterrupt:
            print("\n[INFO] 用户中断等待")
            print("💡 程序将尝试启动摄像头回退模式...")
            self.waiting_for_connection = False

            # 如果不是强制模式，尝试摄像头回退
            if not CONFIG.get("force_shared_memory_only", False):
                self.use_shared_memory = False
                self.init_camera_fallback()
            else:
                raise RuntimeError("强制共享内存模式下用户中断")

    def init_camera_fallback(self):
        """初始化摄像头回退模式"""
        print("🎥 启动摄像头回退模式...")
        if cv2 is not None:
            self.cap = cv2.VideoCapture(0)
            if self.cap.isOpened():
                self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
                self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
                self.mouse_callback_pending = True
                print("✅ 摄像头回退模式已就绪")
            else:
                raise RuntimeError("摄像头初始化失败")
        else:
            raise RuntimeError("OpenCV不可用")

    def handle_mouse_events(self, event, x, y, flags, param):
        """鼠标事件处理（仅在回退模式下使用）"""
        global scroll_offset, circle_radius
        if event == cv2.EVENT_MOUSEMOVE:
            if not self.use_shared_memory:  # 只有在非共享内存模式下才使用鼠标
                self.gaze_pos = (x, y)
        elif event == cv2.EVENT_MOUSEWHEEL:
            global circle_radius, scroll_offset
            scroll_delta = -1 if flags > 0 else 1
            # 通过滚轮调整圆圈大小
            circle_radius = max(20, min(300, circle_radius - (scroll_delta * 5)))
            # 保留原有的文本滚动功能
            scroll_offset = max(0, scroll_offset + scroll_delta)

    def get_frame(self):
        """获取当前帧图像"""

        if self.use_shared_memory:
            # 从共享内存获取图像

            frame = self.shared_memory.get_frame()
            if frame is None:
                return None  # 共享内存模式下如果获取失败，直接返回None，不切换到摄像头
        else:
            # 从摄像头获取图像
            ret, frame = self.cap.read()
            if not ret:
                print("❌ 摄像头帧获取失败！检查摄像头连接或索引。")
                return None

        # 手势识别（如果需要的话）
        if self.hand_detector:
            frame = self.hand_detector.findHands(frame)
            lmList = self.hand_detector.findPosition(frame, draw=False)

            if len(lmList) != 0:
                fingers = self.hand_detector.fingersUp()
                is_closed = self.hand_detector.isFingersClosed(4, 8, frame)
                if is_closed and fingers[3] == 1 and fingers[4] == 1:
                    autopy.key.tap(autopy.key.Code.SPACE)

        return frame

    def get_gaze_position(self):
        """获取当前视线位置（图像坐标）"""
        if self.use_shared_memory:
            # 从共享内存获取屏幕坐标
            screen_gaze_pos = self.shared_memory.get_gaze_point()
            if screen_gaze_pos:
                # 使用窗口的实际位置进行坐标转换
                window_x = getattr(self.shared_memory, 'window_offset_x', 0)
                window_y = getattr(self.shared_memory, 'window_offset_y', 0)

                # 手动校准模式：请调整这两个数值直到红圆圈与T键重合
                MANUAL_OFFSET_X = 490  # 手动调整这个数值
                MANUAL_OFFSET_Y = 271  # 手动调整这个数值

                gaze_x = screen_gaze_pos[0]
                gaze_y = screen_gaze_pos[1]

                # 不限制坐标范围，让调试信息显示真实坐标
                image_gaze_pos = (int(gaze_x), int(gaze_y))
                self.gaze_pos = image_gaze_pos
                return image_gaze_pos
            else:
                # 如果视线数据无效，返回上一次有效位置
                return self.gaze_pos
        else:
            # 回退模式：使用鼠标位置
            return self.gaze_pos

    def cleanup(self):
        """清理资源"""
        if self.use_shared_memory:
            self.shared_memory.disconnect()
        elif hasattr(self, 'cap'):
            self.cap.release()


class VoiceRecognitionCallback(RecognitionCallback):
    def __init__(self, voice_assistant):
        super().__init__()
        self.voice_assistant = voice_assistant
        self.recognized_text = ""
        self.full_transcript = ""
        self.is_final = False
        self.has_triggered = False

    def on_open(self) -> None:
        self.voice_assistant.voice_status = "语音识别会话已打开"
        print("语音识别会话已打开")

    def on_close(self) -> None:
        global voice_recognition_text
        self.voice_assistant.voice_status = "语音识别会话已关闭"
        print("语音识别会话已关闭")
        voice_recognition_text = self.full_transcript
        self.voice_assistant.last_voice_response = self.full_transcript

        # 写入语音识别结果到共享内存（检查程序是否仍在运行）
        if 'ai_results' in globals() and ai_results and ai_results.connected and self.voice_assistant.is_listening:
            try:
                ai_results.write_results(voice_text=self.full_transcript, ai_response=last_response)
                print(f"✅ 语音识别结果已写入共享内存: {self.full_transcript[:50]}...")
            except Exception as e:
                print(f"写入语音识别结果失败（程序可能正在关闭）: {e}")

        if not self.has_triggered and len(self.full_transcript.strip()) > 2:
            self.has_triggered = True
            print(f'语音识别完成："{self.full_transcript}"，准备触发分析')
            self.voice_assistant.trigger_visual_analysis = True

    def on_complete(self) -> None:
        pass

    def on_error(self, message) -> None:
        self.voice_assistant.voice_status = f"语音识别错误: {message.message}"
        print(f"语音识别错误: {message.message}")

    def on_event(self, result: RecognitionResult) -> None:
        sentence = result.get_sentence()
        if 'text' in sentence:
            text = sentence['text']
            if RecognitionResult.is_sentence_end(sentence):
                self.full_transcript += text + " "
                self.recognized_text = self.full_transcript
                self.is_final = True
                # 【删除重复触发】这里不再触发分析，避免与on_partial中的触发重复
            else:
                self.recognized_text = self.full_transcript + text
            print(f"识别结果: {self.recognized_text}")
            self.voice_assistant.last_voice_response = self.recognized_text
            global voice_recognition_text
            voice_recognition_text = self.recognized_text

            # 写入语音识别结果到共享内存（检查程序是否仍在运行）
            if 'ai_results' in globals() and ai_results and ai_results.connected and self.voice_assistant.is_listening:
                try:
                    ai_results.write_results(voice_text=self.recognized_text, ai_response=last_response)
                    print(f"✅ 实时语音结果已写入共享内存: {self.recognized_text[:30]}...")
                except Exception as e:
                    print(f"写入实时语音结果失败（程序可能正在关闭）: {e}")


class VoiceAssistant:
    def __init__(self, audio_interface):
        self.voice_status = "就绪"
        self.last_voice_response = ""
        self.initialized = True
        self.vad = webrtcvad.Vad()
        self.vad.set_mode(2)  # 优化: 提高敏感度到2，更快检测语音
        self.pre_buffer = collections.deque(maxlen=16)  # 优化: 减少预缓冲区大小，降低延迟
        self.vad_thread = None
        self.is_listening = False
        self.recognizer = None
        self.callback = None
        self.trigger_visual_analysis = False
        self.audio_interface = audio_interface
        self.conversation_history = [
            {
                "role": "system",
                "content": [{"type": "text", "text": "你是一个音频助手。回答应当简洁直接，不要重复用户已知的信息。"}],
            }
        ]

    def start_vad_listener(self):
        if self.vad_thread and self.vad_thread.is_alive():
            return
        self.is_listening = True
        self.vad_thread = threading.Thread(target=self.vad_listener, daemon=True)
        self.vad_thread.start()
        self.voice_status = "VAD监听中..."

    def stop_vad_listener(self):
        self.is_listening = False
        if self.vad_thread and self.vad_thread.is_alive():
            self.vad_thread.join(timeout=1.0)
        self.voice_status = "VAD已停止"

    def force_stop_all(self):
        """强制停止所有语音相关操作"""
        print("🛑 强制停止所有语音操作...")

        # 停止VAD监听
        self.is_listening = False

        # 停止当前识别器
        if hasattr(self, 'recognizer') and self.recognizer:
            try:
                print("停止语音识别器...")
                self.recognizer.stop()
                # 清理识别器
                if hasattr(self.recognizer, '_cleanup'):
                    self.recognizer._cleanup()
                self.recognizer = None
                print("✅ 语音识别器已停止")
            except Exception as e:
                print(f"停止语音识别器时出错（可忽略）: {e}")

        # 停止VAD线程
        if self.vad_thread and self.vad_thread.is_alive():
            print("等待VAD线程结束...")
            self.vad_thread.join(timeout=2.0)  # 增加超时时间
            if self.vad_thread.is_alive():
                print("⚠️ VAD线程未能在2秒内结束")
            else:
                print("✅ VAD线程已结束")

        # 清理音频接口
        if hasattr(self, 'audio_interface') and self.audio_interface:
            try:
                print("终止音频接口...")
                self.audio_interface.terminate()
                print("✅ 音频接口已终止")
            except Exception as e:
                print(f"终止音频接口时出错（可忽略）: {e}")

        self.voice_status = "已完全停止"
        print("🔇 所有语音操作已停止")

    def toggle_vad(self):
        global vad_enabled
        if vad_enabled:
            self.stop_vad_listener()
            vad_enabled = False
            self.voice_status = "VAD已关闭"
        else:
            self.start_vad_listener()
            vad_enabled = True
            self.voice_status = "VAD监听中..."
        return vad_enabled

    def vad_listener(self):
        try:
            # 缓存配置参数避免重复计算
            vad_config = CONFIG["vad_config"]["audio"]
            frame_size = int(vad_config["rate"] * vad_config["frame_duration"] / 1000)
            silence_frames = int(vad_config["silence_timeout"] * vad_config["rate"] / frame_size)
            speech_threshold_frames = int(vad_config["speech_threshold"] * vad_config["rate"] / frame_size)
            max_record_frames = int(vad_config["max_record_seconds"] * vad_config["rate"] / frame_size)
            noise_threshold = vad_config["noise_threshold"]
            sample_rate = vad_config["rate"]

            stream = self.audio_interface.open(
                format=vad_config["format"],
                channels=vad_config["channels"],
                rate=sample_rate,
                input=True,
                frames_per_buffer=frame_size,
            )
            stream.start_stream()

            INIT_SILENCE_DURATION = 1.0
            init_silence_frames = int(INIT_SILENCE_DURATION * sample_rate / frame_size)
            for _ in range(init_silence_frames):
                stream.read(frame_size, exception_on_overflow=False)
            self.pre_buffer.clear()

            state = "SILENCE"
            record_frames = []
            speech_counter = 0
            silence_counter = 0
            frame_count = 0

            self.voice_status = "VAD监听中..."
            print("VAD启动...")

            while self.is_listening:
                try:
                    frame = stream.read(frame_size, exception_on_overflow=False)
                except IOError as e:
                    if e.errno == pyaudio.paInputOverflowed:
                        continue
                    raise

                # 优化：减少numpy计算开销
                audio_data = np.frombuffer(frame, dtype=np.int16)
                rms = np.sqrt(np.mean(np.square(audio_data, dtype=np.float32))) if len(audio_data) > 0 else 0

                # 优化：先做简单的RMS检查，再做VAD
                if rms < noise_threshold:
                    is_speech = False
                else:
                    is_speech = self.vad.is_speech(frame, sample_rate)

                if state == "SILENCE":
                    self.pre_buffer.append(frame)

                    if is_speech:
                        speech_counter += 1
                        if speech_counter >= speech_threshold_frames:
                            self.voice_status = "录音中..."
                            state = "SPEECH"
                            record_frames = list(self.pre_buffer)
                            record_frames.append(frame)
                            silence_counter = 0
                            frame_count = len(record_frames)
                    else:
                        speech_counter = 0

                elif state == "SPEECH":
                    record_frames.append(frame)
                    frame_count += 1

                    if frame_count >= max_record_frames:
                        self.voice_status = "达到最大录音时长，停止录音..."
                        self.process_audio_stream(record_frames)
                        state = "SILENCE"
                        record_frames = []
                        speech_counter = 0
                        silence_counter = 0
                        self.pre_buffer.clear()
                        continue

                    if is_speech:
                        silence_counter = 0
                    else:
                        silence_counter += 1
                        if silence_counter >= silence_frames:
                            self.voice_status = "处理中..."
                            # 优化: 立即开始处理，不等待
                            self.process_audio_stream(record_frames)
                            state = "SILENCE"
                            record_frames = []
                            speech_counter = 0
                            silence_counter = 0
                            self.pre_buffer.clear()
                            self.voice_status = "就绪"  # 快速回到就绪状态

        except Exception as e:
            self.voice_status = f"VAD错误: {str(e)}"
            print(f"VAD监听异常: {str(e)}")
        finally:
            if stream.is_active():
                stream.stop_stream()
            stream.close()
            self.voice_status = "VAD已停止"

    def process_audio_stream(self, record_frames):
        try:
            min_speech_duration = 0.3  # 优化: 从0.5s减少到0.3s，更快触发识别
            recorded_duration = len(record_frames) * CONFIG["vad_config"]["audio"]["frame_duration"] / 1000.0

            if recorded_duration >= min_speech_duration:
                # 优化: 使用线程池异步处理音频，避免阻塞VAD
                audio_data = b''.join(record_frames)
                # 在单独线程中处理，避免阻塞VAD监听
                threading.Thread(target=self._async_audio_processing, args=(audio_data,), daemon=True).start()
            else:
                self.voice_status = "录音过短，已忽略"

        except Exception as e:
            self.voice_status = f"音频处理错误: {str(e)}"

    def _async_audio_processing(self, audio_data):
        """异步处理音频数据，避免阻塞VAD"""
        try:
            self.send_to_ai_stream(audio_data)
        except Exception as e:
            self.voice_status = f"异步音频处理错误: {str(e)}"
            print(f"处理音频异常: {str(e)}")

    def send_to_ai_stream(self, audio_data):
        try:
            # 检查是否正在关闭
            if not self.is_listening:
                print("程序正在关闭，跳过语音识别")
                return

            self.voice_status = "语音识别中..."
            self.callback = VoiceRecognitionCallback(self)
            self.recognizer = Recognition(
                model='paraformer-realtime-v2',
                format='pcm',
                sample_rate=16000,
                callback=self.callback
            )
            self.recognizer.start()

            chunk_size = 3200
            for i in range(0, len(audio_data), chunk_size):
                # 再次检查是否正在关闭
                if not self.is_listening:
                    print("检测到程序关闭，停止音频发送")
                    break

                data = audio_data[i:i + chunk_size]
                if data:
                    self.recognizer.send_audio_frame(data)

            # 安全停止识别器
            if hasattr(self, 'recognizer') and self.recognizer:
                try:
                    self.recognizer.stop()
                    # 修复：清理识别器引用
                    if hasattr(self.recognizer, '_cleanup'):
                        self.recognizer._cleanup()
                except Exception as cleanup_e:
                    print(f"清理识别器时出错（可忽略）: {cleanup_e}")
                finally:
                    self.recognizer = None

            self.voice_status = "语音识别完成"
            if hasattr(self, 'callback'):
                self.callback.has_triggered = False

        except Exception as e:
            self.voice_status = f"语音识别失败: {str(e)}"
            print(f"语音识别异常: {str(e)}")
            # 确保清理识别器
            if hasattr(self, 'recognizer') and self.recognizer:
                try:
                    self.recognizer = None
                except:
                    pass


def async_analysis(crop_img, prompt, predictor):
    """main06.py风格的异步分析函数"""
    global last_response, last_thumbnail, analysis_active, sending_status

    try:
        print("🔄 开始AI分析...")
        analysis_active = True
        sending_status = "分析中..."

        thumbnail_size = 120
        thumbnail = cv2.resize(crop_img, (thumbnail_size, thumbnail_size))

        if crop_img is None or crop_img.size == 0:
            print("❌ 错误：无效的裁剪图像")
            return

        print(f"📊 裁剪图像尺寸: {crop_img.shape}")

        # 使用全局变量中的client
        global client
        if client is None:
            print("❌ 错误：API客户端未初始化")
            return

        _, buffer = cv2.imencode('.jpg', cv2.resize(crop_img, (640, 480)),
                                 [int(cv2.IMWRITE_JPEG_QUALITY), 75])
        base64_image = base64.b64encode(buffer).decode('utf-8')
        print(f"📷 图像已编码，长度: {len(base64_image)}")

        full_response = ""
        print("📡 发送API请求...")
        stream = client.chat.completions.create(
            model="qwen-omni-turbo",
            messages=[{
                "role": "user",
                "content": [
                    {"type": "text", "text": prompt},
                    {
                        "type": "image_url",
                        "image_url": {
                            "url": f"data:image/jpeg;base64,{base64_image}"
                        }
                    }
                ]
            }],
            temperature=0.7,
            max_tokens=200,
            timeout=20.0,
            stream=True
        )

        print("📨 接收API响应...")
        for chunk in stream:
            if chunk.choices[0].delta.content:
                chunk_text = chunk.choices[0].delta.content
                full_response += chunk_text
                with response_lock:
                    last_response = full_response
                    last_thumbnail = thumbnail

                time.sleep(0.05)
        with response_lock:
            predictor.analysis_history.append(full_response)

        # 写入AI结果到共享内存 (包含检测结果)
        if 'ai_results' in globals() and ai_results.connected:
            # 获取当前检测到的目标，转换为共享内存格式
            detection_data = []
            if 'current_detected_objects' in globals() and current_detected_objects:
                for obj in current_detected_objects:
                    detection_data.append({
                        'bbox': obj.get('bbox', [0, 0, 0, 0]),
                        'conf': obj.get('conf', 0.0),
                        'class': obj.get('class', 'unknown'),
                        'class_id': hash(obj.get('class', 'unknown')) % 1000  # 简单的class_id生成
                    })

            ai_results.write_results(
                voice_text=voice_recognition_text,
                ai_response=full_response,
                detection_boxes=detection_data
            )
            print(f"\n✅ [完整分析结果] (含{len(detection_data)}个检测目标)\n{full_response}\n{'=' * 40}\n")

    except Exception as e:
        sending_status = f"分析失败: {str(e)}"
        print(f"\n❌ API请求错误: {str(e)}\n")
        import traceback
        traceback.print_exc()
    finally:
        analysis_active = False
        sending_status = ""
        print("🏁 AI分析完成")


class AnalysisManager:
    def __init__(self):
        self.lock = threading.Lock()
        self.active = False
        self.thread = None

    def start_analysis(self, task_func, args):
        with self.lock:
            if not self.active:
                self.active = True
                self.thread = threading.Thread(
                    target=self._run_task,
                    args=(task_func, args),
                    daemon=True
                )
                self.thread.start()
                return True
            else:
                print("分析正在进行中，忽略新请求")
        return False

    def _run_task(self, task_func, args):
        try:
            task_func(*args)
        finally:
            with self.lock:
                self.active = False
                self.thread = None

    def cleanup(self):
        with self.lock:
            if self.thread and self.thread.is_alive():
                self.thread.join(timeout=1.0)


def is_gaze_in_box(gaze_point, bbox):
    """检查注视点是否在检测框内"""
    x1, y1, x2, y2 = map(int, bbox)
    return x1 <= gaze_point[0] <= x2 and y1 <= gaze_point[1] <= y2


def is_box_in_circle(bbox, center, radius):
    """检测框是否在视线圆圈内"""
    x1, y1, x2, y2 = map(int, bbox)
    cx, cy = center

    # 检查检测框的四个角点是否在圆内
    for x in [x1, x2]:
        for y in [y1, y2]:
            distance = math.sqrt((x - cx) ** 2 + (y - cy) ** 2)
            if distance <= radius:
                return True

    # 检查圆心是否在检测框内
    if x1 <= cx <= x2 and y1 <= cy <= y2:
        return True

    # 检查检测框边是否与圆相交
    closest_x = max(x1, min(cx, x2))
    closest_y = max(y1, min(cy, y2))
    distance = math.sqrt((closest_x - cx) ** 2 + (closest_y - cy) ** 2)

    return distance <= radius


def create_circular_crop(frame, center, radius):
    """创建圆形裁剪区域"""
    if frame is None or center is None:
        return frame

    h, w = frame.shape[:2]
    cx, cy = center

    # 创建掩膜
    mask = np.zeros((h, w), dtype=np.uint8)
    cv2.circle(mask, (cx, cy), radius, 255, -1)

    # 应用掩膜
    result = cv2.bitwise_and(frame, frame, mask=mask)
    return result


def is_gaze_in_box(gaze_point, bbox):
    """检查注视点是否在检测框内"""
    if not gaze_point or not bbox:
        return False
    x1, y1, x2, y2 = map(int, bbox)
    return x1 <= gaze_point[0] <= x2 and y1 <= gaze_point[1] <= y2


def is_box_in_circle(bbox, center, radius):
    """检测框是否在视线圆圈内"""
    if not bbox or not center:
        return False
    x1, y1, x2, y2 = map(int, bbox)
    cx, cy = center

    # 检查检测框的四个角是否在圆内
    corners = [(x1, y1), (x2, y1), (x1, y2), (x2, y2)]
    for corner_x, corner_y in corners:
        distance = math.sqrt((corner_x - cx) ** 2 + (corner_y - cy) ** 2)
        if distance <= radius:
            return True

    # 检查圆心是否在检测框内
    if x1 <= cx <= x2 and y1 <= cy <= y2:
        return True

    return False


def encode_image_to_base64(frame):
    """将图像编码为base64"""
    _, buffer = cv2.imencode('.jpg', frame)
    return base64.b64encode(buffer).decode('utf-8')


def main(model, hands, audio_interface, client):
    # 不在开始时创建窗口，等有图像数据后再创建
    if not CV2_AVAILABLE:
        print("Error: OpenCV not available, cannot create window")
        print("Please install OpenCV: pip install opencv-python")
        return

    # 声明全局变量
    global sending_status, analysis_active, last_response, last_thumbnail, voice_recognition_text, circle_radius, vad_enabled, current_detected_objects, current_target, scroll_offset, ai_results

    voice_assistant = VoiceAssistant(audio_interface)
    object_detector = ObjectDetector(model)

    # 初始化模式控制接口
    mode_control = ModeControlInterface()
    mode_control.connect()

    # 初始化AI结果接口
    ai_results = AIResultsInterface()
    if ai_results.connect():
        # 运行测试以确保共享内存正常工作
        ai_results.test_write_and_read()

        # 设置初始状态信息，告知用户检测未激活
        ai_results.write_results(
            voice_text="",
            ai_response="[READY] AI Vision Assistant 已启动 - 目标检测功能待激活\n请在mainwindow中点击'开始目标检测'按钮启动实时检测",
            detection_boxes=[]
        )
        print("[INFO] 已向mainwindow发送初始状态信息")

    # 异步初始化标志 - 用于跟踪后台资源加载状态
    global background_init_completed

    def background_initialization():
        """后台异步初始化VAD和其他资源"""
        global background_init_completed
        try:
            print("[ASYNC] 正在后台初始化VAD语音检测...")

            # 自动启动VAD语音检测
            if vad_enabled:
                voice_assistant.start_vad_listener()
                print("[OK] VAD语音检测已启动")

            # 可以在这里添加其他需要异步初始化的资源
            # 例如：模型预热、缓存预加载等

            background_init_completed = True
            print("[OK] 后台资源初始化完成")

            # 后台资源初始化完成，将在主循环中设置数据收集器连接

        except Exception as e:
            print(f"[ERROR] 后台初始化失败: {e}")
            background_init_completed = False

    # 启动后台初始化线程
    import threading
    background_thread = threading.Thread(target=background_initialization, daemon=True)
    background_thread.start()
    print("[INFO] 已启动后台资源初始化线程")

    # 窗口创建后再初始化所有对象（包括DataCollector）
    hand_detector = HandDetector(hands)
    collector = DataCollector(CONFIG["window_name"], hand_detector)
    predictor = IntentPredictor()
    analysis_manager = AnalysisManager()

    # 等待后台资源初始化完成
    print("⏳ 等待后台资源初始化完成...")
    while not background_init_completed:
        time.sleep(0.1)

    # 在后台资源初始化完成后，设置数据收集器连接
    print("🔄 开始设置数据收集连接模式...")
    collector.setup_connection_after_init()

    # 🔥 关键修复：检查是否需要重新连接模式控制
    if getattr(collector, 'need_reconnect_mode_control', False):
        print("🔄 重新连接模式控制接口...")
        mode_control.connect()
        if mode_control.connected:
            print("✅ 模式控制接口重新连接成功 - ASDF和目标检测功能已恢复")
        else:
            print("❌ 模式控制接口重新连接失败")
        collector.need_reconnect_mode_control = False

    print("🚀 程序启动成功")
    print(f"📺 数据源: {'共享内存 (Eye-tracking)' if collector.use_shared_memory else '摄像头'}")
    print("🖼️  AI结果将显示在主窗口底部白色区域（与main06.py一致）")
    print("\n⌨️  快捷键:")
    print("  空格键 - 触发AI分析 | V键 - 开关语音 | I键 - 显示AI调试信息 | Q键 - 退出")
    print("  A/S/D/F键 - 切换分析模式 | ↑↓键 - 滚动文本")
    if collector.use_shared_memory:
        print("\n📝 提示: 如果一直显示'等待数据'，请检查mainwindow是否正在写入共享内存")
    # 已移除目标跟踪器，使用基础物体检测

    # 初始化为空，等待AI分析结果
    last_response = ""

    # 标记是否已经创建和移动窗口
    window_created = False
    window_positioned = False

    # 连续帧计数器 - 只有连续检测到10帧才创建窗口
    consecutive_frames = 0
    required_frames = 10

    print("[INFO] 开始主循环，等待稳定的图像数据流...")
    frame_count = 0
    last_status_time = time.time()

    try:
        while True:
            start_time = time.time()
            frame = collector.get_frame()
            if frame is None:
                frame_count += 1
                consecutive_frames = 0  # 重置连续帧计数器
                # 🔍 调试信息：检查为什么图像获取失败
                if frame_count % 500 == 0:  # 每50次失败显示一次
                    print(f"🔍 调试：图像获取失败 {frame_count} 次，consecutive_frames重置为0")
                current_time = time.time()

                # 每5秒显示一次等待信息，提供更友好的反馈
                if current_time - last_status_time > 5.0:
                    elapsed_time = int(current_time - last_status_time)
                    print(f"[TIP] 请确保 mainwindow.exe 正在运行并提供共享内存数据")
                    last_status_time = current_time

                # 等待数据时添加短暂延迟，避免CPU占用过高
                time.sleep(0.01)  # 10ms延迟，降低CPU占用
                continue

            # 增加连续帧计数
            consecutive_frames += 1

            # 只有连续接收到足够帧数且窗口未创建时才创建窗口
            if consecutive_frames == required_frames and not window_created:
                print(f"[SUCCESS] 检测到稳定图像流（连续{required_frames}帧），正在创建显示窗口...")
            elif consecutive_frames < required_frames and not window_created:
                # 还未达到要求的连续帧数，继续等待但不创建窗口
                continue

            # 第一次获得图像数据时的处理
            if frame_count > 0:
                print(f"[INFO] 图像数据流已建立，等待了 {frame_count} 次循环")
                frame_count = 0  # 重置计数器

            # 在处理完所有图像后，在第一次显示时移动窗口
            move_window_after_display = not window_positioned and collector.use_shared_memory

            # main05.py的核心功能：从共享内存获取真实的眼球追踪坐标
            # 获取视线位置（已经转换为图像坐标）
            collector.mouse_pos = collector.get_gaze_position()

            # 优化: 直接修改原帧，避免复制
            rendered_frame = frame

            # 🔥 只有在检测激活时才执行目标检测（完全由mainwindow按钮控制）
            if getattr(mode_control, 'detection_active', False):
                # 优化: 隔帧检测，降低GPU负载
                if not hasattr(collector, '_detection_counter'):
                    collector._detection_counter = 0
                collector._detection_counter += 1

                if collector._detection_counter % 2 == 0:  # 每2帧检测一次
                    det_result = object_detector.detect_objects(frame)
                    if not hasattr(collector, '_last_detection'):
                        collector._last_detection = det_result if det_result else []
                    else:
                        collector._last_detection = det_result if det_result else collector._last_detection
                else:
                    det_result = getattr(collector, '_last_detection', [])

                # 处理所有检测到的目标
                all_detected_objects = []
                if det_result and hasattr(det_result, 'boxes') and det_result.boxes is not None:
                    for obj in det_result.boxes:
                        bbox = obj.xyxy[0].tolist()  # [x1, y1, x2, y2]
                        x1, y1, x2, y2 = bbox

                        # 计算目标中心点
                        obj_center_x = (x1 + x2) / 2
                        obj_center_y = (y1 + y2) / 2

                        all_detected_objects.append({
                            "class": det_result.names[int(obj.cls)],
                            "conf": float(obj.conf),
                            "bbox": bbox,
                            "center": (obj_center_x, obj_center_y)
                        })

                    # 绘制所有检测框
                    for obj in all_detected_objects:
                        bbox = obj["bbox"]
                        x1, y1, x2, y2 = map(int, bbox)

                        # 绘制检测框（绿色）
                        cv2.rectangle(rendered_frame, (x1, y1), (x2, y2), (0, 255, 0), 2)

                        # 绘制类别和置信度
                        label = f"{obj['class']}: {obj['conf']:.2f}"
                        cv2.putText(rendered_frame, label, (x1, y1 - 10),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

                        # 绘制目标中心点
                        center_x, center_y = map(int, obj["center"])
                        cv2.circle(rendered_frame, (center_x, center_y), 3, (255, 0, 0), -1)

                    # 更新全局目标列表（包含所有检测到的目标）
                    current_detected_objects = all_detected_objects
                else:
                    current_detected_objects = []
            else:
                # 检测未激活时，清空目标列表
                current_detected_objects = []

            # 🔥 实时写入检测结果到共享内存 (当检测激活时)
            if getattr(mode_control, 'detection_active', False) and ai_results.connected:
                # 准备检测数据 - 传输所有检测到的目标（限制最多28个）
                detection_data = []
                if current_detected_objects:
                    # 取前28个目标（共享内存限制）
                    for obj in current_detected_objects[:28]:
                        # 转换bbox格式：[x1, y1, x2, y2] -> [x, y, w, h]
                        bbox = obj.get('bbox', [0, 0, 0, 0])
                        if len(bbox) == 4:
                            x1, y1, x2, y2 = bbox
                            x, y, w, h = x1, y1, x2 - x1, y2 - y1  # 转换为x,y,w,h格式
                        else:
                            x, y, w, h = 0, 0, 0, 0

                        detection_data.append({
                            'bbox': [x, y, w, h],  # 正确的x,y,w,h格式
                            'conf': obj.get('conf', 0.0),
                            'class': obj.get('class', 'unknown'),
                            'class_id': hash(obj.get('class', 'unknown')) % 1000
                        })

                # 基于时间和内容变化的智能更新策略

                current_time = time.time()
                current_detection_count = len(detection_data)
                last_detection_count = getattr(ai_results, '_last_detection_count', -1)
                last_update_time = getattr(ai_results, '_last_update_time', 0)
                last_detection_data = getattr(ai_results, '_last_detection_data', [])

                # 检查是否需要更新：
                # 1. 检测数量变化
                # 2. 超过100ms未更新（提高更新频率）
                # 3. 目标位置发生显著变化（>10像素）
                should_update = False

                if current_detection_count != last_detection_count:
                    should_update = True
                    reason = f"检测数量变化: {last_detection_count} -> {current_detection_count}"
                elif current_time - last_update_time > 0.033:  # 33ms强制更新（约30FPS）
                    should_update = True
                    reason = f"时间间隔更新 ({int((current_time - last_update_time) * 1000)}ms)"
                elif detection_data and last_detection_data:
                    # 检查位置变化（只对相同数量的检测结果进行比较）
                    if len(detection_data) == len(last_detection_data):
                        for i, (current, last) in enumerate(zip(detection_data, last_detection_data)):
                            curr_bbox = current.get('bbox', [0, 0, 0, 0])
                            last_bbox = last.get('bbox', [0, 0, 0, 0])
                            # 计算位置变化（中心点距离）
                            curr_center_x = curr_bbox[0] + curr_bbox[2] / 2
                            curr_center_y = curr_bbox[1] + curr_bbox[3] / 2
                            last_center_x = last_bbox[0] + last_bbox[2] / 2
                            last_center_y = last_bbox[1] + last_bbox[3] / 2
                            distance = ((curr_center_x - last_center_x) ** 2 + (
                                    curr_center_y - last_center_y) ** 2) ** 0.5
                            if distance > 10:  # 位置变化超过10像素
                                should_update = True
                                reason = f"目标{i + 1}位置变化{distance:.1f}像素"
                                break

                if should_update:
                    # 写入检测结果
                    ai_results.write_results(
                        voice_text="",  # 空语音文本
                        ai_response=f"🎯 全场景检测模式 - 发现 {current_detection_count} 个目标对象",
                        detection_boxes=detection_data
                    )
                    ai_results._last_detection_count = current_detection_count
                    ai_results._last_update_time = current_time
                    ai_results._last_detection_data = detection_data.copy()
                    print(f"🔄 检测结果更新: {reason}")
            elif not getattr(mode_control, 'detection_active', True):
                # 清除检测结果显示（当检测未激活时）
                if ai_results.connected and getattr(ai_results, '_last_detection_count', 0) != 0:
                    ai_results.write_results(
                        voice_text="",
                        ai_response="[PAUSE] 目标检测已关闭 - 点击mainwindow中的'开始目标检测'按钮启动",
                        detection_boxes=[]
                    )
                    ai_results._last_detection_count = 0

            # 【修改】暂时不绘制视线圆圈，等图像处理完成后再绘制
            # 记录当前的视线位置和圆圈半径，稍后在最终画面上绘制
            current_gaze_pos = collector.mouse_pos
            current_circle_radius = circle_radius

            # 显示详细的坐标调试信息
            if collector.use_shared_memory:
                screen_pos = collector.shared_memory.get_gaze_point()
                window_offset_x = getattr(collector.shared_memory, 'window_offset_x', 0)
                window_offset_y = getattr(collector.shared_memory, 'window_offset_y', 0)

                # 计算理论位置（使用固定窗口位置490, 271）
                theoretical_x = screen_pos[0] - 490 if screen_pos else None
                theoretical_y = screen_pos[1] - 271 if screen_pos else None

                # 详细追踪坐标计算过程
                if screen_pos and collector.mouse_pos:
                    actual_x, actual_y = collector.mouse_pos
                    debug_lines = [
                        f"1. Screen Raw: ({screen_pos[0]}, {screen_pos[1]})",
                        f"2. Window Fixed: (490, 271)",
                        f"3. Should Be: ({screen_pos[0] - 490}, {screen_pos[1] - 271})",
                        f"4. Actually Got: ({actual_x}, {actual_y})",
                        f"5. Error: ({actual_x - (screen_pos[0] - 490)}, {actual_y - (screen_pos[1] - 271)})"
                    ]
                else:
                    debug_lines = [f"Screen: {screen_pos}", f"Mouse_pos: {collector.mouse_pos}"]

                # for i, line in enumerate(debug_lines):
                #     cv2.putText(rendered_frame, line, (10, 60 + i * 25),
                #                 cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1)
            else:
                pass  # 移除鼠标坐标显示

            # 优化: 移除重复的检测框绘制 - current_detected_objects已在上方绘制过

            # 完全按照main06.py的UI布局实现
            # 优化: 简化PIL转换，减少内存分配
            rgb_frame = cv2.cvtColor(rendered_frame, cv2.COLOR_BGR2RGB)
            pil_img = Image.fromarray(rgb_frame)
            draw = ImageDraw.Draw(pil_img)

            # 【移除】不在rendered_frame上绘制面板背景
            # panel_width = 400
            # cv2.rectangle(rendered_frame,
            #               (rendered_frame.shape[1] - panel_width, 0),
            #               (rendered_frame.shape[1], rendered_frame.shape[0]),
            #               (40, 40, 40), -1)

            mode_config = predictor.MODE_CONFIG[predictor.active_mode]

            # 根据检测状态显示不同的信息
            detection_status = getattr(mode_control, 'detection_active', False)
            if detection_status:
                detection_info = f"检测到 {len(current_detected_objects)} 物体 | 空格分析 | Q退出"
            else:
                detection_info = "目标检测已关闭 | 在mainwindow中点击'开始目标检测'启动"

            # 显示后台初始化状态
            bg_status = "完成" if background_init_completed else "初始化中..."

            status_lines = [
                f"模式：{mode_config['name']} (A/S/D/F切换)",
                detection_info,
                f"状态：{sending_status if sending_status else '就绪'}",
                f"语音：{voice_assistant.voice_status}",
                f"语音识别：{voice_recognition_text[:30] + '...' if voice_recognition_text else '无'}",
                f"VAD状态: {'开启' if vad_enabled else '关闭'} (V键切换) | 后台: {bg_status}",
                f"视线圈半径: {circle_radius} (滚轮调整)"
            ]

            # 【移除】不在rendered_frame上绘制红色文字，稍后在display_frame上绘制
            # for i, line in enumerate(status_lines):
            #     draw.text((rendered_frame.shape[1] - panel_width + 10, 20 + i * 30),
            #               line, font=font, fill=(255, 0, 0))

            if analysis_active:
                cv2.putText(rendered_frame, "分析中...", (50, 50),
                            cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
            if sending_status:
                left, top, right, bottom = draw.textbbox((0, 0), sending_status, font=font)
                text_width = right - left
                text_height = bottom - top
                text_x = (rendered_frame.shape[1] - text_width) // 2
                text_y = (rendered_frame.shape[0] - text_height) // 2
                draw.text((text_x, text_y), sending_status, font=font, fill=(0, 255, 0))

            rendered_frame = cv2.cvtColor(np.array(pil_img), cv2.COLOR_RGB2BGR)

            # 【最终方案】图像实际尺寸显示，窗口固定1280x720，多余区域填充黑色
            target_width, target_height = 1280, 720
            current_height, current_width = rendered_frame.shape[:2]

            # 创建固定尺寸的黑色背景
            display_frame = np.zeros((target_height, target_width, 3), dtype=np.uint8)

            # 计算居中位置，图像按实际尺寸显示
            start_x = (target_width - current_width) // 2
            start_y = (target_height - current_height) // 2

            # 计算实际可放置的区域（防止越界）
            dst_x1 = max(0, start_x)
            dst_y1 = max(0, start_y)
            dst_x2 = min(target_width, start_x + current_width)
            dst_y2 = min(target_height, start_y + current_height)

            # 计算对应的源图像区域
            src_x1 = max(0, -start_x)
            src_y1 = max(0, -start_y)
            src_x2 = src_x1 + (dst_x2 - dst_x1)
            src_y2 = src_y1 + (dst_y2 - dst_y1)

            # 将图像按实际尺寸放置到居中位置
            if dst_x2 > dst_x1 and dst_y2 > dst_y1:
                display_frame[dst_y1:dst_y2, dst_x1:dst_x2] = rendered_frame[src_y1:src_y2, src_x1:src_x2]

            # 显示信息（仅一次）
            if not hasattr(collector, '_display_info_shown'):
                collector._display_info_shown = True
                print(f"📐 图像实际尺寸显示: {current_width}x{current_height}，窗口: {target_width}x{target_height}")

            # display_frame现在包含最终要显示的图像（实际尺寸，黑色填充）

            # 【简化】直接在display_frame上绘制UI元素
            # 使用PIL绘制红色文字
            rgb_display = cv2.cvtColor(display_frame, cv2.COLOR_BGR2RGB)
            pil_display = Image.fromarray(rgb_display)
            draw_display = ImageDraw.Draw(pil_display)

            # 在右侧绘制状态信息（无论图像大小，直接绘制在黑色背景或图像上）
            panel_width = 400
            panel_x = 1280 - panel_width
            for i, line in enumerate(status_lines):
                draw_display.text((panel_x + 10, 20 + i * 30), line, font=font, fill=(255, 0, 0))

            # 绘制其他状态信息
            if analysis_active:
                draw_display.text((50, 50), "分析中...", font=font, fill=(0, 255, 0))
            if sending_status:
                left, top, right, bottom = draw_display.textbbox((0, 0), sending_status, font=font)
                text_width = right - left
                text_x = (1280 - text_width) // 2
                text_y = (720 - 50) // 2
                draw_display.text((text_x, text_y), sending_status, font=font, fill=(0, 255, 0))

            # 转换回OpenCV格式
            display_frame = cv2.cvtColor(np.array(pil_display), cv2.COLOR_RGB2BGR)

            # 只有在A模式且图像尺寸为1280x720时才绘制红色圆形
            if (predictor.active_mode == "A" and
                current_width >= 1280 and current_height >= 720):
                if current_gaze_pos and current_circle_radius > 0:
                    try:
                        # 绘制主视线圆圈（红色）
                        cv2.circle(display_frame, current_gaze_pos, current_circle_radius, (0, 0, 255), 3)
                        cv2.circle(display_frame, current_gaze_pos, 3, (0, 255, 0), -1)
                    except Exception as e:
                        pass

            # 完全按照main06.py的方式使用response_lock
            with response_lock:
                if last_thumbnail is not None and hasattr(last_thumbnail, 'shape'):
                    try:
                        thumb_h, thumb_w = last_thumbnail.shape[:2]
                        y_start = display_frame.shape[0] - thumb_h - 10
                        x_start = 10
                        if (y_start >= 0 and x_start + thumb_w <= display_frame.shape[1]):
                            display_frame[y_start:y_start + thumb_h, x_start:x_start + thumb_w] = last_thumbnail
                    except Exception as e:
                        print(f"显示缩略图时出错: {str(e)}")
                if last_response:
                    display_frame = add_border_with_text(display_frame, last_response, scroll_offset)

            fps = 1.0 / (time.time() - start_time)
            cv2.putText(display_frame, f"FPS: {fps:.1f}", (10, 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
            # 只有在连续帧数达到要求时才创建和显示窗口
            if consecutive_frames >= required_frames:
                # 确保窗口已创建
                # if not window_created:  #调试可视化
                #     print("[INFO] 正在创建主显示窗口...")
                #     cv2.waitKey(100)  # 减少等待时间从1000ms到100ms
                #     cv2.namedWindow(CONFIG["window_name"], cv2.WINDOW_NORMAL)
                #     cv2.resizeWindow(CONFIG["window_name"], 1280, 720)  # 固定窗口尺寸
                #     cv2.setWindowProperty(CONFIG["window_name"], cv2.WND_PROP_TOPMOST, 0)
                #     window_created = True
                #
                #     # 窗口创建后，设置鼠标回调
                #     cv2.setMouseCallback(CONFIG["window_name"], collector.handle_mouse_events)
                #     if collector.mouse_callback_pending:
                #         collector.mouse_callback_pending = False
                #         print("[INFO] 鼠标回调已设置 - 摄像头模式下可用鼠标模拟视线")
                #     else:
                #         print("[INFO] 鼠标回调已设置 - 支持滚轮调整圆圈大小")
                #
                #     print(f"[OK] 主显示窗口已创建: '{CONFIG['window_name']}'")

                # cv2.imshow(CONFIG["window_name"], display_frame)
                # cv2.waitKey(1)
                #调试窗口


                # 只有窗口创建后才处理键盘事件和窗口移动
                should_process_events = True
            else:
                # 尚未达到连续帧数要求，跳过窗口显示和事件处理
                # 🔥 临时修复：如果使用共享内存且已连接，允许按键处理（即使图像不稳定）
                if collector.use_shared_memory and collector.shared_memory.connected:
                    should_process_events = True
                    # 创建一个最小化的显示（避免按键检测失效）
                    # if not window_created:  #调试可视化
                    #     print("[INFO] 共享内存已连接，创建最小显示窗口以启用按键处理...")
                    #     cv2.namedWindow(CONFIG["window_name"], cv2.WINDOW_NORMAL)
                    #     cv2.resizeWindow(CONFIG["window_name"], 1280, 720)
                    #     window_created = True
                    # 显示一个简单的提示画面
                    placeholder_frame = np.zeros((720, 1280, 3), dtype=np.uint8)
                    cv2.putText(placeholder_frame, "Waiting for stable image...", (400, 360),
                                cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                    cv2.imshow(CONFIG["window_name"], placeholder_frame)
                else:
                    should_process_events = False

            # 在第一次显示后立即移动窗口到居中位置（只有窗口创建后才执行）
            if move_window_after_display and should_process_events:
                image_height, image_width = frame.shape[:2]
                screen_width = collector.shared_memory.screen_width
                screen_height = collector.shared_memory.screen_height

                # 简单方法：直接让窗口内容居中
                window_x = (screen_width - image_width) // 2
                window_y = (screen_height - image_height) // 2

                # 确保窗口不超出屏幕
                window_x = max(0, min(window_x, screen_width - image_width))
                window_y = max(0, min(window_y, screen_height - image_height))

                print(f"[INFO] 正在移动窗口到屏幕中心: ({window_x}, {window_y})")
                print(f"[INFO] 目标窗口: '{CONFIG['window_name']}'")

                # 移动窗口到居中位置
                try:
                    cv2.moveWindow(CONFIG["window_name"], window_x, window_y)
                    print(f"[OK] moveWindow命令执行成功")

                    # 等待一帧让窗口移动生效
                    cv2.waitKey(1) & 0xFF  # 优化: 只取低8位

                    # 再次尝试移动确保生效
                    cv2.moveWindow(CONFIG["window_name"], window_x, window_y)
                    cv2.waitKey(1) & 0xFF  # 优化: 只取低8位

                    # 使用实际测量的窗口位置
                    # 根据用户实际测量，窗口左上角在(490, 271)
                    actual_window_x = 490
                    actual_window_y = 271
                    collector.shared_memory.window_offset_x = actual_window_x
                    collector.shared_memory.window_offset_y = actual_window_y
                    print(f"🔧 设置窗口位置: moveWindow({window_x}, {window_y})")
                    print(f"🔧 使用实际窗口位置: ({actual_window_x}, {actual_window_y})")

                except Exception as e:
                    print(f"❌ moveWindow失败: {e}")
                    print(f"🔍 尝试直接使用窗口名称...")
                    try:
                        cv2.moveWindow("AI Vision Assistant", window_x, window_y)
                        print(f"✅ 使用直接窗口名称成功")
                        # 使用实际测量的窗口位置
                        collector.shared_memory.window_offset_x = 490
                        collector.shared_memory.window_offset_y = 271
                        print(f"🔧 使用实际窗口位置: (490, 271)")
                    except Exception as e2:
                        print(f"❌ 直接窗口名称也失败: {e2}")

                # 使用固定的窗口位置，不使用动态计算的值
                collector.shared_memory.window_offset_x = 490
                collector.shared_memory.window_offset_y = 271

                print(f"✅ 窗口已移动到屏幕中心")
                pass
                print(f"🗺️ 屏幕: {screen_width}x{screen_height}, 图像: {image_width}x{image_height}")

                window_positioned = True

            elapsed = (time.time() - start_time) * 1000
            delay = max(1, 30 - int(elapsed))

            # 优化: 检查窗口是否被用户关闭（在显示后、waitKey前检查）
            if window_created:  # 只在窗口已创建时检查
                try:
                    # 检查窗口的可见性和存在性
                    window_visible = cv2.getWindowProperty(CONFIG["window_name"], cv2.WND_PROP_VISIBLE)

                    # 如果窗口不可见或属性获取失败说明窗口被关闭
                    if window_visible < 1:  # 修改判断条件，<1表示窗口不可见或不存在
                        print("检测到窗口被关闭，退出程序")
                        break
                except cv2.error as e:
                    print(f"窗口不存在或已被关闭，退出程序: {e}")
                    break

            # 检查模式控制共享内存中的模式变化和检测控制指令
            if mode_control.connected:
                mode_change, detection_change = mode_control.check_mode_change()
                if mode_change:
                    predictor.switch_mode(mode_change)

                    # 🔥 模式切换后自动触发AI分析（模拟空格键功能）
                    if not analysis_active:
                        print(f"[AUTO] 模式切换到{mode_change}，自动触发AI分析...")
                        try:
                            print(f"[AUTO] 当前位置: {collector.mouse_pos}")
                            print(f"[AUTO] 检测到物体: {len(current_detected_objects)}")
                            print(f"[AUTO] 图像尺寸: {frame.shape if frame is not None else 'None'}")

                            # 【修改】检测是否为全图模式(1280x720)，实现聚焦分析
                            if frame.shape[1] >= 1280 and frame.shape[0] >= 720:
                                # 全图模式：使用红色圆形区域进行聚焦分析
                                crop_img = create_circular_crop(frame.copy(), current_gaze_pos, current_circle_radius)
                                print(f"[FOCUS] 全图模式聚焦分析: 圆心={current_gaze_pos}, 半径={current_circle_radius}")
                            else:
                                # 小图模式：直接使用gazeRegion
                                crop_img = frame.copy()
                                print(f"[REGION] 区域模式分析: 图像尺寸={frame.shape}")

                            # 【注释】A模式原来的圆形裁剪逻辑
                            # if predictor.active_mode == "A":
                            #     crop_img = create_circular_crop(frame.copy(), collector.mouse_pos, circle_radius)
                            # else:
                            #     crop_img = frame.copy()

                            prompt = predictor.generate_prompt(current_detected_objects, collector.mouse_pos,
                                                               voice_recognition_text)
                            print(f"[AUTO] AI提示词: {prompt[:100]}...")

                            success = analysis_manager.start_analysis(async_analysis, (crop_img, prompt, predictor))
                            if success:
                                print("[OK] 模式按钮自动分析已启动")
                            else:
                                print("[ERROR] 模式按钮自动分析启动失败")

                            # 重置语音识别文本
                            voice_recognition_text = ""
                        except Exception as e:
                            print(f"[ERROR] 模式按钮自动分析异常: {e}")
                            import traceback
                            traceback.print_exc()

                # 处理检测控制指令
                if detection_change == 'start':
                    print("🎯 目标检测已启动 (通过共享内存指令)")
                    print(f"   检测状态: detection_active = {getattr(mode_control, 'detection_active', False)}")
                    if ai_results.connected:
                        ai_results.write_results(ai_response="🎯 目标检测已启动，正在分析视线区域的目标对象...")
                elif detection_change == 'stop':
                    print("🛑 目标检测已停止 (通过共享内存指令)")
                    print(f"   检测状态: detection_active = {getattr(mode_control, 'detection_active', False)}")
                    if ai_results.connected:
                        ai_results.write_results(ai_response="🛑 目标检测已停止")

            # 只有窗口创建后才处理键盘事件
            if should_process_events:
                key = cv2.waitKey(delay) & 0xFF
            else:
                # 窗口未创建时，使用短延迟并继续循环
                time.sleep(0.01)
                continue

            if key == ord('q') or key == 27:  # ESC键也可以退出
                break
            elif key in [ord('a'), ord('s'), ord('d'), ord('f')]:
                predictor.switch_mode(chr(key).upper())
            elif key == 32:  # 空格键触发分析
                print("🔍 空格键触发分析...")
                try:
                    if not analysis_active:
                        print(f"📍 当前位置: {collector.mouse_pos}")
                        print(f"🎯 检测到物体: {len(current_detected_objects)}")

                        # 检查图像和提示词
                        print(f"📊 图像尺寸: {frame.shape if frame is not None else 'None'}")

                        # 【修改】检测是否为全图模式(1280x720)，实现聚焦分析
                        if frame.shape[1] >= 1280 and frame.shape[0] >= 720:
                            # 全图模式：使用红色圆形区域进行聚焦分析
                            crop_img = create_circular_crop(frame.copy(), current_gaze_pos, current_circle_radius)
                            print(f"[FOCUS] 全图模式聚焦分析: 圆心={current_gaze_pos}, 半径={current_circle_radius}")
                        else:
                            # 小图模式：直接使用gazeRegion
                            crop_img = frame.copy()
                            print(f"[REGION] 区域模式分析: 图像尺寸={frame.shape}")

                        # 【注释】A模式原来的圆形裁剪逻辑
                        # if predictor.active_mode == "A":
                        #     crop_img = create_circular_crop(frame.copy(), collector.mouse_pos, circle_radius)
                        # else:
                        #     crop_img = frame.copy()

                        prompt = predictor.generate_prompt(current_detected_objects, collector.mouse_pos,
                                                           voice_recognition_text)
                        print(f"🤖 AI提示词: {prompt[:100]}...")
                        print(f"📡 启动AI分析...")

                        success = analysis_manager.start_analysis(async_analysis, (crop_img, prompt, predictor))
                        if success:
                            print("✅ AI分析已启动")
                        else:
                            print("❌ AI分析启动失败")

                        # 重置语音识别文本
                        voice_recognition_text = ""
                    else:
                        print("⚠️ 分析正在进行中，忽略新请求")
                except Exception as e:
                    print(f"❌ 空格键处理异常: {e}")
                    import traceback
                    traceback.print_exc()
                    last_response = f"错误: {str(e)}"
            elif key == ord('v'):  # V键切换VAD开关
                voice_assistant.toggle_vad()

            # 语音触发的视觉分析处理
            if voice_assistant.trigger_visual_analysis and not analysis_active and voice_recognition_text.strip():
                voice_assistant.trigger_visual_analysis = False

                # 【修改】语音分析检测是否为全图模式(1280x720)，实现聚焦分析
                if frame.shape[1] >= 1280 and frame.shape[0] >= 720:
                    # 全图模式：使用红色圆形区域进行聚焦分析
                    crop_img = create_circular_crop(frame.copy(), current_gaze_pos, current_circle_radius)
                    print(f"[VOICE-FOCUS] 全图模式聚焦分析: 圆心={current_gaze_pos}, 半径={current_circle_radius}")
                else:
                    # 小图模式：直接使用gazeRegion
                    crop_img = frame.copy()
                    print(f"[VOICE-REGION] 区域模式分析: 图像尺寸={frame.shape}")

                # 【注释】A模式原来的圆形裁剪逻辑
                # if predictor.active_mode == "A":
                #     crop_img = create_circular_crop(frame.copy(), collector.mouse_pos, circle_radius)
                # else:
                #     crop_img = frame.copy()

                if crop_img is not None and crop_img.size > 0:
                    obj_list = [f"{obj['class']}({obj['conf']:.1%})" for obj in current_detected_objects]
                    prompt = f"用户语音输入: {voice_recognition_text}\n" \
                        f"当前环境物体列表: {', '.join(obj_list)}\n" \
                        "请根据用户问题直接打印用户问题并给出答案，不要分析过程"

                    # 启动视觉分析
                    analysis_manager.start_analysis(async_analysis, (crop_img, prompt, predictor))
                    sending_status = "语音触发分析中..."
                    # 清空语音识别文本
                    voice_recognition_text = ""
                else:
                    print("圆形区域无效，跳过分析")

    except KeyboardInterrupt:
        pass
    finally:
        # 修复资源泄漏：完善资源清理
        analysis_active = False

        try:
            analysis_manager.cleanup()
        except Exception as e:
            print(f"分析管理器清理异常: {e}")

        try:
            # 使用强制停止方法，确保所有语音操作都被正确清理
            voice_assistant.force_stop_all()

            # 短暂延迟，确保所有异步操作完成
            print("等待所有异步操作完成...")
            time.sleep(0.5)  # 等待500ms

        except Exception as e:
            print(f"语音助手清理异常: {e}")

        try:
            # 清理模式控制接口
            mode_control.disconnect()
        except Exception as e:
            print(f"模式控制接口清理异常: {e}")

        try:
            # 清理AI结果接口
            ai_results.disconnect()
        except Exception as e:
            print(f"AI结果接口清理异常: {e}")

        try:
            collector.cleanup()
        except Exception as e:
            print(f"数据收集器清理异常: {e}")

        try:
            cv2.destroyAllWindows()
            print("✅ 窗口已关闭")
        except Exception as e:
            print(f"OpenCV窗口清理异常: {e}")

        # 延迟一下确保异步操作完成
        time.sleep(0.5)
        print("🔄 正在完成最后的清理...")
        time.sleep(0.3)  # 再等一下确保异步操作完全结束
        print("✅ 系统安全关闭")


# AI结果现在显示在主窗口底部（与main06.py一致）

def test_ai_response():
    """测试AI响应显示功能"""
    global last_response

    current_time = time.strftime('%H:%M:%S')
    test_response = f"🗺️ 测试AI响应 - {current_time}\n\n这是一个测试响应，用于验证AI结果显示功能是否正常。\n\n如果你看到这条消息显示在主窗口底部的白色区域，说明AI结果显示功能已经正常工作。\n\n✅ 测试成功！"
    last_response = test_response
    print(f"✅ 测试AI响应已设置: {test_response[:50]}...")


if __name__ == "__main__":
    try:
        print("Loading resources asynchronously...")
        load_resources_async(main)
    except Exception as e:
        print(f"程序启动失败: {e}")
        print("请检查：")
        print("1. API密钥是否正确设置")
        print("2. 共享内存是否可用（或摄像头是否连接正常）")
        print("3. YOLO模型文件是否存在")
        print("4. 字体文件是否存在")
        print("5. 主程序是 否已启动并创建共 享内存")
