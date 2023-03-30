# ThreadPool

В данном проекте реализуется ThreadPool на C++. ThreadPool поддерживает следующие возможности:
```
1. AddTask - добавить задание (функция, функтор, лямбда-выражение) на выполнение.

2. GetTaskResult - получить результат выполнения задания (возвращаемое функцией значение).

3. Wait - ожидать завершения выполнения определённого задания.

4. WaitAll - ожидать завершения выполнения всех заданий.
```

## Использование

Склонировать репозиторий:
```
git clone https://github.com/ArtemMaslov/ThreadPool
cd ThreadPool
```

Проект разрабатывался в Visual Studio Code, поэтому для удобства можно открыть workspace-файл `ThreadPool.code-workspace`.
Зайти в папке проекта, выполнив команду `cd ThreadPool` ещё раз.

Скомпилировать проект и запустить:
```
make
make run
```

Будет запущена тестовая программа, приближенно вычисляющая интеграл Пуассона.

Для использования ThreadPool в качестве библиотеки необходимо добавить в разрабатываемый проект исходные файлы `ThreadPool.h`, `ThreadPool.cpp`, `ThreadPool_impl.h`.