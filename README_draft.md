# esphome-dlms-cosem

Подключение ESPHome к счётчикам электроэнергии по DLMS/COSEM/СПОДЭС (Энергомера CE207/CE307/CE308, Милур 107S, Мир, Нартис, РиМ, Пульсар и др.) через RS‑485 или оптопорт (*).

(*) Работа через оптопорт поддерживается для приборов, которые сразу используют скорость 9600 бод. Сценарий «сначала 300 бод, затем переход на 9600» пока не реализован.

Связанная инструкция по физическому подключению ESP32/ESP8266 к счётчику: https://github.com/latonita/esphome-energomera-iec

---

## Кратко о возможностях
- Поддержка бинарного протокола DLMS/COSEM через HDLC
- Аутентификация: NONE и LOW (пароль)
- Чтение базовых числовых типов (int/float)
- Чтение текстовых данных (octet-string)
- Корректная работа с русским текстом (cp1251 → UTF‑8)
- Настройка логического и физического адресов
- Поддержка нескольких счётчиков на одной шине
- Работа в пассивном режиме (Push mode)

План на будущее:
- Чтение/запись дат и времени
- Синхронизация времени
- Управление реле
- Полноценная работа через оптопорт по стандартной процедуре (300 → 9600)

Если готовы помочь тестированием — пишите на anton.viktorov@live.com.

---

## Установка
Добавьте внешний компонент в конфигурацию ESPHome:

```yaml
external_components:
  - source: github://latonita/esphome-dlms-cosem
    components: [dlms_cosem]
    refresh: 1s
```

Требуется настроенный UART (RS‑485 через конвертер) или оптопорт.

---

## Быстрый старт
Минимальная конфигурация хаба и одного сенсора:

```yaml
uart:
  id: bus_1
  rx_pin: GPIO16
  tx_pin: GPIO17
  baud_rate: 9600
  data_bits: 8
  parity: NONE
  stop_bits: 1

dlms_cosem:
  client_address: 32
  server_address: 1      # см. документацию на ваш счётчик
  auth: true
  password: "12345678"
  update_interval: 60s

sensor:
  - platform: dlms_cosem
    name: Active Power
    obis_code: 1.0.1.7.0.255
    unit_of_measurement: W
    accuracy_decimals: 1
    device_class: power
    state_class: measurement
```

---

## Конфигурация хаба (`dlms_cosem`)

```yaml
dlms_cosem:
  client_address: 32
  server_address:
    logical_device: 1
    physical_device: 576
    address_length: 2
  auth: true
  password: "12345678"
  update_interval: 60s
  receive_timeout: 500ms
  delay_between_requests: 50ms
  flow_control_pin: GPIO12
  id: energo_01
  cp1251: true
  push_mode: false
```

Параметры:
- **client_address** (*Optional*) — уровень доступа клиента. Для чтения часто используется 32 (требует пароль). По умолчанию: 16.
- **server_address** (*Optional*) — HDLC‑адрес. Можно указать числом либо как объект. По умолчанию: 1.
  - **logical_device** (*Optional*) — логический адрес устройства. По умолчанию: 1.
  - **physical_device** (**Required**) — физический адрес устройства (часто зависит от серийного номера). См. инструкцию к счётчику.
  - **address_length** (*Optional*) — длина адреса (1|2|4 байта). По умолчанию: 2.
- **auth** (*Optional*) — включить авторизацию. По умолчанию: false.
- **password** (*Optional*) — пароль при LOW‑auth.
- **update_interval** (*Optional*) — период опроса. По умолчанию: 60s.
- **receive_timeout** (*Optional*) — таймаут ожидания ответа. По умолчанию: 500ms.
- **delay_between_requests** (*Optional*) — пауза между запросами. По умолчанию: 50ms.
- **flow_control_pin** (*Optional*) — пин управления направлением RE/DE RS‑485‑модуля.
- **id** (*Optional*) — идентификатор хаба (укажите, если их несколько).
- **cp1251** (*Optional*) — конвертация cp1251 → UTF‑8 для текстовых значений. По умолчанию: true.
- **push_mode** (*Optional*) — включить пассивный режим (Push mode), если поддерживается. По умолчанию: false.

### Адресация: client_address и server_address
- Если не указать, будут использованы значения по умолчанию (16 и 1). Но лучше свериться с документацией к конкретному счётчику.
- Часто используется client_address = 32 (требуется пароль). Уровни по СПОДЭС:

| Код | Уровень | Операции | Защита |
|-----|---------|----------|--------|
| 16  | Публичный клиент | чтение | нет |
| 32  | Считыватель показаний | чтение, выборка, отдельные действия | пароль |
| 48  | Конфигуратор | чтение/запись/выборка/действия | пароль или шифрование (*) |

(*) Шифрование пока не поддерживается.

server_address обычно двухбайтный: старший байт — логический адрес, младший — физический. Детали — в инструкции к счётчику.

Пример (Милур 107S): физический адрес = (последние 4 цифры серийного номера + 16).

---

## cp1251 и русские строки
Некоторые счётчики отдают строки в cp1251 (например, тип ПУ по `0.0.96.1.1.255`). Для корректного отображения в Home Assistant включите `cp1251: true` на уровне хаба или конкретного текстового сенсора. Если конвертация мешает, её можно отключить глобально или точечно в сенсоре.

---

## Сенсоры

### Числовой сенсор (`sensor`)
```yaml
sensor:
  - platform: dlms_cosem
    name: Phase Current
    obis_code: 1.0.11.7.0.255
    multiplier: 1.0        # предварительная мультипликация (до filters:)
    dont_publish: false    # не публиковать в шину (видно только в логах)
    unit_of_measurement: A
    accuracy_decimals: 1
    device_class: current
    state_class: measurement
```

### Текстовый сенсор (`text_sensor`)
```yaml
text_sensor:
  - platform: dlms_cosem
    name: Type
    obis_code: 0.0.96.1.1.255
    dont_publish: false
    # cp1251: false         # переопределение настройки хаба (опционально)
    entity_category: diagnostic
```
- **cp1251** — если указано у сенсора, перекрывает настройку хаба. Полезно для полей вроде `0.0.96.1.1.255` (тип ПУ на русском).

### Бинарные сенсоры (`binary_sensor`)
```yaml
binary_sensor:
  - platform: dlms_cosem
    connection:
      name: Connection      # есть связь с прибором
    session:
      name: Session         # активна сессия
    transmission:
      name: Transmission    # идёт обмен (каждый запрос)
```
Пример использования для индикации активности светодиодом:
```yaml
binary_sensor:
  - platform: dlms_cosem
    transmission:
      name: Transmission
      on_press:
        output.turn_on: transmission_led
      on_release:
        output.turn_off: transmission_led

output:
  - platform: gpio
    id: transmission_led
    pin: GPIO04
    inverted: true
```

---

## Несколько счётчиков

```yaml
uart:
  - id: bus_1
    rx_pin: GPIO16
    tx_pin: GPIO17
    baud_rate: 9600
    data_bits: 8
    parity: NONE
    stop_bits: 1

  - id: bus_2
    rx_pin: GPIO23
    tx_pin: GPIO22
    baud_rate: 9600
    data_bits: 8
    parity: NONE
    stop_bits: 1

dlms_cosem:
  - id: energo_1
    uart_id: bus_1
    client_address: 32
    server_address:
      logical_device: 1
      physical_device: 576
    auth: true
    password: "12345678"
    update_interval: 30s

  - id: energo_2
    uart_id: bus_1
    client_address: 32
    server_address:
      logical_device: 1
      physical_device: 16
    auth: true
    password: "12345678"
    update_interval: 30s

  - id: nartis_3
    uart_id: bus_2
    client_address: 32
    server_address:
      logical_device: 1
      physical_device: 17
      address_length: 2
    auth: true
    password: "00000001"

text_sensor:
  - platform: dlms_cosem
    dlms_cosem_id: energo_2
    name: Serial number
    obis_code: 0.0.96.1.0.255
    entity_category: diagnostic
    cp1251: true

sensor:
  - platform: dlms_cosem
    dlms_cosem_id: nartis_3
    name: Current
    obis_code: 1.0.11.7.0.255
    unit_of_measurement: A
    accuracy_decimals: 1
    device_class: current
    state_class: measurement
```

---

## Lambda‑помощники

Доступна функция `update_server_address(logical_device, physical_device, addr_len)` — обновляет адрес и инициирует внеочередной опрос. Ниже пример перебора адресов вместе с бинарным сенсором `Connection` и выводом текущих значений в лог:

```yaml
globals:
  - id: logaddr
    type: uint16_t
    initial_value: '1'

  - id: phyaddr
    type: uint16_t
    initial_value: '1'

  - id: servaddr
    type: uint16_t
    initial_value: '1'

binary_sensor:
  - platform: dlms_cosem
    connection:
      name: Connection
      on_release:
        - lambda: |-
            if (id(phyaddr) < 255) {
              id(phyaddr)++;
            } else {
              id(logaddr)++;
              id(phyaddr) = 1;
            }
            id(servaddr) = id(energo_01)->update_server_address(id(logaddr), id(phyaddr), 2);
        - lambda: |-
            ESP_LOGI("main", "Logical: %d, physical: %d, server: %d", (int) id(logaddr), (int) id(phyaddr), (int) id(servaddr));

interval:
  interval: 10s
  then:
    - lambda: |-
        ESP_LOGI("main", "Logical: %d, physical: %d, server: %d", (int) id(logaddr), (int) id(phyaddr), (int) id(servaddr));
```

---

## Особенности отдельных счётчиков

### Нартис И100‑W112
- Тип ПУ отдаётся на русском языке. Для `0.0.96.1.1.255` включите `cp1251: true`.
- Встречались расхождения паролей/адресов между инструкциями и реальностью. Пример рабочих параметров:
  - Пароль администрирования: `0000000100000001`
  - Пароль чтения: `00000001`
  - Логический адрес: `1`
  - Физический адрес: `17`
  - Размер адреса: `2`

### РиМ 489.38 (и серия)
- Тип ПУ на русском — для `0.0.96.1.1.255` используйте `cp1251: true`.

---

## Пример: однофазный счётчик (ПУ категории D)
Набор параметров ПУ категории D по СПОДЭС. Проверено на Энергомера CE207‑SPds.

```yaml
esphome:
  name: energomera-ce207-spds
  friendly_name: Energomera-ce207-spds

esp32:
  board: esp32dev
  framework:
    type: arduino

external_components:
  - source: github://latonita/esphome-dlms-cosem
    components: [dlms_cosem]
    refresh: 1s

uart:
  id: bus_1
  rx_pin: GPIO16
  tx_pin: GPIO17
  baud_rate: 9600
  data_bits: 8
  parity: NONE
  stop_bits: 1

dlms_cosem:
  id: energo_01
  client_address: 32
  server_address: 1
  auth: true
  password: "12345678"
  update_interval: 60s
  receive_timeout: 1s

sensor:
  - platform: dlms_cosem
    name: Phase Current
    obis_code: 1.0.11.7.0.255
    unit_of_measurement: A
    accuracy_decimals: 1
    device_class: current
    state_class: measurement
  - platform: dlms_cosem
    name: Neutral Current
    obis_code: 1.0.91.7.0.255
    unit_of_measurement: A
    accuracy_decimals: 1
    device_class: current
    state_class: measurement
  - platform: dlms_cosem
    name: Phase Voltage
    obis_code: 1.0.12.7.0.255
    unit_of_measurement: V
    accuracy_decimals: 1
    device_class: voltage
    state_class: measurement
  - platform: dlms_cosem
    name: Power Factor
    obis_code: 1.0.13.7.0.255
    unit_of_measurement: ''
    accuracy_decimals: 2
    device_class: power_factor
    state_class: measurement
  - platform: dlms_cosem
    name: Grid Frequency
    obis_code: 1.0.14.7.0.255
    unit_of_measurement: Hz
    accuracy_decimals: 1
    device_class: frequency
    state_class: measurement
  - platform: dlms_cosem
    name: Apparent Power
    obis_code: 1.0.9.7.0.255
    unit_of_measurement: W
    accuracy_decimals: 1
    device_class: power
    state_class: measurement
  - platform: dlms_cosem
    name: Active Power
    obis_code: 1.0.1.7.0.255
    unit_of_measurement: W
    accuracy_decimals: 1
    device_class: power
    state_class: measurement
  - platform: dlms_cosem
    name: Reactive Power
    obis_code: 1.0.3.7.0.255
    unit_of_measurement: W
    accuracy_decimals: 1
    device_class: power
    state_class: measurement
  - platform: dlms_cosem
    name: Active Energy
    obis_code: 1.0.1.8.0.255
    unit_of_measurement: kWh
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.001
  - platform: dlms_cosem
    name: Active Energy T1
    obis_code: 1.0.1.8.1.255
    unit_of_measurement: kWh
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.001
  - platform: dlms_cosem
    name: Active Energy T2
    obis_code: 1.0.1.8.2.255
    unit_of_measurement: kWh
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.001
  - platform: dlms_cosem
    name: Active Energy Export
    obis_code: 1.0.2.8.0.255
    unit_of_measurement: kWh
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.001
  - platform: dlms_cosem
    name: Reactive Energy
    obis_code: 1.0.3.8.0.255
    unit_of_measurement: kWh
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.001
  - platform: dlms_cosem
    name: Reactive Energy Export
    obis_code: 1.0.4.8.0.255
    unit_of_measurement: kWh
    accuracy_decimals: 3
    device_class: energy
    state_class: total_increasing
    filters:
      - multiply: 0.001

text_sensor:
  - platform: dlms_cosem
    name: Serial Number
    obis_code: 0.0.96.1.0.255
    entity_category: diagnostic
  - platform: dlms_cosem
    name: Type
    obis_code: 0.0.96.1.1.255
    cp1251: true
    entity_category: diagnostic
  - platform: dlms_cosem
    name: Metrology Software Version
    obis_code: 0.0.96.1.2.255
    entity_category: diagnostic
  - platform: dlms_cosem
    name: Manufacturer
    obis_code: 0.0.96.1.3.255
    entity_category: diagnostic
```

---

## Диагностика и советы
- Нет ответа: проверьте RE/DE (flow_control_pin) и направление конвертера RS‑485
- Таймауты: увеличьте `receive_timeout` и `delay_between_requests`
- Адреса: перепроверьте `client_address` и `server_address` по инструкции (учитывайте адреса для оптопорта vs RS‑485)
- Русский текст «кракозябрами»: включите `cp1251: true`

---

## Лицензия
См. LICENSE в репозитории.
