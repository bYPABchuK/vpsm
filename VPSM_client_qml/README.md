# VPSM Qt 6 tunnel client

Linux/IPv4 TUN-клиент с Qt Quick GUI и headless-режимом.

## Реализовано

- Linux IPv4 TUN backend (`/dev/net/tun`, `IFF_TUN | IFF_NO_PI`);
- безопасная настройка interface/address/route/MTU через `iproute2` без shell;
- один `QUdpSocket` для отправки и приёма;
- ChaCha20-Poly1305 Data Plane V2, совместимый с `VPSM_router`;
- replay window;
- проверка IPv4 source/destination и subnet;
- один глобально уникальный VIP пользователя на сервер в общем overlay `10.240.0.0/16`;
- несколько logical networks на одном VIP с выбором outbound network по destination membership;
- authenticated self-KEEPALIVE перед переходом в `Connected`;
- `TunnelController` со state machine и rollback;
- QML-ready `TunnelViewModel` с `Q_PROPERTY`;
- platform factory для Linux/Stub backend;
- однофайловый Qt Quick UI (`Main.qml`);
- GUI executable `vpsm_client`;
- headless executable;
- QtTest unit/integration tests без root и offscreen QML smoke test.

## Сборка

```bash
cmake -S VPSM_client_qml -B build/vpsm-client-qml -DCMAKE_BUILD_TYPE=Debug
cmake --build build/vpsm-client-qml -j
ctest --test-dir build/vpsm-client-qml --output-on-failure
```

Для headless-only сборки без Qt Quick:

```bash
cmake -S VPSM_client_qml -B build/vpsm-client-headless \
  -DVPSM_BUILD_GUI=OFF
```

### Зависимости Gentoo

На Gentoo нужны Qt 6 с Qt Quick/QML, OpenSSL, CMake и Ninja. Если они не
установлены, используйте Portage и проверьте необходимые USE flags для Qt:

```bash
doas emerge --ask dev-build/cmake dev-build/ninja dev-libs/openssl \
  dev-qt/qtbase dev-qt/qtdeclarative
```

Выбор backend:

```bash
-DVPSM_PLATFORM_BACKEND=AUTO   # Linux на Linux, иначе Stub
-DVPSM_PLATFORM_BACKEND=LINUX
-DVPSM_PLATFORM_BACKEND=STUB
```

## GUI запуск

```bash
build/vpsm-client-qml/vpsm_client
```

`Main.qml` встроен в executable как Qt resource. GUI использует тот же
`AppViewModel`, `TunnelController`, UDP transport и platform backend, что и
headless-клиент. GUI всегда остаётся непривилегированным. Только при нажатии
`Подключить` он запускает `vpsm_net_helper` через первый доступный инструмент:
сначала `doas`, при его отсутствии — `sudo`. Helper создаёт и
настраивает TUN, передаёт его file descriptor клиенту через Unix `SCM_RIGHTS` и
остаётся для cleanup. Пароли, HTTP, UDP и ключи helper не получает. Текстовый
запрос системного пароля показывается в терминале (`Terminal=true` в desktop
entry). При ручном запуске используйте терминал: `doas`/`sudo` требуют
controlling TTY и не обязаны показывать графический password dialog.

## Headless запуск

Рекомендуемый режим сам выполняет login и join через Control Plane, получает
session key и overlay-конфигурацию, после чего поднимает TUN:

```bash
sudo build/vpsm-client-qml/vpsm_tunnel_headless \
  --control-url http://203.0.113.10:8080 \
  --router 203.0.113.10 \
  --router-port 4000 \
  --local-port 4001 \
  --nickname alice \
  --password user-password \
  --network-id 1 \
  --network-password network-password \
  --interface vpsm0
```

Для быстрого MVP executable запускается целиком через `sudo`: привилегии нужны
для `TUNSETIFF` и команд `ip addr/link/route`. На двух разных Linux-машинах
достаточно запустить по одному экземпляру. Два клиента на одной машине следует
помещать в разные network namespaces, иначе два одинаковых overlay route будут
конфликтовать в общей routing table.

## Привилегированный Docker integration test через doas

Полный headless-тест без UI запускает router и два клиента в отдельных Docker
network namespaces. Клиенты стартуют от пользователя `vpsm` через настроенный
внутри контейнера `doas`. Контейнерам выдаются только `NET_ADMIN` и
`/dev/net/tun`, без `--privileged`:

```bash
doas ./VPSM_client_qml/tests/integration/run_privileged_docker_test.sh
```

Тест собирает production targets и проверяет TUN, адреса, MTU, routes,
двусторонний ping, SSH и передачу файла по SCP через overlay. Контейнеры и
временная Docker-сеть удаляются автоматически. Для первого запуска может
потребоваться загрузка `archlinux:base` и пакетов из сети.

## Debian package

Минимальная целевая система — Debian 12 Bookworm с Qt 6.4. Пакет необходимо
собирать внутри самой старой версии Debian, которая будет
поддерживаться. Это позволяет `dpkg-shlibdeps` вычислить правильные ABI
зависимости для конкретного Debian release.

Build dependencies:

```bash
sudo apt update
sudo apt install --no-install-recommends \
  build-essential cmake ninja-build pkg-config \
  qt6-base-dev qt6-declarative-dev libssl-dev \
  qml6-module-qtquick qml6-module-qtquick-window dpkg-dev
```

Сборка и установка:

```bash
./VPSM_client_qml/packaging/debian/build_deb.sh
sudo apt install ./VPSM_client_qml/dist/vpsm-client_0.1.0_amd64.deb
```

Подробности находятся в `packaging/debian/README.md`. Пакет содержит GUI,
headless executable, привилегированный `vpsm_net_helper` и desktop entry;
Dependencies Qt/OpenSSL вычисляются через `dpkg-shlibdeps`, а `sudo | doas`, QML
runtime modules и `iproute2` указаны явно.

## Авторизация

GUI использует следующую последовательность:

1. IP сервера и неаутентифицированная проверка доступности (`GET /health`);
2. nickname и пароль пользователя (`POST /user/login`);
3. загрузка уже доступных пользователю сетей с пользовательской сессией;
4. выбор сети без повторного ввода network ID или пароля сети;
5. запуск TUN и authenticated UDP Data Plane.

Выбор и проверка сервера не требуют credentials. Операции с сетями требуют
успешного пользовательского login и заголовков сессии. Control Plane сейчас
использует HTTP. Перед передачей реальных паролей через
недоверенную сеть необходимо разместить его за HTTPS reverse proxy или добавить
TLS непосредственно в router. UDP Data Plane при этом уже защищён AEAD.

Ручной diagnostic режим также доступен:

```bash
sudo setcap cap_net_admin+ep build/vpsm-client-qml/vpsm_tunnel_headless

build/vpsm-client-qml/vpsm_tunnel_headless \
  --router 203.0.113.10 \
  --router-port 4000 \
  --local-port 4001 \
  --peer-id 1 \
  --session-id 123 \
  --session-key 456 \
  --data-key 64_HEX_CHARACTERS \
  --network-id 10 \
  --address 10.240.0.1 \
  --network 10.240.0.0 \
  --prefix 16 \
  --mtu 1400 \
  --interface vpsm0
```
