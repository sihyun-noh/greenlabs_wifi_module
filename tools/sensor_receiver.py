#!/usr/bin/env python3
"""Minimal HTTP server for inspecting sensor POST requests."""

import argparse
import json
from datetime import datetime
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


MAX_BODY_SIZE = 1024 * 1024


class SensorRequestHandler(BaseHTTPRequestHandler):
    server_version = "GreenlabsSensorReceiver/1.0"

    def _send_json(self, status: int, payload: dict) -> None:
        response = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(response)))
        self.end_headers()
        self.wfile.write(response)

    def do_GET(self) -> None:
        if self.path == "/health":
            self._send_json(200, {"status": "ok"})
            return

        self._send_json(404, {"success": False, "error": "not found"})

    def do_POST(self) -> None:
        try:
            content_length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self._send_json(400, {"success": False, "error": "invalid Content-Length"})
            return

        if content_length > MAX_BODY_SIZE:
            self._send_json(413, {"success": False, "error": "request body too large"})
            return

        body = self.rfile.read(content_length)
        text = body.decode("utf-8", errors="replace")

        try:
            displayed_body = json.dumps(
                json.loads(text), ensure_ascii=False, indent=2
            )
        except json.JSONDecodeError:
            displayed_body = text

        print("\n=== 센서 데이터 수신 ===")
        print(f"수신 시각: {datetime.now().astimezone().isoformat()}")
        print(f"송신 주소: {self.client_address[0]}:{self.client_address[1]}")
        print(f"요청 경로: {self.path}")
        print(f"Content-Type: {self.headers.get('Content-Type', '')}")
        print(displayed_body, flush=True)

        self._send_json(200, {"success": True})

    def log_message(self, message_format: str, *args: object) -> None:
        print(f"[HTTP] {self.address_string()} - {message_format % args}")


def main() -> None:
    parser = argparse.ArgumentParser(description="센서 HTTP POST 수신 확인 서버")
    parser.add_argument("--host", default="0.0.0.0", help="수신 주소")
    parser.add_argument("--port", type=int, default=8080, help="수신 포트")
    args = parser.parse_args()

    server = ThreadingHTTPServer((args.host, args.port), SensorRequestHandler)
    print(f"센서 수신 서버 시작: http://{args.host}:{args.port}")
    print("상태 확인: GET /health")
    print("센서 수신: POST /sensor-data")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n서버를 종료합니다.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
