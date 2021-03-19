## О проекте
Данный проект был создан для проверки вашего сборочного окружения на готовность работать с conan пакетами и корутинами C++

[[_TOC_]]

## Настройка окружения
Для начала вам необходимо установить пакетный менеджер conan и clang (минимум 11ой версии).
На MacOS это можно сделать через brew, для большиства последних версии Linux есть готовые пакеты
(возможно вам придется подключить дополнительный репозитарий).
conan ставится через `pip install conan`

Затем вам необходимо настроить conan. Вообще, достаточно добавить `CXXFLAGS=-fcoroutines-ts` в секцию `[env]`.
Но я приведу для примера содержимое своего профиля conan (~/.conan/profiles/default) с пояснениями:
```
[settings]
os=Macos
os_build=Macos
arch=x86_64
arch_build=x86_64
compiler=clang
compiler.version=11
#
# libc++ идет в комплекте с clang и содержит всё необходимое для работы с корутинами
# возможно, вам удастся использовать libstdc++, но я не пробовал
#
compiler.libcxx=libc++
compiler.cppstd=20
#
# для тестирования я всегда собираю программы с address sanitizer, но это не обязательно 
#
compiler.sanitizer=Address
build_type=Debug
[options]
[build_requires]
[env]
#
# -fcoroutines-ts - включает поддержку корутин
# -fsanitize=address - указывает компилятору, что необходимо использовать address sanitizer (не обязательно)
# -fprofile-instr-generate -fcoverage-mapping - это необходимо для измерения покрытия кода тестами (тоже не обязательно)
# -fno-omit-frame-pointer - точно не помню, почему я его использую, но подозреваю,
#  что без этого или покрытие некорретно считается, или отладчик не работает, или санитайзер 
#
CFLAGS=-fcoroutines-ts -fsanitize=address -fprofile-instr-generate -fcoverage-mapping -fno-omit-frame-pointer
CXXFLAGS=-fcoroutines-ts -fsanitize=address -fprofile-instr-generate -fcoverage-mapping -fno-omit-frame-pointer
#
# -shared-libsan - необходим опять же для санитайзера (как и явное указание линковаться с libc++)
# -L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib - это для сборки на MacOS Big Sur
#
LDFLAGS=-shared-libsan -pthread -ldl -lm -lc++ -L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib
#
# Если в вашей системе установлено более одного компилятора - лучше явно указать путь до него
# В противном случае conan может определить его неправильно 
#
CC=/usr/local/opt/ccache/libexec/clang
CXX=/usr/local/opt/ccache/libexec/clang++
```

## Сборка
Если ваше окружение настроено идеально, то проект должен успешно собраться следующей последовательностью команд:
```shell
# Выполняем из каталога с исходниками
mkdir build
cd build
conan install . --build missing
# передаем в cmake точно такой же набор параметров сборки как в профиле conan
# то, что мы указали его conan - помогает при сборке conan пакетов, но никак не влияет на cmake
cmake .. -DCMAKE_CXX_COMPILER=/usr/local/opt/ccache/libexec/clang++ -DCMAKE_CXX_FLAGS="-fcoroutines-ts -fsanitize=address -fprofile-instr-generate -fcoverage-mapping -fno-omit-frame-pointer -stdlib=libc++"
make
bin/cpp_test
```

Должны увидеть следующий результат:
```
% ./bin/cpp_test 
Before yield
Hello, World!
After yield
```

### Что может пойти не так
если возникают ошибки с conan - для начала убедитесь, что у вас свежая версия.
На момент написания этой инструкции мой conan имел версию 1.33.1

## Статическая линковка
Для того, чтобы не тащить с собой 100500 библиотек, которые,
даже будучи собранными без отладочной информации, могут занимать
многие сотни мегабайт я использую статическую линковку. Для этого
нужны другие настройки conan (пример со сборочного docker образа):
```
[settings]
os=Linux
os_build=Linux
arch=x86_64
arch_build=x86_64
compiler=clang
compiler.version=11
compiler.libcxx=libc++
build_type=Release
compiler.cppstd=20
compiler.sanitizer=None
[options]
[build_requires]
[env]
CC=clang
CFLAGS=-fPIC -fcoroutines-ts
CXX=clang++
CXXFLAGS=-fPIC -fcoroutines-ts
LDFLAGS=-fuse-ld=lld
```

Сам проект собираем следующим образом:
```shell
mkdir build
cd build
conan install ..
cmake -DCMAKE_BUILD_TYPE=RELEASE ..
make
```

## Оптимизация
При работе с conan пакетами, особенно при их разработке и тестировании,
эти пакеты приходится часто пересобирать. Причем conan всегда это делает "с нуля".
Тут очень спасает использование ccache (вы можете видеть по моим примерам, что я его использую).

Ну и второе, cmake - не очень быстрый инструмент. Ninja - гораздо быстрее.
Беда только в том, что мой IDE до недавнего времени не работал с ninja - не
мог позгрузить проект, если используется ninja. В остальном, этот инструмент полностью совместим с
cmake - использует тот же CMakeLists.txt и имеет те же параметры.
