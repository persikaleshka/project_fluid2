---
# project_fluid2
Название: "Это второй проект по с++"

Чтобы запустить вам нужно: |
mkdir build
cd build
cmake -DTYPES=float,"FAST_FIXED(13,7)","FIXED(64,15)",double .. (можно с другими типами и числами, ниже написано об этом)
cmake --build .
./project2 --p-type=float, --v-type="FIXED(64,15)", --v-flow-type="FAST_FIXED(13,7)"

Типы: |
Множество типов при генерации могут вызвать проблемы с памятью, рекомендуется использовать не более трех типов или просто увеличить размер стека.

Программу можно запустить с произвольными типами через cmake -DTYPES=типы .. (пример: cmake -DTYPES=float,double,"FIXED(32,16)",FAST_FIXED\\(32,16\\) ..)символы () нужно либо экранировать через \, либо писать как строку, используя кавычки.
При работе программы памяти хватает на комбинации из 2х типов, чтобы использовать большее количество комбинаций - нужно прописать команду `ulimit -s bytes` при запуске. 
Программа работает без потери производительности
