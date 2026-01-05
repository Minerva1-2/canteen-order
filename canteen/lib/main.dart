import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

void main() {
  runApp(const CanteenApp());
}

class CanteenApp extends StatelessWidget {
  const CanteenApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Canteen Management',
      theme: ThemeData(
        primarySwatch: Colors.orange,
        scaffoldBackgroundColor: const Color(0xFFF5F5F5),
        useMaterial3: true,
      ),
      home: const RestaurantManagementScreen(),
    );
  }
}

// ---------------- 数据模型 ----------------

class Order {
  final String id; // 订单唯一ID
  final int tableId; // 桌号
  final List<String> items; // 菜品列表
  final double totalPrice; // 总价
  String status; // 状态: 'processing', 'ready'
  final String time; // 下单时间

  Order({
    required this.id,
    required this.tableId,
    required this.items,
    required this.totalPrice,
    this.status = 'processing',
    required this.time,
  });

  // 从 MQTT JSON 解析
  factory Order.fromJson(Map<String, dynamic> json) {
    List<String> itemsList = [];
    if (json['items'] != null) {
      json['items'].forEach((v) {
        itemsList.add("${v['name']} x${v['count']}");
      });
    }

    DateTime now = DateTime.now();
    String timeStr =
        "${now.hour.toString().padLeft(2, '0')}:${now.minute.toString().padLeft(2, '0')}";

    return Order(
      id: DateTime.now().millisecondsSinceEpoch.toString().substring(8),
      tableId: json['table'] ?? 0,
      items: itemsList,
      totalPrice: (json['total'] ?? 0).toDouble(),
      time: timeStr,
      status: 'processing',
    );
  }
}

// ---------------- 主界面 ----------------

class RestaurantManagementScreen extends StatefulWidget {
  const RestaurantManagementScreen({super.key});

  @override
  State<RestaurantManagementScreen> createState() =>
      _RestaurantManagementScreenState();
}

class _RestaurantManagementScreenState
    extends State<RestaurantManagementScreen> {
  // 订单数据源
  final List<Order> _orders = [];

  // MQTT 客户端
  late MqttServerClient client;
  bool isConnected = false;

  @override
  void initState() {
    super.initState();
    // 启动时连接 MQTT
    _connectMqtt();
  }

  @override
  void dispose() {
    client.disconnect();
    super.dispose();
  }

  // --- MQTT 连接逻辑 ---
  Future<void> _connectMqtt() async {
    // 配置连接信息 (阿里云 IP)
    client = MqttServerClient(
      '47.108.190.17',
      'flutter_admin_${DateTime.now().millisecondsSinceEpoch}',
    );
    client.port = 1883;
    client.logging(on: false);
    client.keepAlivePeriod = 60;
    client.onDisconnected = _onDisconnected;
    client.onConnected = _onConnected;

    final connMess = MqttConnectMessage()
        .withClientIdentifier('flutter_client')
        .startClean()
        .withWillQos(MqttQos.atLeastOnce);
    client.connectionMessage = connMess;

    try {
      print('MQTT: Connecting...');
      await client.connect();
    } catch (e) {
      print('MQTT: Connection failed - $e');
      client.disconnect();
    }
  }

  // 连接成功回调
  void _onConnected() {
    setState(() {
      isConnected = true;
    });
    print('MQTT: Connected');

    // 订阅主题
    client.subscribe('canteen/order/new', MqttQos.atLeastOnce);
    client.subscribe('canteen/service/urge', MqttQos.atLeastOnce);

    // 监听消息流
    client.updates!.listen((List<MqttReceivedMessage<MqttMessage?>>? c) {
      final MqttPublishMessage recMess = c![0].payload as MqttPublishMessage;
      final String pt = MqttPublishPayload.bytesToStringAsString(
        recMess.payload.message,
      );
      final String topic = c[0].topic;

      _handleMessage(topic, pt);
    });
  }

  void _onDisconnected() {
    setState(() {
      isConnected = false;
    });
    print('MQTT: Disconnected');
  }

  // --- 核心：消息处理与 UI 刷新 ---
  void _handleMessage(String topic, String payload) {
    try {
      final Map<String, dynamic> data = jsonDecode(payload);

      // 情况 A: 新订单
      if (topic == 'canteen/order/new') {
        Order newOrder = Order.fromJson(data);

        // setState 触发无感刷新
        setState(() {
          _orders.insert(0, newOrder);
        });

        // 底部弹出绿色提示条 (SnackBar)
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text("收到 ${newOrder.tableId} 号桌的新订单!"),
            backgroundColor: Colors.green,
            duration: const Duration(seconds: 2),
          ),
        );
      }
      // 情况 B: 催单请求
      else if (topic == 'canteen/service/urge') {
        int tableId = data['table'];

        // 底部弹出红色警告条
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Row(
              children: [
                const Icon(Icons.warning_amber_rounded, color: Colors.white),
                const SizedBox(width: 10),
                Text("注意！$tableId 号桌正在催单！"),
              ],
            ),
            backgroundColor: Colors.redAccent,
            duration: const Duration(seconds: 5),
            action: SnackBarAction(
              label: '收到',
              textColor: Colors.white,
              onPressed: () {
                // 点击收到后的逻辑，例如关闭提示
              },
            ),
          ),
        );
      }
    } catch (e) {
      print("JSON Parse Error: $e");
    }
  }

  // --- 发送通知给开发板 ---
  void _remindCustomer(Order order) {
    // 1. 本地状态更新
    setState(() {
      order.status = 'ready'; // 变为待取餐状态
    });

    // 2. 发送 MQTT 消息给开发板 (通知其弹窗)
    if (isConnected) {
      final builder = MqttClientPayloadBuilder();
      String payload =
          '{"type":"service", "action":"notify", "table":${order.tableId}}';
      builder.addString(payload);

      client.publishMessage(
        'canteen/service/notify',
        MqttQos.atLeastOnce,
        builder.payload!,
      );
      print('MQTT Published: $payload');
    }

    // 3. App 端提示操作成功
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Row(
          children: [
            const Icon(Icons.notifications_active, color: Colors.white),
            const SizedBox(width: 10),
            Text('已向 ${order.tableId} 号桌发送取餐弹窗'),
          ],
        ),
        backgroundColor: Colors.orange,
        duration: const Duration(seconds: 1),
      ),
    );
  }

  // ---------------- UI 构建 ----------------

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text(
              "美滋滋订单管理",
              style: TextStyle(fontWeight: FontWeight.bold),
            ),
            const SizedBox(width: 8),
            // 连接状态指示灯
            Container(
              width: 10,
              height: 10,
              decoration: BoxDecoration(
                color: isConnected ? Colors.green : Colors.red,
                shape: BoxShape.circle,
              ),
            ),
          ],
        ),
        backgroundColor: Colors.white,
        elevation: 0,
        centerTitle: true,
      ),
      body: Column(
        children: [
          _buildHeaderSection(),
          const SizedBox(height: 10),
          Expanded(child: _buildOrderList()),
        ],
      ),
    );
  }

  Widget _buildHeaderSection() {
    int processingCount = _orders.where((o) => o.status == 'processing').length;
    int readyCount = _orders.where((o) => o.status == 'ready').length;

    return Container(
      padding: const EdgeInsets.symmetric(vertical: 16, horizontal: 20),
      color: Colors.white,
      child: Row(
        children: [
          _buildCountCard('处理中', processingCount, Colors.blue),
          const SizedBox(width: 15),
          _buildCountCard('待取餐', readyCount, Colors.orange),
        ],
      ),
    );
  }

  Widget _buildCountCard(String label, int count, Color color) {
    return Expanded(
      child: Container(
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: color.withOpacity(0.1),
          borderRadius: BorderRadius.circular(12),
          border: Border.all(color: color.withOpacity(0.3)),
        ),
        child: Column(
          children: [
            Text(
              count.toString(),
              style: TextStyle(
                fontSize: 24,
                fontWeight: FontWeight.bold,
                color: color,
              ),
            ),
            const SizedBox(height: 4),
            Text(
              label,
              style: TextStyle(
                fontSize: 14,
                color: color.withOpacity(0.8),
                fontWeight: FontWeight.w500,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildOrderList() {
    if (_orders.isEmpty) {
      return Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(Icons.receipt_long, size: 64, color: Colors.grey[300]),
            const SizedBox(height: 10),
            Text(
              isConnected ? "暂无订单" : "连接服务器中...",
              style: TextStyle(color: Colors.grey[500]),
            ),
          ],
        ),
      );
    }

    return ListView.builder(
      padding: const EdgeInsets.symmetric(horizontal: 16),
      itemCount: _orders.length,
      itemBuilder: (context, index) {
        return _buildOrderItem(_orders[index]);
      },
    );
  }

  Widget _buildOrderItem(Order order) {
    bool isReady = order.status == 'ready';

    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      elevation: 2,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Row(
                  children: [
                    Container(
                      padding: const EdgeInsets.symmetric(
                        horizontal: 8,
                        vertical: 4,
                      ),
                      decoration: BoxDecoration(
                        color: Colors.black87,
                        borderRadius: BorderRadius.circular(6),
                      ),
                      child: Text(
                        '${order.tableId}号桌',
                        style: const TextStyle(
                          color: Colors.white,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                    ),
                    const SizedBox(width: 10),
                    Text(
                      '订单 #${order.id}',
                      style: TextStyle(color: Colors.grey[600], fontSize: 13),
                    ),
                  ],
                ),
                Text(
                  order.time,
                  style: TextStyle(color: Colors.grey[500], fontSize: 12),
                ),
              ],
            ),

            const Divider(height: 24),

            Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      ...order.items.map(
                        (item) => Padding(
                          padding: const EdgeInsets.only(bottom: 4),
                          child: Text(
                            item,
                            style: const TextStyle(
                              fontSize: 16,
                              fontWeight: FontWeight.w500,
                            ),
                          ),
                        ),
                      ),
                      const SizedBox(height: 8),
                      Text(
                        '¥${order.totalPrice}',
                        style: const TextStyle(
                          color: Colors.orange,
                          fontWeight: FontWeight.bold,
                          fontSize: 16,
                        ),
                      ),
                    ],
                  ),
                ),

                Center(
                  child: SizedBox(
                    height: 40,
                    child: ElevatedButton.icon(
                      onPressed: () => _remindCustomer(order),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: isReady
                            ? Colors.orange
                            : Colors.blueGrey,
                        foregroundColor: Colors.white,
                        elevation: 0,
                        padding: const EdgeInsets.symmetric(horizontal: 16),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(20),
                        ),
                      ),
                      icon: const Icon(Icons.notifications_none, size: 18),
                      label: Text(isReady ? '已通知' : '通知取餐'),
                    ),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
