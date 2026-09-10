# Control Plane API (актуальные ручки)

Ниже описаны только **активные** маршруты, зарегистрированные в `ControlPlaneBoost`.

## Auth

Подключение и проверка доступности router не требуют авторизации:

```http
GET /health
```

Пользовательская session-авторизация нужна для всех дальнейших `/user/*` и
`/network/*`, кроме самого `/user/login`. Login по nickname/password выдаёт
Control/Data Plane session.

Поддерживаемые пары заголовков:
- `sessionId` / `sessionKey`
- `SessionId` / `SessionKey`
- `X-Session-Id` / `X-Session-Key`
- `x-session-id` / `x-session-key`

При отсутствии/невалидности сессии возвращается `401 auth_required`.

Сессия по умолчанию действует 24 часа. `DELETE /user/logout` отзывает её сразу
одновременно для Control Plane и Data Plane.

> Важно: встроенный HTTP listener пока не использует TLS. В публичной сети его
> необходимо размещать за TLS reverse proxy; иначе login credentials и ключ
> Data Plane могут быть перехвачены до начала защищённого UDP-обмена.

Для SDUI-ручек есть отдельное поведение:
- `GET /ui/screens/main-body`: `401` при отсутствии/битых session headers, `403` при невалидной сессии.
- `GET /ui/screens/license-screen`: публичная ручка, без обязательной сессии.

---

## SDUI

### `GET /ui/screens/main-body`

Требует заголовки сессии:
- `sessionId`
- `sessionKey`

Успех (`200`):
```json
{
  "ok": true,
  "screen": {
    "schemaVersion": 1,
    "screenId": "main-body",
    "components": []
  }
}
```

Ошибки:
- `401 auth_required` — заголовки отсутствуют или не парсятся
- `403 forbidden` — сессия невалидна/просрочена
- `404 not_supported` — файл экрана отсутствует
- `500 internal_error` — ошибка чтения/парсинга

### `GET /ui/screens/license-screen`

Публичная ручка.

Успех (`200`):
```json
{
  "ok": true,
  "screen": {
    "title": "Лицензия сервера",
    "text": "Server is licensed under ...",
    "styleRef": "body"
  }
}
```

Также поддерживается конфиг с обёрткой `screen` в файле — сервер корректно вернёт вложенный объект.

Ошибки:
- `404 not_supported` — файл плашки отсутствует
- `500 internal_error` — ошибка чтения/парсинга

### Файлы конфигурации SDUI

По умолчанию:
- `config/ui/main-body.json`
- `config/ui/license-screen.json`

Можно переопределить через env:
- `VPSM_UI_MAIN_BODY_FILE`
- `VPSM_UI_LICENSE_screen_FILE`

SDUI mini-service кэширует JSON в памяти и проверяет обновления по `mtime` не чаще, чем раз в 1 секунду.

---

## 1) Login

### `POST /user/login`

Request JSON:
```json
{
  "nickname": "str", 
  "passwordHash": "str"
}
```

Дополнительно поддерживается legacy-формат полей:
- `nick` вместо `nickname`
- `password` вместо `passwordHash`

Response JSON (успех):
```json
{
  "ok": true,
  "peerId": "1",
  "sessionId": "100",
  "sessionKey": "200",
  "dataPlaneKey": "64-hex-characters"
}
```

64-битные идентификаторы и credentials возвращаются строками, чтобы исключить
потерю точности JSON number. `dataPlaneKey` — 256-битный ключ в hex.

### `DELETE /user/logout`

Требует обычные session headers. При успехе возвращает `200 {"ok":true}` и
немедленно отзывает HTTP- и UDP-доступ этой сессии.

---

## Data Plane V2 authentication

UDP packet имеет формат:

```text
version(1) | sessionId(8) | sequence(8)
| ChaCha20-Poly1305 ciphertext(inner header + payload)
| authentication tag(16)
```

Outer header используется как AEAD associated data. Inner header содержит:

```text
packetType(1) | networkId(4) | srcVip(4) | dstVip(4)
```

Nonce разделён по направлениям:

- `1 || sequence` — client → router;
- `2 || sequence` — router → client.

Router проверяет AEAD tag, срок и отзыв сессии, replay window и соответствие
`authenticatedPeerId` заявленному `srcVip`. Endpoint обновляется только после
всех этих проверок. При пересылке router расшифровывает входной пакет и повторно
шифрует его ключом активной сессии получателя.

---

## 2) Создание сети

### `POST /network/create`

Request JSON:
```json
{
  "name": "my-network",
  "passwordHash": "secret"
}
```

Response JSON (успех):
```json
{
  "ok": true,
  "networkId": "10"
}
```

Важная бизнес-логика:
- owner определяется по authenticated session; доверять `ownerPeerId` из body не требуется;
- имя сети уникально;
- при попытке создать сеть с уже существующим именем: `409 network_name_already_exists`.

---

## 3) Добавить текущего пользователя в сеть

### `PUT /network/{id}/user-add`

`{id}` — networkId из path.

Request body:
- может быть пустым;
- если есть body, то только JSON;
- поддерживается поле `passwordHash`.

Пример:
```json
{
  "passwordHash": "secret"
}
```

Response JSON (успех):
```json
{
  "ok": true,
  "networkId": "1",
  "vip": "10.240.1.1",
  "address": "10.240.1.1",
  "networkAddress": "10.240.1.0",
  "prefixLength": 24,
  "mtu": 1400,
  "alreadyExists": false
}
```

Если пользователь уже в сети, возвращается:
- `status = 208`
- `ok = true`
- `alreadyExists = true`
- текущий `vip`.

---

## 4) Список сетей пользователя

### `GET /user/{id}/network-list`

`{id}` должен совпадать с `authenticatedPeerId`, иначе `403 forbidden`.

Response JSON:
```json
{
  "ok": true,
  "networks": [
    {
      "id": "1",
      "name": "my-network",
      "ownerPeerId": "1",
      "address": "10.240.1.1",
      "vip": "10.240.1.1",
      "networkAddress": "10.240.1.0",
      "prefixLength": 24,
      "mtu": 1400
    }
  ]
}
```

---

## 5) Сети пользователя + участники

### `GET /user/{id}/network-peers-list`

`{id}` должен совпадать с `authenticatedPeerId`, иначе `403 forbidden`.

Response JSON:
```json
{
  "ok": true,
  "networks": [
    {
      "id": "1",
      "name": "my-network",
      "ownerPeerId": "1",
      "address": "10.240.1.1",
      "networkAddress": "10.240.1.0",
      "prefixLength": 24,
      "mtu": 1400,
      "peers": [
        {
          "peerId": "1",
          "vip": "10.240.1.1"
        }
      ]
    }
  ]
}
```

---

## 6) Список участников сети

### `GET /network/{id}/peers-list`

Response JSON:
```json
{
  "ok": true,
  "peers": [
    {
      "peerId": "1",
      "vip": "10.240.1.1"
    }
  ]
}
```

---

## 7) Вход в сеть (REST)

Основной endpoint для GUI использует уникальное имя сети:

### `PUT /network/join`

```json
{
  "name": "my-network",
  "passwordHash": "secret"
}
```

`peerId` определяется по authenticated session. Успешный ответ содержит
`networkId`, VIP и параметры overlay, необходимые Data Plane.

Для совместимости headless/старых клиентов также доступен ID-вариант:

### `PUT /user/networks/{networkId}/members/{peerId}`

Request JSON:
```json
{
  "passwordHash": "secret"
}
```

Response JSON (успех):
```json
{
  "ok": true,
  "networkId": "1",
  "vip": "10.240.1.1",
  "address": "10.240.1.1",
  "networkAddress": "10.240.1.0",
  "prefixLength": 24,
  "mtu": 1400
}
```

---

## 8) Выход из сети (REST)

### `DELETE /user/networks/{networkId}/members/{peerId}`

Request JSON (требуется JSON body, можно пустой объект):
```json
{}
```

Response JSON (успех):
```json
{
  "ok": true
}
```

---

## Типовые ошибки

Общий формат:
```json
{
  "ok": false,
  "error": "error_code"
}
```

Часто встречающиеся коды:
- `400`: `invalid_payload`, `invalid_id`, `peer_not_found`, `network_not_found`, `not_member`, `create_failed`, `delete_failed`
- `401`: `auth_required`
- `403`: `forbidden`, `invalid_password`
- `405`: `method_not_allowed`
- `409`: `network_name_already_exists`
- `500`: `internal_error`

Специальный успешный статус:
- `208` для `PUT /network/{id}/user-add`, если пользователь уже состоит в сети.

