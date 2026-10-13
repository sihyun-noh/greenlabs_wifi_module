# 센서 데이터 수신 확인 서버

별도 패키지 설치 없이 Python 3만으로 실행할 수 있습니다.

```bash
python3 tools/sensor_receiver.py
```

기본 포트는 `8080`이며 필요하면 변경할 수 있습니다.

```bash
python3 tools/sensor_receiver.py --port 9000
```

동작 확인:

```bash
curl http://localhost:8080/health

curl -X POST http://localhost:8080/api/v1/wifi-sensor \
  -H 'Content-Type: application/json' \
  -d '{"device_id":"test","flow":123}'
```

정상 수신 시 펌웨어가 기대하는 형식으로 응답합니다.

```json
{"code":"0000","result":"OK"}
```

ESP32에는 서버를 실행하는 PC의 실제 IP 주소를 지정해야 합니다.

```text
http://192.168.0.20:8080/api/v1/wifi-sensor
```
