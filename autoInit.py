
# 开始覆盖原文件
print("开始覆盖CMake原文件")
with open('./MAINCmakeBack.cp', 'r', encoding='utf-8') as src:
    content = src.read()

with open('./CMakeLists.txt', 'w', encoding='utf-8') as dst:
    dst.write(content)
print("CMake覆盖原完成")

print("开始覆盖链接文件")
with open('./FLASHLD.cp', 'r', encoding='utf-8') as src:
    content = src.read()

with open('./STM32F103C8Tx_FLASH.ld', 'w', encoding='utf-8') as dst:
    dst.write(content)
print("链接文件覆盖成功")