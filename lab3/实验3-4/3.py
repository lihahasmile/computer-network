import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

# 数据
# categories = [1, 2, 3, 4, 5]
# stopwait_time = [50, 1048, 1565, 1862, 2051, 2055]  # 停等协议的时间
# sliding_time = [1528, 1531, 2536, 1028, 1029, 2535]  # 滑动窗口协议的时间
# stop_wait= [1772.28 , 1186.81 , 1021.63 , 905.58 , 903.82]
# sliding=[1213.16 , 732.39 , 1806.76 , 1805.01 , 732.68]
categories = [1,5,10]
have = [892, 1269,3354]  # 停等协议的时间
no = [1013,1530,3952]  # 滑动窗口协议的时间
no1=[1237,2025,4102]

# categories = [0,5,10]
# have = [2531, 3032,3534]  # 停等协议的时间
# no = [2031,2530,2036]  # 滑动窗口协议的时间
# have1 = [1028,2030,4541]  # 停等协议的时间
# no1= [2036,1529,2033]  # 滑动窗口协议的时间
# 初始化图表样式
sns.set_theme(style="whitegrid")

# 创建图表和坐标轴
fig, ax1 = plt.subplots(figsize=(10, 6))

# 绘制两个协议的时间折线图
ax1.plot(categories, have, marker='o', label='5', color='b')
ax1.plot(categories, no, marker='s', label='10', color='g')
# ax1.plot(categories, have1, marker='o', label='15', color='orange')
ax1.plot(categories, no1, marker='s', label='20', color='r')
ax1.set_xlabel('latency/ms', fontsize=12)
ax1.set_ylabel('time/ms', fontsize=12, color='black')  # Y轴标签设置为“时间 (ms)”
ax1.tick_params(axis='y', labelcolor='black')

# 提取图例并显示
lines_1, labels_1 = ax1.get_legend_handles_labels()
ax1.legend(lines_1, labels_1, loc='upper left', fontsize=20)  # 添加图例并设置位置

# 设置标题和网格
# plt.title('丢包率与时间的关系', fontsize=14)
# plt.tight_layout()

# 显示图表
plt.show()
